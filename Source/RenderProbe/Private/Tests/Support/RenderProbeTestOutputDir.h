// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

#include "HAL/FileManager.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"

/** A unique output directory under the automation transient directory, deleted with its contents on destruction. */
class FRenderProbeTestOutputDir
{
public:
	FRenderProbeTestOutputDir()
		: Path(FPaths::Combine(FPaths::AutomationTransientDir(), TEXT("RenderProbe"), FGuid::NewGuid().ToString()))
	{
	}

	~FRenderProbeTestOutputDir()
	{
		IFileManager::Get().DeleteDirectory(*Path, false, true);
	}

	FRenderProbeTestOutputDir(const FRenderProbeTestOutputDir&) = delete;
	FRenderProbeTestOutputDir& operator=(const FRenderProbeTestOutputDir&) = delete;
	FRenderProbeTestOutputDir(FRenderProbeTestOutputDir&&) = delete;
	FRenderProbeTestOutputDir& operator=(FRenderProbeTestOutputDir&&) = delete;

	const FString& Get() const { return Path; }

private:
	FString Path;
};

#endif
