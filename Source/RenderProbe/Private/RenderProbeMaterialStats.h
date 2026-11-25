// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FMaterialResource;
class UMaterialInterface;
struct FRenderProbeItemResult;

namespace RenderProbeMaterialStats
{
	FMaterialResource* FindResource(UMaterialInterface* Material);

	/** Blocks until every resource finished compiling or TimeoutSeconds passed; returns the resources still compiling. */
	TSet<const FMaterialResource*> WaitForCompilation(const TArray<FMaterialResource*>& Resources, double TimeoutSeconds);

	/** Adds the slot's statistics to OutItem, or a diagnostic when the slot cannot be measured. */
	void Measure(UMaterialInterface* Material, const FString& SlotKey, const TSet<const FMaterialResource*>& StillCompiling, FRenderProbeItemResult& OutItem);
}
