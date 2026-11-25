// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/IntPoint.h"
#include "Math/Transform.h"
#include "RenderProbeCaptureSetup.h"
#include "RenderProbeProfileSettings.h"
#include "RenderProbeTransform.h"
#include "Templates/UniquePtr.h"
#include "UObject/StrongObjectPtr.h"

class FPreviewScene;
class ULevelStreamingDynamic;
class UMaterialInterface;
class UMeshComponent;
class UStaticMesh;
class UTextureCube;
class UTextureRenderTarget2D;
class UWorld;
struct FRenderProbeCamera;

struct FRenderProbeStageLevel
{
	UWorld* Level = nullptr;
	FRenderProbePlacement Placement;
};

struct FRenderProbeStageAssets
{
	UTextureCube* SkyCubemap = nullptr;
	UStaticMesh* FloorMesh = nullptr;
	UMaterialInterface* FloorMaterial = nullptr;
	TArray<FRenderProbeStageLevel> Levels;
};

class FRenderProbeStage
{
public:
	FRenderProbeStage(const FRenderProbeProfileSettings& Settings, const FRenderProbeStageAssets& Assets, FIntPoint Resolution);
	~FRenderProbeStage();

	FRenderProbeStage(const FRenderProbeStage&) = delete;
	FRenderProbeStage& operator=(const FRenderProbeStage&) = delete;
	FRenderProbeStage(FRenderProbeStage&&) = delete;
	FRenderProbeStage& operator=(FRenderProbeStage&&) = delete;

	TArray<FString> StreamLevels(const TArray<FRenderProbeStageLevel>& Levels);

	UMeshComponent* PlaceSubject(UObject& Mesh, const TArray<UMaterialInterface*>& SlotMaterials, const FRenderProbeTransform& Transform);

	void RemoveSubject(UMeshComponent& Subject);

	bool Capture(const UMeshComponent& Subject, const FRenderProbeCamera& Camera, const FString& FilePath, FString& OutFailedStep);

private:
	void AddRig(const FRenderProbeRigSettings& Rig, const FRenderProbeStageAssets& Assets);

	FTransform MakeCameraTransform(const UMeshComponent& Subject, const FRenderProbeCamera& Camera) const;

	TUniquePtr<FPreviewScene> Scene;
	TStrongObjectPtr<UTextureRenderTarget2D> RenderTarget;
	TArray<ULevelStreamingDynamic*> StreamedLevels;
	FRenderProbeCaptureSetup CaptureSetup;
	FTransform SubjectTransform;
	FIntPoint Resolution;
};
