// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Scene.h"
#include "Math/IntPoint.h"
#include "Math/Transform.h"
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

/** Profile assets resolved by the caller. A null asset leaves that rig part out. */
struct FRenderProbeStageAssets
{
	UTextureCube* SkyCubemap = nullptr;
	UStaticMesh* FloorMesh = nullptr;
	UMaterialInterface* FloorMaterial = nullptr;
	TArray<FRenderProbeStageLevel> Levels;
};

/** Preview world built from a scene profile. It is never ticked, so world time and material time stay at zero. */
class FRenderProbeStage
{
public:
	FRenderProbeStage(const FRenderProbeProfileSettings& Settings, const FRenderProbeStageAssets& Assets, FIntPoint Resolution);
	~FRenderProbeStage();

	FRenderProbeStage(const FRenderProbeStage&) = delete;
	FRenderProbeStage& operator=(const FRenderProbeStage&) = delete;
	FRenderProbeStage(FRenderProbeStage&&) = delete;
	FRenderProbeStage& operator=(FRenderProbeStage&&) = delete;

	/** Streams the levels in and finishes loading them; returns the paths of levels that could not be streamed. */
	TArray<FString> StreamLevels(const TArray<FRenderProbeStageLevel>& Levels);

	/** Places a static or skeletal mesh at Transform relative to the profile's subject placement. */
	UMeshComponent* PlaceSubject(UObject& Mesh, const TArray<UMaterialInterface*>& SlotMaterials, const FRenderProbeTransform& Transform);

	void RemoveSubject(UMeshComponent& Subject);

	/** Captures Subject from Camera and writes the PNG; false when reading back or writing the image failed. */
	bool Capture(const UMeshComponent& Subject, const FRenderProbeCamera& Camera, const FString& FilePath);

private:
	void AddRig(const FRenderProbeRigSettings& Rig, const FRenderProbeStageAssets& Assets);

	FTransform MakeCameraTransform(const UMeshComponent& Subject, const FRenderProbeCamera& Camera) const;

	TUniquePtr<FPreviewScene> Scene;
	TStrongObjectPtr<UTextureRenderTarget2D> RenderTarget;
	TArray<ULevelStreamingDynamic*> StreamedLevels;
	FPostProcessSettings PostProcess;
	FRenderProbeLumenSettings Lumen;
	FTransform SubjectTransform;
	FIntPoint Resolution;
};
