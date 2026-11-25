// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FMaterialResource;
class UMaterialInterface;
struct FRenderProbeItemResult;

namespace RenderProbeMaterialStats
{
	FMaterialResource* FindResource(UMaterialInterface* Material);

	TSet<const FMaterialResource*> WaitForCompilation(const TArray<FMaterialResource*>& Resources, double TimeoutSeconds);

	void Measure(UMaterialInterface* Material, const FString& SlotKey, const TSet<const FMaterialResource*>& StillCompiling, FRenderProbeItemResult& OutItem);
}
