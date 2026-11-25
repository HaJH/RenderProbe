// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Scene.h"

struct FRenderProbeProfileSettings;

struct FRenderProbeCaptureSetup
{
	bool bUsesLumen = false;
	int32 CaptureCount = 1;
	FPostProcessSettings PostProcess;
};

namespace RenderProbeCaptureSetup
{
	FRenderProbeCaptureSetup Make(const FRenderProbeProfileSettings& Settings);
}
