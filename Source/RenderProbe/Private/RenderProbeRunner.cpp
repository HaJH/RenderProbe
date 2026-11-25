// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeRunner.h"

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "Components/MeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkinnedAssetCommon.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "MaterialDomain.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Optional.h"
#include "Misc/PackageName.h"
#include "RenderProbeDiagnostic.h"
#include "RenderProbeJob.h"
#include "RenderProbeJobValidation.h"
#include "RenderProbeJson.h"
#include "RenderProbeMaterialStats.h"
#include "RenderProbeOutputPaths.h"
#include "RenderProbeProfile.h"
#include "RenderProbeStage.h"
#include "UObject/SoftObjectPath.h"

namespace RenderProbeRunnerLocal
{
	static constexpr TCHAR ObjectNameDelimiter = TEXT('.');
	static constexpr const TCHAR* WriteResultStep = TEXT("writing result.json");

	static FRenderProbeDiagnostic MakeOutputFailed(const FString& Path, const FString& FailedStep)
	{
		return FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::OutputFailed, FString(),
			FString::Printf(TEXT("%s failed for '%s'"), *FailedStep, *Path));
	}

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

	static void AddResource(UMaterialInterface* Material, TArray<FMaterialResource*>& OutResources)
	{
		if (FMaterialResource* Resource = RenderProbeMaterialStats::FindResource(Material))
		{
			OutResources.AddUnique(Resource);
		}
	}

	static TOptional<FString> FindDefaultMaterialReason(UMaterialInterface* Material, const bool bSkeletal)
	{
		if (!Material)
		{
			return FString(TEXT("slot has no material"));
		}
		if (Material == UMaterial::GetDefaultMaterial(MD_Surface))
		{
			return FString::Printf(TEXT("slot uses the engine default material %s"), *Material->GetPathName());
		}

		const FMaterialResource* Resource = RenderProbeMaterialStats::FindResource(Material);
		if (!Resource)
		{
			return FString::Printf(TEXT("%s has no material resource for the editor shader platform"), *Material->GetPathName());
		}
		if (!Resource->GetCompileErrors().IsEmpty())
		{
			return FString::Printf(TEXT("%s failed to compile"), *Material->GetPathName());
		}
		if (!Resource->GetGameThreadShaderMap())
		{
			return FString::Printf(TEXT("%s has no shader map"), *Material->GetPathName());
		}
		if (bSkeletal && !Material->CheckMaterialUsage_Concurrent(MATUSAGE_SkeletalMesh))
		{
			return FString::Printf(TEXT("%s cannot be used with skeletal meshes"), *Material->GetPathName());
		}
		return {};
	}

	static void ReportDefaultMaterials(const UMeshComponent& Subject, const bool bSkeletal, FRenderProbeItemResult& OutItemResult)
	{
		for (int32 SlotIndex = 0; SlotIndex < Subject.GetNumMaterials(); ++SlotIndex)
		{
			if (TOptional<FString> Reason = FindDefaultMaterialReason(Subject.GetMaterial(SlotIndex), bSkeletal))
			{
				OutItemResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::DefaultMaterial, LexToString(SlotIndex), Reason.GetValue()));
			}
		}
	}

	static bool ReportStillCompiling(const TArray<UMaterialInterface*>& SlotMaterials, const TSet<const FMaterialResource*>& StillCompiling, FRenderProbeItemResult& OutItemResult)
	{
		bool bAnyStillCompiling = false;
		for (int32 SlotIndex = 0; SlotIndex < SlotMaterials.Num(); ++SlotIndex)
		{
			const FMaterialResource* Resource = RenderProbeMaterialStats::FindResource(SlotMaterials[SlotIndex]);
			if (Resource && StillCompiling.Contains(Resource))
			{
				bAnyStillCompiling = true;
				OutItemResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::Timeout, LexToString(SlotIndex),
					FString::Printf(TEXT("%s was still compiling when the wait limit passed; the subject was not captured"), *SlotMaterials[SlotIndex]->GetPathName())));
			}
		}
		return bAnyStillCompiling;
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

	const FRenderProbeProfileSettings* Settings = ResolveProfileSettings(Job, Result);
	const FRenderProbeStageAssets StageAssets = Settings ? ResolveStageAssets(*Settings, Result) : FRenderProbeStageAssets();

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
			RenderProbeRunnerLocal::AddResource(Slot.Material, Resources);
		}
		for (UMaterialInterface* Material : Plan.RenderMaterials)
		{
			RenderProbeRunnerLocal::AddResource(Material, Resources);
		}
	}
	const TSet<const FMaterialResource*> StillCompiling = RenderProbeMaterialStats::WaitForCompilation(Resources, Job.WaitTimeoutSeconds);

	if (Settings)
	{
		CaptureItems(Job, *Settings, StageAssets, Plans, StillCompiling, Result);
	}

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

template <typename AssetType>
AssetType* URenderProbeRunner::LoadProfileAsset(const FSoftObjectPath& Path, const TCHAR* What, FRenderProbeResult& OutResult)
{
	AssetType* Asset = Path.IsNull() ? nullptr : Cast<AssetType>(LoadAsset(Path.ToString()));
	if (!Asset)
	{
		OutResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::MissingAsset, FString(),
			FString::Printf(TEXT("%s '%s' does not exist or is not a %s"), What, *Path.ToString(), *AssetType::StaticClass()->GetName())));
	}
	return Asset;
}

FRenderProbeStageAssets URenderProbeRunner::ResolveStageAssets(const FRenderProbeProfileSettings& Settings, FRenderProbeResult& OutResult)
{
	FRenderProbeStageAssets Assets;
	const FRenderProbeRigSettings& Rig = Settings.Rig;
	if (Rig.bEnabled && Rig.SkyLight.bEnabled && !Rig.SkyLight.Cubemap.IsNull())
	{
		Assets.SkyCubemap = LoadProfileAsset<UTextureCube>(Rig.SkyLight.Cubemap.ToSoftObjectPath(), TEXT("sky light cubemap"), OutResult);
	}
	if (Rig.bEnabled && Rig.Floor.bEnabled)
	{
		Assets.FloorMesh = LoadProfileAsset<UStaticMesh>(Rig.Floor.Mesh.ToSoftObjectPath(), TEXT("floor mesh"), OutResult);
		if (!Rig.Floor.Material.IsNull())
		{
			Assets.FloorMaterial = LoadProfileAsset<UMaterialInterface>(Rig.Floor.Material.ToSoftObjectPath(), TEXT("floor material"), OutResult);
		}
	}

	for (const FRenderProbeEnvironmentLevel& Level : Settings.EnvironmentLevels)
	{
		UWorld* LevelWorld = LoadProfileAsset<UWorld>(Level.Level.ToSoftObjectPath(), TEXT("environment level"), OutResult);
		if (!LevelWorld)
		{
			continue;
		}
		if (LevelWorld->IsPartitionedWorld())
		{
			OutResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::UnsupportedLevel, FString(),
				FString::Printf(TEXT("environment level '%s' is a world partition map"), *LevelWorld->GetPathName())));
			continue;
		}
		Assets.Levels.Add({ LevelWorld, Level.Placement });
	}
	return Assets;
}

void URenderProbeRunner::CaptureItems(const FRenderProbeJob& Job, const FRenderProbeProfileSettings& Settings, const FRenderProbeStageAssets& Assets,
	const TArray<FItemPlan>& Plans, const TSet<const FMaterialResource*>& StillCompiling, FRenderProbeResult& OutResult)
{
	const bool bAnyMesh = Algo::AnyOf(Plans, [](const FItemPlan& Plan) { return Plan.Mesh != nullptr; });
	if (!bAnyMesh || Job.Cameras.IsEmpty())
	{
		return;
	}

	FRenderProbeStage Stage(Settings, Assets, Job.Resolution);
	for (const FString& FailedLevel : Stage.StreamLevels(Assets.Levels))
	{
		OutResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::MissingAsset, FString(),
			FString::Printf(TEXT("environment level '%s' could not be streamed"), *FailedLevel)));
	}

	for (int32 PlanIndex = 0; PlanIndex < Plans.Num(); ++PlanIndex)
	{
		const FItemPlan& Plan = Plans[PlanIndex];
		if (!Plan.Mesh)
		{
			continue;
		}
		FRenderProbeItemResult& ItemResult = OutResult.Items[PlanIndex];
		if (RenderProbeRunnerLocal::ReportStillCompiling(Plan.RenderMaterials, StillCompiling, ItemResult))
		{
			continue;
		}

		UMeshComponent* Subject = Stage.PlaceSubject(*Plan.Mesh, Plan.RenderMaterials, Plan.Item->Transform);
		RenderProbeRunnerLocal::ReportDefaultMaterials(*Subject, Plan.Mesh->IsA<USkeletalMesh>(), ItemResult);
		for (const FRenderProbeCamera& Camera : Job.Cameras)
		{
			const FString CapturePath = RenderProbeOutputPaths::CapturePath(Job.OutputDir, Plan.Item->Name, Camera.Name);
			FString FailedStep;
			if (Stage.Capture(*Subject, Camera, CapturePath, FailedStep))
			{
				ItemResult.Images.Add(Camera.Name, CapturePath);
			}
			else
			{
				ItemResult.Diagnostics.Add(RenderProbeRunnerLocal::MakeOutputFailed(CapturePath, FailedStep));
			}
		}
		Stage.RemoveSubject(*Subject);
	}
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

	UObject* Mesh = LoadAsset(Item.Mesh);
	if (!Mesh)
	{
		OutItemResult.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::MissingAsset, FString(),
			FString::Printf(TEXT("mesh '%s' does not exist"), *Item.Mesh)));
		return Plan;
	}
	PlanMeshSlots(Item, *Mesh, Plan, OutItemResult);
	return Plan;
}

void URenderProbeRunner::PlanMeshSlots(const FRenderProbeItem& Item, UObject& Mesh, FItemPlan& OutPlan, FRenderProbeItemResult& OutItemResult)
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

	TArray<UMaterialInterface*> RenderMaterials = SlotMaterials;
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
		if (Material)
		{
			RenderMaterials[SlotIndex] = Material;
		}
	}

	if (Mesh.IsA<USkeletalMesh>())
	{
		for (UMaterialInterface* Material : RenderMaterials)
		{
			if (Material)
			{
				Material->CheckMaterialUsage(MATUSAGE_SkeletalMesh);
			}
		}
	}
	OutPlan.Mesh = &Mesh;
	OutPlan.RenderMaterials = MoveTemp(RenderMaterials);

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
			Result.Diagnostics.Add(RenderProbeRunnerLocal::MakeOutputFailed(ResultPath, RenderProbeRunnerLocal::WriteResultStep));
			Result.bOk = false;
		}
	}
	return Result;
}
