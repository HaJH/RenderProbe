// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Scene.h"
#include "Math/Color.h"
#include "Math/Rotator.h"
#include "RenderProbeTransform.h"
#include "UObject/SoftObjectPtr.h"

#include "RenderProbeProfileSettings.generated.h"

class UMaterialInterface;
class UStaticMesh;
class UTextureCube;
class UWorld;

USTRUCT(BlueprintType, meta = (ToolTip = "Directional light of the base rig."))
struct RENDERPROBE_API FRenderProbeDirectionalLightSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Whether the rig places a directional light."))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Light intensity in lux."))
	float Intensity = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Light color, as {r, g, b, a} in linear space."))
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Light rotation in degrees, as {pitch, yaw, roll}."))
	FRotator Rotation = FRotator(-45.f, -45.f, 0.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Whether the light acts as the atmosphere sun light (bAtmosphereSunLight), which materials read through SkyAtmosphereLightDirection."))
	bool bAtmosphereSunLight = false;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Sky light of the base rig."))
struct RENDERPROBE_API FRenderProbeSkyLightSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Whether the rig places a sky light."))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Sky light intensity scale."))
	float Intensity = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Cubemap asset path for the sky light. Empty means the sky light captures the scene in real time."))
	TSoftObjectPtr<UTextureCube> Cubemap;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Sky atmosphere of the base rig."))
struct RENDERPROBE_API FRenderProbeSkyAtmosphereSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Whether the rig places a SkyAtmosphere component."))
	bool bEnabled = false;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Floor of the base rig."))
struct RENDERPROBE_API FRenderProbeFloorSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Whether the rig places a floor mesh."))
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Static mesh asset path of the floor."))
	TSoftObjectPtr<UStaticMesh> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Material asset path applied to every slot of the floor mesh. Empty keeps the mesh's own materials."))
	TSoftObjectPtr<UMaterialInterface> Material;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Floor transform in the preview world."))
	FRenderProbeTransform Transform;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Base rig: lights, atmosphere and floor the plugin places in the preview world itself."))
struct RENDERPROBE_API FRenderProbeRigSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Whether the base rig is placed at all. When false, every rig part below is ignored."))
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Directional light settings."))
	FRenderProbeDirectionalLightSettings DirectionalLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Sky light settings."))
	FRenderProbeSkyLightSettings SkyLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Sky atmosphere settings."))
	FRenderProbeSkyAtmosphereSettings SkyAtmosphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Floor settings."))
	FRenderProbeFloorSettings Floor;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Lumen settings of the preview world."))
struct RENDERPROBE_API FRenderProbeLumenSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Whether Lumen is enabled in the preview world."))
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ClampMin = "0", ToolTip = "Number of captures taken and discarded before the kept capture, so that Lumen converges."))
	int32 ConvergenceCaptures = 0;
};

USTRUCT(BlueprintType, meta = (ToolTip = "A level streamed into the preview world as environment."))
struct RENDERPROBE_API FRenderProbeEnvironmentLevel
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Level asset path. World partition maps are not supported."))
	TSoftObjectPtr<UWorld> Level;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Where the level is placed in the preview world."))
	FRenderProbePlacement Placement;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Scene profile: base rig, post process, Lumen, environment levels and where the subject is placed."))
struct RENDERPROBE_API FRenderProbeProfileSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Base rig the plugin places in the preview world."))
	FRenderProbeRigSettings Rig;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Post process settings of the capture, as the engine's FPostProcessSettings. Exposure is fixed here."))
	FPostProcessSettings PostProcess;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Lumen settings."))
	FRenderProbeLumenSettings Lumen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Levels streamed into the preview world, each with its placement."))
	TArray<FRenderProbeEnvironmentLevel> EnvironmentLevels;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Base placement of the subjects, for example a spot inside an environment level."))
	FRenderProbePlacement SubjectTransform;
};
