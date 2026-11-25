// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/IntPoint.h"
#include "Math/Rotator.h"
#include "Math/Vector.h"
#include "Misc/Optional.h"
#include "RenderProbeProfileSettings.h"
#include "RenderProbeTransform.h"

#include "RenderProbeJob.generated.h"

UENUM(BlueprintType, meta = (ToolTip = "How a camera is positioned."))
enum class ERenderProbeCameraMode : uint8
{
	FitBounds UMETA(ToolTip = "The camera looks at the subject from yaw and pitch, at the distance where the subject's bounding sphere fills the view."),
	Explicit UMETA(ToolTip = "The camera uses location and rotation as given."),
};

UENUM(BlueprintType, meta = (ToolTip = "Tolerance preset of an image comparison."))
enum class ERenderProbeTolerance : uint8
{
	Zero UMETA(ToolTip = "Any pixel difference counts."),
	Low UMETA(ToolTip = "Small per-channel and brightness differences are ignored."),
	AntiAliasing UMETA(ToolTip = "Differences along anti-aliased edges are ignored."),
	Colors UMETA(ToolTip = "Color differences are ignored; only brightness counts."),
};

USTRUCT(BlueprintType, meta = (ToolTip = "Where the scene profile of a job comes from. Exactly one of asset and settings must be given."))
struct RENDERPROBE_API FRenderProbeProfileSource
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Asset path of a RenderProbeProfile data asset. Empty means not given."))
	FString Asset;

	UPROPERTY(EditAnywhere, Category = "RenderProbe", meta = (ToolTip = "Inline scene profile settings. Absent means not given."))
	TOptional<FRenderProbeProfileSettings> Settings;
};

USTRUCT(BlueprintType, meta = (ToolTip = "A camera every subject with a mesh is captured from."))
struct RENDERPROBE_API FRenderProbeCamera
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Camera name. Only A-Z, a-z, 0-9, '_' and '-'; must not contain '__'; unique among cameras. Used in output file names."))
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "How the camera is positioned."))
	ERenderProbeCameraMode Mode = ERenderProbeCameraMode::FitBounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "FitBounds mode: yaw in degrees around the subject."))
	float Yaw = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "FitBounds mode: pitch in degrees; negative looks down at the subject."))
	float Pitch = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Horizontal field of view in degrees."))
	float Fov = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Explicit mode: camera location, as {x, y, z}."))
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Explicit mode: camera rotation in degrees, as {pitch, yaw, roll}."))
	FRotator Rotation = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType, meta = (ToolTip = "A subject: a mesh with material overrides, or materials alone for statistics."))
struct RENDERPROBE_API FRenderProbeItem
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Subject name. Only A-Z, a-z, 0-9, '_' and '-'; must not contain '__'; unique among items. Captures are written to <outputDir>/<item>__<camera>.png."))
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Static or skeletal mesh asset path. Skeletal meshes are shown in reference pose. Empty means no mesh: nothing is rendered, stats must be true, and the item cannot be a compare target."))
	FString Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Material asset paths. With a mesh, the key is a slot index or slot name and the material overrides that slot. Without a mesh, the key is a label used in the result."))
	TMap<FString, FString> Materials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Mesh transform relative to the profile's subject placement."))
	FRenderProbeTransform Transform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Whether the result reports shader statistics for the material of every slot (or every label without a mesh)."))
	bool bStats = false;
};

USTRUCT(BlueprintType, meta = (ToolTip = "A comparison of two subjects' captures, per camera."))
struct RENDERPROBE_API FRenderProbeCompare
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Comparison name. Only A-Z, a-z, 0-9, '_' and '-'; must not contain '__'; unique among comparisons. Difference images are written to <outputDir>/diff/<compare>__<camera>.png."))
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Name of the first subject. Must be an item with a mesh."))
	FString A;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Name of the second subject. Must be an item with a mesh."))
	FString B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Tolerance preset used for this comparison and for its determinism check."))
	ERenderProbeTolerance Tolerance = ERenderProbeTolerance::Low;
};

USTRUCT(BlueprintType, meta = (ToolTip = "A render job: scene profile, cameras, subjects and comparisons."))
struct RENDERPROBE_API FRenderProbeJob
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Scene profile source."))
	FRenderProbeProfileSource Profile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Absolute output directory. Captures, difference images and result.json are written here."))
	FString OutputDir;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Capture resolution in pixels, as {x, y}."))
	FIntPoint Resolution = FIntPoint(1024, 1024);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ClampMin = "0", ToolTip = "Upper limit in seconds for waiting on shader compilation and streaming. Past it, the affected work is skipped with a timeout diagnostic."))
	double WaitTimeoutSeconds = 600.0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Cameras every subject with a mesh is captured from."))
	TArray<FRenderProbeCamera> Cameras;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Subjects."))
	TArray<FRenderProbeItem> Items;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Comparisons between subjects."))
	TArray<FRenderProbeCompare> Compare;
};
