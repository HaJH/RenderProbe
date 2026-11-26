// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FMaterialResource;

namespace RenderProbeCompilationWait
{
	/** Returns the resources still compiling when Deadline (in FPlatformTime::Seconds) passed. */
	TSet<const FMaterialResource*> WaitForMaterials(const TArray<FMaterialResource*>& Resources, double Deadline);

	/** Returns whether every asset compilation, shaders included, finished before Deadline (in FPlatformTime::Seconds). */
	bool WaitForAllAssets(double Deadline);
}
