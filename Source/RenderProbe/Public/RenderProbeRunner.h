// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderProbeResult.h"
#include "UObject/Object.h"

#include "RenderProbeRunner.generated.h"

class UMaterialInterface;
struct FRenderProbeItem;
struct FRenderProbeJob;
struct FRenderProbeProfileSettings;

/** Runs one render job. Holds the job's loaded assets until it is discarded; create one per job. */
UCLASS()
class RENDERPROBE_API URenderProbeRunner : public UObject
{
	GENERATED_BODY()

public:
	FRenderProbeResult Run(const FRenderProbeJob& Job);

	FRenderProbeResult RunJson(const FString& JobJson);

	FRenderProbeResult RunJobFile(const FString& JobFilePath);

private:
	struct FSlotMaterial
	{
		FString Key;
		UMaterialInterface* Material = nullptr;
	};

	struct FItemPlan
	{
		const FRenderProbeItem* Item = nullptr;
		TArray<FSlotMaterial> Slots;
	};

	UObject* LoadAsset(const FString& Path);

	const FRenderProbeProfileSettings* ResolveProfileSettings(const FRenderProbeJob& Job, FRenderProbeResult& OutResult);

	FItemPlan PlanItem(const FRenderProbeItem& Item, FRenderProbeItemResult& OutItemResult);

	void PlanMeshSlots(const FRenderProbeItem& Item, const UObject& Mesh, FItemPlan& OutPlan, FRenderProbeItemResult& OutItemResult);

	void PlanLabeledMaterials(const FRenderProbeItem& Item, FItemPlan& OutPlan, FRenderProbeItemResult& OutItemResult);

	UMaterialInterface* LoadMaterial(const FString& SlotKey, const FString& Path, FRenderProbeItemResult& OutItemResult);

	static FRenderProbeResult InvalidJobResult(FString Detail);

	static FRenderProbeResult Finish(const FString& OutputDir, FRenderProbeResult Result);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedAssets;
};
