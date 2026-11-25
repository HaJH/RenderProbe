// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeOutputPaths.h"

#include "Misc/Paths.h"

namespace RenderProbeOutputPaths
{
	static constexpr const TCHAR* ImageExtension = TEXT(".png");
	static constexpr const TCHAR* DiffDirectoryName = TEXT("diff");
	static constexpr const TCHAR* ResultFileName = TEXT("result.json");

	static FString ImageFileName(const FString& SubjectName, const FString& CameraName)
	{
		return SubjectName + NameSeparator + CameraName + ImageExtension;
	}

	FString CapturePath(const FString& OutputDir, const FString& ItemName, const FString& CameraName)
	{
		return FPaths::Combine(OutputDir, ImageFileName(ItemName, CameraName));
	}

	FString DiffPath(const FString& OutputDir, const FString& CompareName, const FString& CameraName)
	{
		return FPaths::Combine(OutputDir, DiffDirectoryName, ImageFileName(CompareName, CameraName));
	}

	FString ResultPath(const FString& OutputDir)
	{
		return FPaths::Combine(OutputDir, ResultFileName);
	}
}
