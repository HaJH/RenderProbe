// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderProbeDiagnostic.h"

struct FRenderProbeJob;

namespace RenderProbeJobValidation
{
	/** Returns one InvalidJob diagnostic per rule the job breaks; empty when the job may run. */
	TArray<FRenderProbeDiagnostic> Validate(const FRenderProbeJob& Job);
}
