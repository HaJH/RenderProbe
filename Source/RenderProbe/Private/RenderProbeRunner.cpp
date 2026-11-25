// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeRunner.h"

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Engine/StaticMesh.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "RenderProbeDiagnostic.h"
#include "RenderProbeJob.h"
#include "RenderProbeJobValidation.h"
#include "RenderProbeJson.h"
#include "RenderProbeLog.h"
#include "RenderProbeMaterialStats.h"
#include "RenderProbeOutputPaths.h"
#include "RenderProbeProfile.h"
#include "UObject/SoftObjectPath.h"

namespace RenderProbeRunnerLocal
{
	static constexpr TCHAR ObjectNameDelimiter = TEXT('.');

	/** Accepts both "/Root/Path/Asset.Asset" and the package-only form "/Root/Path/Asset". */
	static FString ToObjectPath(const FString& Path)
	{
		int32 DelimiterIndex = INDEX_NONE;
		if (Path.FindChar(ObjectNameDelimiter, DelimiterIndex))
		{
			return Path;
		}
		FString ObjectPath = Path;
		ObjectPath.AppendChar(ObjectNameDelimiter);
		ObjectPath += FPackageName::GetShortName(Path);
		return ObjectPath;
	}

	static bool IsSlotIndexKey(const FString& Key)
	{
		return !Key.IsEmpty() && Algo::AllOf(Key, [](const TCHAR Character) { return FChar::IsDigit(Character); });
	}

	static int32 FindSlotIndex(const FString& Key, const TArray<FName>& SlotNames)
	{
		if (IsSlotIndexKey(Key))
		{
			int32 SlotIndex = INDEX_NONE;
			LexFromString(SlotIndex, *Key);
			return SlotNames.IsValidIndex(SlotIndex) ? SlotIndex : INDEX_NONE;
		}
		return SlotNames.IndexOfByKey(FName(*Key));
	}

	static bool HasAnyDiagnostic(const FRenderProbeResult& Result)
	{
		return !Result.Diagnostics.IsEmpty()
			|| Algo::AnyOf(Result.Items, [](const FRenderProbeItemResult& Item) { return !Item.Diagnostics.IsEmpty(); });
	}

	static bool AreAllComparisonsSimilar(const FRenderProbeResult& Result)
	{
		return Algo::AllOf(Result.Compare, [](const FRenderProbeCompareResult& Compare)
		{
			return Compare.Similar.IsSet() && Compare.Similar.GetValue();
		});
	}
}

FRenderProbeResult URenderProbeRunner::Run(const FRenderProbeJob& Job)
{
	FRenderProbeResult Result;
	Result.Diagnostics = RenderProbeJobValidation::Validate(Job);
	if (!Result.Diagnostics.IsEmpty())
	{
		return Finish(Job.OutputDir, MoveTemp(Result));
	}

	ResolveProfileSettings(Job, Result);

	TArray<FItemPlan> Plans;
	for (const FRenderProbeItem& Item : Job.Items)
	{
		FRenderProbeItemResult& ItemResult = Result.Items.AddDefaulted_GetRef();
		ItemResult.Name = Item.Name;
		Plans.Add(PlanItem(Item, ItemResult));
	}

	TArray<FMaterialResource*> Resources;
	for (const FItemPlan& Plan : Plans)
	{
		for (const FSlotMaterial& Slot : Plan.Slots)
		{
			if (FMaterialResource* Resource = RenderProbeMaterialStats::FindResource(Slot.Material))
			{
				Resources.AddUnique(Resource);
			}
		}
	}
	const TSet<const FMaterialResource*> StillCompiling = RenderProbeMaterialStats::WaitForCompilation(Resources, Job.WaitTimeoutSeconds);

	for (int32 PlanIndex = 0; PlanIndex < Plans.Num(); ++PlanIndex)
	{
		const FItemPlan& Plan = Plans[PlanIndex];
		if (!Plan.Item->bStats)
		{
			continue;
		}
		for (const FSlotMaterial& Slot : Plan.Slots)
		{
			RenderProbeMaterialStats::Measure(Slot.Material, Slot.Key, StillCompiling, Result.Items[PlanIndex]);
		}
	}

	return Finish(Job.OutputDir, MoveTemp(Result));
}

FRenderProbeResult URenderProbeRunner::RunJson(const FString& JobJson)
{
	FRenderProbeJob Job;
	FString Error;
	if (!RenderProbeJson::JobFromJsonString(JobJson, Job, Error))
	{
		return InvalidJobResult(MoveTemp(Error));
	}
	return Run(Job);
}

FRenderProbeResult URenderProbeRunner::RunJobFile(const FString& JobFilePath)
{
	FString JobJson;
	if (!FFileHelper::LoadFileToString(JobJson, *JobFilePath))
	{
		return InvalidJobResult(FString::Printf(TEXT("cannot read job file '%s'"), *JobFilePath));
	}
	return RunJson(JobJson);
}

UObject* URenderProbeRunner::LoadAsset(const FString& Path)
{
	const FSoftObjectPath SoftPath(RenderProbeRunnerLocal::ToObjectPath(Path));
	if (SoftPath.IsNull())
	{
		return nullptr;
	}

	UObject* Asset = SoftPath.ResolveObject();
	if (!Asset && FPackageName::DoesPackageExist(SoftPath.GetLongPackageName()))
	{
		Asset = SoftPath.TryLoad();
	}
	if (Asset)
	{
		LoadedAssets.Add(Asset);
	}
	return Asset;
}

const FRenderProbeProfileSettings* URenderProbeRunner::ResolveProfileSettings(const FRenderProbeJob& Job, FRenderProbeResult& OutResult)
{
	if (Job.Profile.Settings.IsSet())
	{
		return &Job.Profile.Settings.GetValue();
	}

	const URenderProbeProfile* Profile = Cast<URenderProbeProfile>(LoadAsset(Job.Profile.Asset));
	if (!Profile)
	{
		OutResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::MissingAsset, FString(),
			FString::Printf(TEXT("profile asset '%s' does not exist or is not a RenderProbeProfile"), *Job.Profile.Asset)));
		return nullptr;
	}
	return &Profile->Settings;
}

URenderProbeRunner::FItemPlan URenderProbeRunner::PlanItem(const FRenderProbeItem& Item, FRenderProbeItemResult& OutItemResult)
{
	FItemPlan Plan;
	Plan.Item = &Item;

	if (Item.Mesh.IsEmpty())
	{
		PlanLabeledMaterials(Item, Plan, OutItemResult);
		return Plan;
	}

	const UObject* Mesh = LoadAsset(Item.Mesh);
	if (!Mesh)
	{
		OutItemResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::MissingAsset, FString(),
			FString::Printf(TEXT("mesh '%s' does not exist"), *Item.Mesh)));
		return Plan;
	}
	PlanMeshSlots(Item, *Mesh, Plan, OutItemResult);
	return Plan;
}

void URenderProbeRunner::PlanMeshSlots(const FRenderProbeItem& Item, const UObject& Mesh, FItemPlan& OutPlan, FRenderProbeItemResult& OutItemResult)
{
	TArray<FName> SlotNames;
	TArray<UMaterialInterface*> SlotMaterials;
	if (const UStaticMesh* StaticMesh = Cast<const UStaticMesh>(&Mesh))
	{
		for (const FStaticMaterial& StaticMaterial : StaticMesh->GetStaticMaterials())
		{
			SlotNames.Add(StaticMaterial.MaterialSlotName);
			SlotMaterials.Add(StaticMaterial.MaterialInterface.Get());
		}
	}
	else if (const USkeletalMesh* SkeletalMesh = Cast<const USkeletalMesh>(&Mesh))
	{
		for (const FSkeletalMaterial& SkeletalMaterial : SkeletalMesh->GetMaterials())
		{
			SlotNames.Add(SkeletalMaterial.MaterialSlotName);
			SlotMaterials.Add(SkeletalMaterial.MaterialInterface.Get());
		}
	}
	else
	{
		OutItemResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::MissingAsset, FString(),
			FString::Printf(TEXT("'%s' is not a static or skeletal mesh"), *Item.Mesh)));
		return;
	}

	TArray<bool> SlotUsable;
	SlotUsable.Init(true, SlotMaterials.Num());
	for (const TPair<FString, FString>& Override : Item.Materials)
	{
		const int32 SlotIndex = RenderProbeRunnerLocal::FindSlotIndex(Override.Key, SlotNames);
		if (SlotIndex == INDEX_NONE)
		{
			OutItemResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::InvalidJob, Override.Key,
				FString::Printf(TEXT("material key '%s' matches no slot index or slot name of mesh '%s'"), *Override.Key, *Item.Mesh)));
			continue;
		}

		UMaterialInterface* Material = LoadMaterial(Override.Key, Override.Value, OutItemResult);
		SlotMaterials[SlotIndex] = Material;
		SlotUsable[SlotIndex] = Material != nullptr;
	}

	for (int32 SlotIndex = 0; SlotIndex < SlotMaterials.Num(); ++SlotIndex)
	{
		if (SlotUsable[SlotIndex])
		{
			OutPlan.Slots.Add({ LexToString(SlotIndex), SlotMaterials[SlotIndex] });
		}
	}
}

void URenderProbeRunner::PlanLabeledMaterials(const FRenderProbeItem& Item, FItemPlan& OutPlan, FRenderProbeItemResult& OutItemResult)
{
	for (const TPair<FString, FString>& Labeled : Item.Materials)
	{
		if (UMaterialInterface* Material = LoadMaterial(Labeled.Key, Labeled.Value, OutItemResult))
		{
			OutPlan.Slots.Add({ Labeled.Key, Material });
		}
	}
}

UMaterialInterface* URenderProbeRunner::LoadMaterial(const FString& SlotKey, const FString& Path, FRenderProbeItemResult& OutItemResult)
{
	UObject* Asset = LoadAsset(Path);
	UMaterialInterface* Material = Cast<UMaterialInterface>(Asset);
	if (!Material)
	{
		const FString Detail = Asset
			? FString::Printf(TEXT("'%s' is not a material"), *Path)
			: FString::Printf(TEXT("material '%s' does not exist"), *Path);
		OutItemResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::MissingAsset, SlotKey, Detail));
	}
	return Material;
}

FRenderProbeResult URenderProbeRunner::InvalidJobResult(FString Detail)
{
	FRenderProbeResult Result;
	Result.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::InvalidJob, FString(), MoveTemp(Detail)));
	return Finish(FString(), MoveTemp(Result));
}

FRenderProbeResult URenderProbeRunner::Finish(const FString& OutputDir, FRenderProbeResult Result)
{
	Result.bOk = !RenderProbeRunnerLocal::HasAnyDiagnostic(Result) && RenderProbeRunnerLocal::AreAllComparisonsSimilar(Result);

	if (!OutputDir.IsEmpty())
	{
		const FString ResultPath = RenderProbeOutputPaths::ResultPath(OutputDir);
		IFileManager::Get().MakeDirectory(*OutputDir, true);
		if (!FFileHelper::SaveStringToFile(RenderProbeJson::ResultToJsonString(Result), *ResultPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
		{
			UE_LOG(LogRenderProbe, Error, TEXT("Failed to write %s"), *ResultPath);
		}
	}
	return Result;
}
