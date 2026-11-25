// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "HAL/FileManager.h"
#include "ImageComparer.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Math/Color.h"
#include "Math/IntRect.h"
#include "Misc/Paths.h"
#include "RenderProbeComparison.h"
#include "RenderProbeDiagnostic.h"
#include "RenderProbeJob.h"
#include "Tests/Support/RenderProbeTestOutputDir.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeComparisonTests
{

static constexpr int32 ImageWidth = 32;
static constexpr int32 ImageHeight = 32;
static constexpr int32 ImagePixelCount = ImageWidth * ImageHeight;
static constexpr int32 BlockMinX = 4;
static constexpr int32 BlockMinY = 6;
static constexpr int32 BlockMaxX = 9;
static constexpr int32 BlockMaxY = 9;
static constexpr int32 BlockPixelCount = (BlockMaxX - BlockMinX) * (BlockMaxY - BlockMinY);
static constexpr uint8 BlockGray = 128;
static constexpr uint8 SlightGray = 10;
static constexpr double DifferenceTolerance = 1e-6;
static constexpr const TCHAR* CompareName = TEXT("pair");
static constexpr const TCHAR* SubjectA = TEXT("first");
static constexpr const TCHAR* SubjectB = TEXT("second");
static constexpr const TCHAR* CameraName = TEXT("front");
static constexpr const TCHAR* DeltaFileName = TEXT("diff/pair__front.png");
static constexpr const TCHAR* CaptureAFile = TEXT("a.png");
static constexpr const TCHAR* RepeatAFile = TEXT("a_repeat.png");
static constexpr const TCHAR* CaptureBFile = TEXT("b.png");
static constexpr const TCHAR* RepeatBFile = TEXT("b_repeat.png");
static constexpr const TCHAR* SlightRepeatAFile = TEXT("a_repeat_slight.png");
static constexpr const TCHAR* WriteDeltaStep = TEXT("writing the difference image");

static bool IsSameTolerance(const FImageTolerance& Actual, const FImageTolerance& Expected)
{
	return Actual.Red == Expected.Red
		&& Actual.Green == Expected.Green
		&& Actual.Blue == Expected.Blue
		&& Actual.Alpha == Expected.Alpha
		&& Actual.MinBrightness == Expected.MinBrightness
		&& Actual.MaxBrightness == Expected.MaxBrightness
		&& Actual.IgnoreAntiAliasing == Expected.IgnoreAntiAliasing
		&& Actual.IgnoreColors == Expected.IgnoreColors
		&& Actual.MaximumLocalError == Expected.MaximumLocalError
		&& Actual.MaximumGlobalError == Expected.MaximumGlobalError;
}

static FImage MakeBlackImage()
{
	FImage Image(ImageWidth, ImageHeight, ERawImageFormat::BGRA8);
	for (FColor& Pixel : Image.AsBGRA8())
	{
		Pixel = FColor::Black;
	}
	return Image;
}

static FImage MakeImageWithBlock(const uint8 Gray)
{
	FImage Image = MakeBlackImage();
	const TArrayView64<FColor> Pixels = Image.AsBGRA8();
	for (int32 Y = BlockMinY; Y < BlockMaxY; ++Y)
	{
		for (int32 X = BlockMinX; X < BlockMaxX; ++X)
		{
			Pixels[static_cast<int64>(Y) * ImageWidth + X] = FColor(Gray, Gray, Gray, MAX_uint8);
		}
	}
	return Image;
}

static FRenderProbeCompare MakeCompare(const ERenderProbeTolerance Tolerance)
{
	FRenderProbeCompare Compare;
	Compare.Name = CompareName;
	Compare.A = SubjectA;
	Compare.B = SubjectB;
	Compare.Tolerance = Tolerance;
	return Compare;
}

TEST_CLASS(RenderProbeToleranceMapping, "RenderProbe.Unit.Compare")
{
	TEST_METHOD(EachPresetMapsToTheMatchingEngineTolerance)
	{
		ASSERT_THAT(IsTrue(IsSameTolerance(RenderProbeComparison::ToEngineTolerance(ERenderProbeTolerance::Zero), FImageTolerance::DefaultIgnoreNothing)));
		ASSERT_THAT(IsTrue(IsSameTolerance(RenderProbeComparison::ToEngineTolerance(ERenderProbeTolerance::Low), FImageTolerance::DefaultIgnoreLess)));
		ASSERT_THAT(IsTrue(IsSameTolerance(RenderProbeComparison::ToEngineTolerance(ERenderProbeTolerance::AntiAliasing), FImageTolerance::DefaultIgnoreAntiAliasing)));
		ASSERT_THAT(IsTrue(IsSameTolerance(RenderProbeComparison::ToEngineTolerance(ERenderProbeTolerance::Colors), FImageTolerance::DefaultIgnoreColors)));
	}
};

TEST_CLASS(RenderProbeComparisonJudge, "RenderProbe.Unit.Compare")
{
	FRenderProbeTestOutputDir OutputDir;

	FString SaveImage(const FString& FileName, const FImage& Image)
	{
		const FString Path = FPaths::Combine(OutputDir.Get(), FileName);
		if (!FImageUtils::SaveImageByExtension(*Path, Image))
		{
			TestRunner->AddError(FString::Printf(TEXT("could not write the synthetic capture '%s'"), *Path));
		}
		return Path;
	}

	FString DeltaPath() const
	{
		return FPaths::Combine(OutputDir.Get(), DeltaFileName);
	}

	FRenderProbeComparisonCaptures DeterministicCaptures(const FImage& A, const FImage& B)
	{
		FRenderProbeComparisonCaptures Captures;
		Captures.A = SaveImage(CaptureAFile, A);
		Captures.ARepeat = SaveImage(RepeatAFile, A);
		Captures.B = SaveImage(CaptureBFile, B);
		Captures.BRepeat = SaveImage(RepeatBFile, B);
		return Captures;
	}

	TEST_METHOD(IdenticalCapturesAreSimilarWithoutDifferenceImage)
	{
		const FRenderProbeComparisonCaptures Captures = DeterministicCaptures(MakeImageWithBlock(BlockGray), MakeImageWithBlock(BlockGray));

		const FRenderProbeComparisonOutcome Outcome = RenderProbeComparison::Judge(MakeCompare(ERenderProbeTolerance::Zero), CameraName, Captures, DeltaPath());

		ASSERT_THAT(IsTrue(Outcome.Result.Similar.IsSet() && Outcome.Result.Similar.GetValue()));
		ASSERT_THAT(IsTrue(Outcome.Result.GlobalDifference.IsSet()));
		ASSERT_THAT(IsNear(0.0, Outcome.Result.GlobalDifference.GetValue(), DifferenceTolerance));
		ASSERT_THAT(IsFalse(Outcome.Result.DiffBounds.IsSet()));
		ASSERT_THAT(IsTrue(Outcome.Result.DeltaImage.IsEmpty()));
		ASSERT_THAT(IsFalse(IFileManager::Get().FileExists(*DeltaPath())));
		ASSERT_THAT(IsTrue(Outcome.SubjectDiagnostics.IsEmpty()));
		ASSERT_THAT(IsTrue(Outcome.JobDiagnostics.IsEmpty()));
	}

	TEST_METHOD(DifferingBlockGivesItsBoundsAndLargestDifference)
	{
		const FRenderProbeComparisonCaptures Captures = DeterministicCaptures(MakeBlackImage(), MakeImageWithBlock(BlockGray));

		const FRenderProbeComparisonOutcome Outcome = RenderProbeComparison::Judge(MakeCompare(ERenderProbeTolerance::Zero), CameraName, Captures, DeltaPath());

		ASSERT_THAT(IsTrue(Outcome.Result.Similar.IsSet() && !Outcome.Result.Similar.GetValue()));
		ASSERT_THAT(IsNear(static_cast<double>(BlockPixelCount) / ImagePixelCount, Outcome.Result.GlobalDifference.Get(-1.0), DifferenceTolerance));
		ASSERT_THAT(IsTrue(Outcome.Result.DiffBounds.IsSet()));
		ASSERT_THAT(IsTrue(Outcome.Result.DiffBounds.GetValue() == FIntRect(BlockMinX, BlockMinY, BlockMaxX, BlockMaxY)));
		ASSERT_THAT(IsNear(BlockGray / static_cast<double>(MAX_uint8), Outcome.Result.MaxLocalDifference.Get(-1.0), DifferenceTolerance));
		ASSERT_THAT(AreEqual(DeltaPath(), Outcome.Result.DeltaImage));
		ASSERT_THAT(IsTrue(IFileManager::Get().FileExists(*DeltaPath())));
		ASSERT_THAT(IsTrue(Outcome.JobDiagnostics.IsEmpty()));
	}

	TEST_METHOD(ToleranceDecidesWhetherASlightDifferenceCounts)
	{
		const FRenderProbeComparisonCaptures Captures = DeterministicCaptures(MakeBlackImage(), MakeImageWithBlock(SlightGray));

		const FRenderProbeComparisonOutcome Strict = RenderProbeComparison::Judge(MakeCompare(ERenderProbeTolerance::Zero), CameraName, Captures, DeltaPath());
		const FRenderProbeComparisonOutcome Loose = RenderProbeComparison::Judge(MakeCompare(ERenderProbeTolerance::Low), CameraName, Captures, DeltaPath());

		ASSERT_THAT(IsTrue(Strict.Result.Similar.IsSet() && !Strict.Result.Similar.GetValue()));
		ASSERT_THAT(IsTrue(Strict.Result.DiffBounds.IsSet()));
		ASSERT_THAT(IsTrue(Loose.Result.Similar.IsSet() && Loose.Result.Similar.GetValue()));
		ASSERT_THAT(IsFalse(Loose.Result.DiffBounds.IsSet()));
	}

	TEST_METHOD(UnrepeatableCaptureWithholdsVerdictOnlyUnderStrictTolerance)
	{
		FRenderProbeComparisonCaptures Captures = DeterministicCaptures(MakeBlackImage(), MakeBlackImage());
		Captures.ARepeat = SaveImage(SlightRepeatAFile, MakeImageWithBlock(SlightGray));

		const FRenderProbeComparisonOutcome Strict = RenderProbeComparison::Judge(MakeCompare(ERenderProbeTolerance::Zero), CameraName, Captures, DeltaPath());
		const FRenderProbeComparisonOutcome Loose = RenderProbeComparison::Judge(MakeCompare(ERenderProbeTolerance::Low), CameraName, Captures, DeltaPath());

		ASSERT_THAT(IsFalse(Strict.Result.Similar.IsSet()));
		ASSERT_THAT(IsFalse(Strict.Result.GlobalDifference.IsSet()));
		ASSERT_THAT(AreEqual(1, Strict.SubjectDiagnostics.Num()));
		const FRenderProbeSubjectDiagnostic& Reported = Strict.SubjectDiagnostics[0];
		ASSERT_THAT(AreEqual(FString(SubjectA), Reported.Subject));
		ASSERT_THAT(IsTrue(Reported.Diagnostic.Kind == ERenderProbeDiagnosticKind::Nondeterministic));
		ASSERT_THAT(IsTrue(Reported.Diagnostic.Detail.Contains(CompareName, ESearchCase::CaseSensitive)));
		ASSERT_THAT(IsTrue(Reported.Diagnostic.Detail.Contains(CameraName, ESearchCase::CaseSensitive)));
		ASSERT_THAT(IsTrue(Loose.Result.Similar.IsSet() && Loose.Result.Similar.GetValue()));
		ASSERT_THAT(IsTrue(Loose.SubjectDiagnostics.IsEmpty()));
	}

	TEST_METHOD(MissingCaptureWithholdsVerdictWithoutDiagnostic)
	{
		FRenderProbeComparisonCaptures Captures = DeterministicCaptures(MakeBlackImage(), MakeBlackImage());
		Captures.BRepeat.Reset();

		const FRenderProbeComparisonOutcome Outcome = RenderProbeComparison::Judge(MakeCompare(ERenderProbeTolerance::Low), CameraName, Captures, DeltaPath());

		ASSERT_THAT(IsFalse(Outcome.Result.Similar.IsSet()));
		ASSERT_THAT(AreEqual(FString(CompareName), Outcome.Result.Name));
		ASSERT_THAT(AreEqual(FString(CameraName), Outcome.Result.Camera));
		ASSERT_THAT(IsTrue(Outcome.SubjectDiagnostics.IsEmpty()));
		ASSERT_THAT(IsTrue(Outcome.JobDiagnostics.IsEmpty()));
	}

	TEST_METHOD(UnwritableDifferenceImageReportsOutputFailed)
	{
		const FRenderProbeComparisonCaptures Captures = DeterministicCaptures(MakeBlackImage(), MakeImageWithBlock(BlockGray));
		ASSERT_THAT(IsTrue(IFileManager::Get().MakeDirectory(*DeltaPath(), true)));

		const FRenderProbeComparisonOutcome Outcome = RenderProbeComparison::Judge(MakeCompare(ERenderProbeTolerance::Zero), CameraName, Captures, DeltaPath());

		ASSERT_THAT(AreEqual(1, Outcome.JobDiagnostics.Num()));
		const FRenderProbeDiagnostic& Reported = Outcome.JobDiagnostics[0];
		ASSERT_THAT(IsTrue(Reported.Kind == ERenderProbeDiagnosticKind::OutputFailed));
		ASSERT_THAT(IsTrue(Reported.Detail.Contains(WriteDeltaStep, ESearchCase::CaseSensitive)));
		ASSERT_THAT(IsTrue(Reported.Detail.Contains(DeltaPath(), ESearchCase::CaseSensitive)));
		ASSERT_THAT(IsTrue(Outcome.Result.DeltaImage.IsEmpty()));
	}
};

}

#endif
