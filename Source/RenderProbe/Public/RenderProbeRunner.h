// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderProbeResult.h"
#include "Templates/ValueOrError.h"
#include "UObject/Object.h"

#include "RenderProbeRunner.generated.h"

class FMaterialResource;
class UMaterialInterface;
struct FRenderProbeItem;
struct FRenderProbeJob;
struct FRenderProbeProfileSettings;
struct FRenderProbeStageAssets;
struct FSoftObjectPath;

UCLASS()
class RENDERPROBE_API URenderProbeRunner : public UObject
{
	GENERATED_BODY()

public:
	FRenderProbeResult Run(const FRenderProbeJob& Job);

	FRenderProbeResult RunJson(const FString& JobJson);

	FRenderProbeResult RunJobFile(const FString& JobFilePath);

	static TValueOrError<FRenderProbeJob, FRenderProbeResult> ReadJobJson(const FString& JobJson);

	static TValueOrError<FRenderProbeJob, FRenderProbeResult> ReadJobFile(const FString& JobFilePath);

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
		UObject* Mesh = nullptr;
		TArray<UMaterialInterface*> RenderMaterials;
	};

	UObject* LoadAsset(const FString& Path);

	const FRenderProbeProfileSettings* ResolveProfileSettings(const FRenderProbeJob& Job, FRenderProbeResult& OutResult);

	FRenderProbeStageAssets ResolveStageAssets(const FRenderProbeProfileSettings& Settings, FRenderProbeResult& OutResult);

	template <typename AssetType>
	AssetType* LoadProfileAsset(const FSoftObjectPath& Path, const TCHAR* What, FRenderProbeResult& OutResult);

	void CaptureItems(const FRenderProbeJob& Job, const FRenderProbeProfileSettings& Settings, const FRenderProbeStageAssets& Assets,
		const TArray<FItemPlan>& Plans, const TSet<const FMaterialResource*>& StillCompiling, TMap<FString, FString>& OutRepeatPathsByCapture, FRenderProbeResult& OutResult);

	FItemPlan PlanItem(const FRenderProbeItem& Item, FRenderProbeItemResult& OutItemResult);

	void PlanMeshSlots(const FRenderProbeItem& Item, UObject& Mesh, FItemPlan& OutPlan, FRenderProbeItemResult& OutItemResult);

	void PlanLabeledMaterials(const FRenderProbeItem& Item, FItemPlan& OutPlan, FRenderProbeItemResult& OutItemResult);

	UMaterialInterface* LoadMaterial(const FString& SlotKey, const FString& Path, FRenderProbeItemResult& OutItemResult);

	static FRenderProbeResult InvalidJobResult(FString Detail);

	static FRenderProbeResult Finish(const FString& OutputDir, FRenderProbeResult Result);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> LoadedAssets;
};
