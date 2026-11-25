// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderProbeDiagnostic.h"

struct FRenderProbeJob;

namespace RenderProbeJobValidation
{
	TArray<FRenderProbeDiagnostic> Validate(const FRenderProbeJob& Job);
}
