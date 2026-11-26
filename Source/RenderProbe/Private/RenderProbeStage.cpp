// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeStage.h"

#include "AssetCompilingManager.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "PreviewScene.h"
#include "RenderProbeJob.h"
#include "SceneInterface.h"
#include "UObject/Package.h"

namespace RenderProbeStageLocal
{
	/** Any id other than none gives the streamed level a name distinct from an already loaded copy of the same level. */
	static constexpr int32 LevelInstanceRenameId = 1;
	static constexpr double Half = 0.5;
	static constexpr uint8 OpaqueAlpha = 255;
	static constexpr const TCHAR* ReadRenderTargetStep = TEXT("reading the render target");
	static constexpr const TCHAR* WritePngStep = TEXT("writing the PNG");

	static FTransform ToTransform(const FRenderProbeTransform& Transform)
	{
		return FTransform(Transform.Rotation, Transform.Location, Transform.Scale);
	}

	static UMeshComponent* CreateMeshComponent(UObject& Mesh)
	{
		if (UStaticMesh* StaticMesh = Cast<UStaticMesh>(&Mesh))
		{
			UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(GetTransientPackage());
			Component->SetStaticMesh(StaticMesh);
			return Component;
		}
		USkeletalMeshComponent* Component = NewObject<USkeletalMeshComponent>(GetTransientPackage());
		Component->bComponentUseFixedSkelBounds = true;
		Component->SetSkeletalMeshAsset(CastChecked<USkeletalMesh>(&Mesh));
		return Component;
	}

	static void MakeOpaque(FImage& Image)
	{
		Image.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		for (FColor& Pixel : Image.AsBGRA8())
		{
			Pixel.A = OpaqueAlpha;
		}
	}
}

FRenderProbeStage::FRenderProbeStage(const FRenderProbeProfileSettings& Settings, const FRenderProbeStageAssets& Assets, const FIntPoint InResolution)
	: CaptureSetup(RenderProbeCaptureSetup::Make(Settings))
	, SubjectTransform(Settings.SubjectTransform.Rotation, Settings.SubjectTransform.Location)
	, Resolution(InResolution)
{
	Scene = MakeUnique<FPreviewScene>(FPreviewScene::ConstructionValues()
		.SetCreateDefaultLighting(false)
		.SetForceMipsResident(true)
		.SetTransactional(false)
		.AllowLumenPrimitiveTrackingInPreviewWorld(CaptureSetup.bUsesLumen));

	RenderTarget.Reset(NewObject<UTextureRenderTarget2D>(GetTransientPackage()));
	RenderTarget->RenderTargetFormat = RTF_RGBA8_SRGB;
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->InitAutoFormat(Resolution.X, Resolution.Y);
	RenderTarget->UpdateResourceImmediate(true);

	AddRig(Settings.Rig, Assets);
}

FRenderProbeStage::~FRenderProbeStage()
{
	for (ULevelStreamingDynamic* StreamedLevel : StreamedLevels)
	{
		StreamedLevel->SetIsRequestingUnloadAndRemoval(true);
	}
	if (!StreamedLevels.IsEmpty())
	{
		Scene->GetWorld()->FlushLevelStreaming(EFlushLevelStreamingType::Full);
	}
}

void FRenderProbeStage::AddRig(const FRenderProbeRigSettings& Rig, const FRenderProbeStageAssets& Assets)
{
	if (!Rig.bEnabled)
	{
		return;
	}

	if (Rig.DirectionalLight.bEnabled)
	{
		UDirectionalLightComponent* Light = NewObject<UDirectionalLightComponent>(GetTransientPackage());
		Light->SetMobility(EComponentMobility::Movable);
		Light->SetIntensity(Rig.DirectionalLight.Intensity);
		Light->SetLightColor(Rig.DirectionalLight.Color);
		Light->SetAtmosphereSunLight(Rig.DirectionalLight.bAtmosphereSunLight);
		Scene->AddComponent(Light, FTransform(Rig.DirectionalLight.Rotation));
	}

	if (Rig.SkyLight.bEnabled)
	{
		USkyLightComponent* SkyLight = NewObject<USkyLightComponent>(GetTransientPackage());
		SkyLight->SetMobility(EComponentMobility::Movable);
		SkyLight->SetIntensity(Rig.SkyLight.Intensity);
		if (Assets.SkyCubemap)
		{
			SkyLight->SourceType = SLS_SpecifiedCubemap;
			SkyLight->Cubemap = Assets.SkyCubemap;
		}
		else
		{
			SkyLight->SourceType = SLS_CapturedScene;
		}
		Scene->AddComponent(SkyLight, FTransform::Identity);
	}

	if (Rig.SkyAtmosphere.bEnabled)
	{
		Scene->AddComponent(NewObject<USkyAtmosphereComponent>(GetTransientPackage()), FTransform::Identity);
	}

	if (Rig.Floor.bEnabled && Assets.FloorMesh)
	{
		UStaticMeshComponent* Floor = NewObject<UStaticMeshComponent>(GetTransientPackage());
		Floor->SetStaticMesh(Assets.FloorMesh);
		if (Assets.FloorMaterial)
		{
			for (int32 SlotIndex = 0; SlotIndex < Floor->GetNumMaterials(); ++SlotIndex)
			{
				Floor->SetMaterial(SlotIndex, Assets.FloorMaterial);
			}
		}
		Scene->AddComponent(Floor, RenderProbeStageLocal::ToTransform(Rig.Floor.Transform));
	}
}

bool FRenderProbeStage::HasRenderScene() const
{
	const FSceneInterface* SceneInterface = Scene->GetWorld()->Scene;
	return SceneInterface && SceneInterface->GetRenderScene();
}

TArray<FString> FRenderProbeStage::StreamLevels(const TArray<FRenderProbeStageLevel>& Levels)
{
	UWorld* World = Scene->GetWorld();
	TArray<FString> FailedLevels;
	for (const FRenderProbeStageLevel& Level : Levels)
	{
		bool bSucceeded = false;
		ULevelStreamingDynamic* StreamedLevel = ULevelStreamingDynamic::LoadLevelInstance(
			World, Level.Level->GetPathName(), Level.Placement.Location, Level.Placement.Rotation, bSucceeded);
		if (!bSucceeded)
		{
			FailedLevels.Add(Level.Level->GetPathName());
			continue;
		}
		StreamedLevel->RenameForPIE(RenderProbeStageLocal::LevelInstanceRenameId);
		StreamedLevels.Add(StreamedLevel);
	}

	World->FlushLevelStreaming(EFlushLevelStreamingType::Full);
	FAssetCompilingManager::Get().FinishAllCompilation();
	Scene->UpdateCaptureContents();
	return FailedLevels;
}

UMeshComponent* FRenderProbeStage::PlaceSubject(UObject& Mesh, const TArray<UMaterialInterface*>& SlotMaterials, const FRenderProbeTransform& Transform)
{
	UMeshComponent* Component = RenderProbeStageLocal::CreateMeshComponent(Mesh);
	for (int32 SlotIndex = 0; SlotIndex < SlotMaterials.Num(); ++SlotIndex)
	{
		if (SlotMaterials[SlotIndex])
		{
			Component->SetMaterial(SlotIndex, SlotMaterials[SlotIndex]);
		}
	}
	Scene->AddComponent(Component, RenderProbeStageLocal::ToTransform(Transform) * SubjectTransform);
	return Component;
}

void FRenderProbeStage::RemoveSubject(UMeshComponent& Subject)
{
	Scene->RemoveComponent(&Subject);
}

bool FRenderProbeStage::Capture(const UMeshComponent& Subject, const FRenderProbeCamera& Camera, const FString& FilePath, FString& OutFailedStep)
{
	USceneCaptureComponent2D* CaptureComponent = NewObject<USceneCaptureComponent2D>(GetTransientPackage());
	CaptureComponent->TextureTarget = RenderTarget.Get();
	CaptureComponent->CaptureSource = SCS_FinalColorLDR;
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->bAlwaysPersistRenderingState = CaptureSetup.bUsesLumen;
	CaptureComponent->FOVAngle = Camera.Fov;
	CaptureComponent->PostProcessSettings = CaptureSetup.PostProcess;
	CaptureComponent->PostProcessBlendWeight = 1.f;
	Scene->AddComponent(CaptureComponent, MakeCameraTransform(Subject, Camera));
	CaptureComponent->ShowFlags.SetTemporalAA(false);
	CaptureComponent->ShowFlags.SetMotionBlur(false);

	for (int32 CaptureIndex = 0; CaptureIndex < CaptureSetup.CaptureCount; ++CaptureIndex)
	{
		CaptureComponent->CaptureScene();
	}

	FImage Image;
	const bool bRead = FImageUtils::GetRenderTargetImage(RenderTarget.Get(), Image);
	Scene->RemoveComponent(CaptureComponent);
	if (!bRead)
	{
		OutFailedStep = RenderProbeStageLocal::ReadRenderTargetStep;
		return false;
	}
	RenderProbeStageLocal::MakeOpaque(Image);
	if (!FImageUtils::SaveImageByExtension(*FilePath, Image))
	{
		OutFailedStep = RenderProbeStageLocal::WritePngStep;
		return false;
	}
	return true;
}

FTransform FRenderProbeStage::MakeCameraTransform(const UMeshComponent& Subject, const FRenderProbeCamera& Camera) const
{
	if (Camera.Mode == ERenderProbeCameraMode::Explicit)
	{
		return FTransform(Camera.Rotation, Camera.Location);
	}

	const double HalfHorizontalFov = FMath::DegreesToRadians(Camera.Fov * RenderProbeStageLocal::Half);
	const double HalfVerticalFov = FMath::Atan(FMath::Tan(HalfHorizontalFov) * Resolution.Y / Resolution.X);
	const double Distance = Subject.Bounds.SphereRadius / FMath::Sin(FMath::Min(HalfHorizontalFov, HalfVerticalFov));
	const FRotator Rotation(Camera.Pitch, Camera.Yaw, 0.f);
	return FTransform(Rotation, Subject.Bounds.Origin - Rotation.Vector() * Distance);
}
