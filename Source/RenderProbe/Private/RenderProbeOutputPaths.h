// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace RenderProbeOutputPaths
{
	inline constexpr const TCHAR* NameSeparator = TEXT("__");

	FString CapturePath(const FString& OutputDir, const FString& ItemName, const FString& CameraName);

	FString DiffPath(const FString& OutputDir, const FString& CompareName, const FString& CameraName);

	FString ResultPath(const FString& OutputDir);
}
