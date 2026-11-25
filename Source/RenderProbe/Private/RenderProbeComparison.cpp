// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeComparison.h"

#include "HAL/FileManager.h"
#include "ImageComparer.h"
#include "Math/Color.h"
#include "Math/IntRect.h"
#include "Misc/Optional.h"
#include "Misc/Paths.h"

namespace RenderProbeComparisonLocal
{
	static constexpr const TCHAR* WriteDeltaStep = TEXT("writing the difference image");
	static constexpr double FullDifference = 1.0;
	static constexpr double MaxChannelValue = 255.0;

	struct FCaptureImages
	{
		FComparableImage A;
		FComparableImage ARepeat;
		FComparableImage B;
		FComparableImage BRepeat;
	};

	static bool LoadImage(const FString& Path, FComparableImage& OutImage)
	{
		FText Error;
		return !Path.IsEmpty() && OutImage.LoadFile(Path, Error);
	}

	static bool LoadCaptures(const FRenderProbeComparisonCaptures& Captures, FCaptureImages& OutImages)
	{
		return LoadImage(Captures.A, OutImages.A)
			&& LoadImage(Captures.ARepeat, OutImages.ARepeat)
			&& LoadImage(Captures.B, OutImages.B)
			&& LoadImage(Captures.BRepeat, OutImages.BRepeat);
	}

	static FImageComparisonResult CompareImages(const FString& PathA, const FString& PathB, const FComparableImage& A, const FComparableImage& B,
		const FImageTolerance& Tolerance, const FString& DeltaPath)
	{
		FImageComparer Comparer;
		FImageComparisonResult Comparison = Comparer.Compare(&A, &B, Tolerance, DeltaPath);
		Comparison.ApprovedFilePath = PathA;
		Comparison.IncomingFilePath = PathB;
		return Comparison;
	}

	static FString MakeNondeterministicDetail(const FString& Subject, const FRenderProbeCompare& Compare, const FString& CameraName)
	{
		return FString::Printf(TEXT("two captures of '%s' from camera '%s' differ under the tolerance of compare '%s'"),
			*Subject, *CameraName, *Compare.Name);
	}

	static bool IsDeltaErrorPixel(const FColor& Pixel)
	{
		return Pixel != FColor::Black;
	}

	static TOptional<FIntRect> ReadDiffBounds(const FString& DeltaImagePath)
	{
		FComparableImage Delta;
		if (!LoadImage(DeltaImagePath, Delta))
		{
			return {};
		}

		TOptional<FIntRect> Bounds;
		for (int32 Y = 0; Y < Delta.Height; ++Y)
		{
			for (int32 X = 0; X < Delta.Width; ++X)
			{
				if (!IsDeltaErrorPixel(Delta.GetPixel(X, Y)))
				{
					continue;
				}
				const FIntRect PixelRect(X, Y, X + 1, Y + 1);
				if (Bounds.IsSet())
				{
					Bounds->Union(PixelRect);
				}
				else
				{
					Bounds = PixelRect;
				}
			}
		}
		return Bounds;
	}

	static double PixelDifference(const FComparableImage& A, const FComparableImage& B, const int32 X, const int32 Y)
	{
		if (!A.CanGetPixel(X, Y) || !B.CanGetPixel(X, Y))
		{
			return FullDifference;
		}
		const FColor PixelA = A.GetPixel(X, Y);
		const FColor PixelB = B.GetPixel(X, Y);
		const int32 Largest = FMath::Max(
			FMath::Max(FMath::Abs(PixelA.R - PixelB.R), FMath::Abs(PixelA.G - PixelB.G)),
			FMath::Max(FMath::Abs(PixelA.B - PixelB.B), FMath::Abs(PixelA.A - PixelB.A)));
		return Largest / MaxChannelValue;
	}

	static double MaxDifferenceInside(const FComparableImage& A, const FComparableImage& B, const FIntRect& Bounds)
	{
		double Largest = 0.0;
		for (int32 Y = Bounds.Min.Y; Y < Bounds.Max.Y; ++Y)
		{
			for (int32 X = Bounds.Min.X; X < Bounds.Max.X; ++X)
			{
				Largest = FMath::Max(Largest, PixelDifference(A, B, X, Y));
			}
		}
		return Largest;
	}
}

namespace RenderProbeComparison
{
	FImageTolerance ToEngineTolerance(const ERenderProbeTolerance Tolerance)
	{
		switch (Tolerance)
		{
		case ERenderProbeTolerance::Zero:
			return FImageTolerance::DefaultIgnoreNothing;
		case ERenderProbeTolerance::Low:
			return FImageTolerance::DefaultIgnoreLess;
		case ERenderProbeTolerance::AntiAliasing:
			return FImageTolerance::DefaultIgnoreAntiAliasing;
		case ERenderProbeTolerance::Colors:
			return FImageTolerance::DefaultIgnoreColors;
		}
		checkf(false, TEXT("Unhandled ERenderProbeTolerance value %d"), static_cast<int32>(Tolerance));
		return FImageTolerance::DefaultIgnoreNothing;
	}

	FRenderProbeComparisonOutcome Judge(const FRenderProbeCompare& Compare, const FString& CameraName, const FRenderProbeComparisonCaptures& Captures, const FString& DeltaPath)
	{
		using namespace RenderProbeComparisonLocal;

		FRenderProbeComparisonOutcome Outcome;
		Outcome.Result.Name = Compare.Name;
		Outcome.Result.A = Compare.A;
		Outcome.Result.B = Compare.B;
		Outcome.Result.Camera = CameraName;

		FCaptureImages Images;
		if (!LoadCaptures(Captures, Images))
		{
			return Outcome;
		}

		const FImageTolerance Tolerance = ToEngineTolerance(Compare.Tolerance);
		const bool bADeterministic = CompareImages(Captures.A, Captures.ARepeat, Images.A, Images.ARepeat, Tolerance, FString()).AreSimilar();
		if (!bADeterministic)
		{
			Outcome.SubjectDiagnostics.Add({ Compare.A, FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::Nondeterministic, FString(),
				MakeNondeterministicDetail(Compare.A, Compare, CameraName)) });
		}
		const bool bSameSubject = Compare.A.Equals(Compare.B, ESearchCase::CaseSensitive);
		const bool bBDeterministic = bSameSubject
			|| CompareImages(Captures.B, Captures.BRepeat, Images.B, Images.BRepeat, Tolerance, FString()).AreSimilar();
		if (!bBDeterministic)
		{
			Outcome.SubjectDiagnostics.Add({ Compare.B, FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::Nondeterministic, FString(),
				MakeNondeterministicDetail(Compare.B, Compare, CameraName)) });
		}
		if (!bADeterministic || !bBDeterministic)
		{
			return Outcome;
		}

		IFileManager::Get().MakeDirectory(*FPaths::GetPath(DeltaPath), true);
		const FImageComparisonResult Comparison = CompareImages(Captures.A, Captures.B, Images.A, Images.B, Tolerance, DeltaPath);
		Outcome.Result.Similar = Comparison.AreSimilar();
		Outcome.Result.GlobalDifference = Comparison.GlobalDifference;

		if (Comparison.GlobalDifference <= 0.0)
		{
			if (!Comparison.ComparisonFilePath.IsEmpty())
			{
				IFileManager::Get().Delete(*Comparison.ComparisonFilePath);
			}
			Outcome.Result.MaxLocalDifference = 0.0;
			return Outcome;
		}

		const TOptional<FIntRect> Bounds = ReadDiffBounds(Comparison.ComparisonFilePath);
		if (!Bounds.IsSet())
		{
			Outcome.JobDiagnostics.Add(FRenderProbeDiagnostic::MakeOutputFailed(DeltaPath, WriteDeltaStep));
			return Outcome;
		}
		Outcome.Result.DiffBounds = Bounds;
		Outcome.Result.MaxLocalDifference = MaxDifferenceInside(Images.A, Images.B, Bounds.GetValue());
		Outcome.Result.DeltaImage = Comparison.ComparisonFilePath;
		return Outcome;
	}
}
