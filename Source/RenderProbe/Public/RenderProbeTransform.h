// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/Rotator.h"
#include "Math/Vector.h"

#include "RenderProbeTransform.generated.h"

USTRUCT(BlueprintType, meta = (ToolTip = "Location and rotation in the preview world, without scale."))
struct RENDERPROBE_API FRenderProbePlacement
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "World location in centimeters, as {x, y, z}."))
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "World rotation in degrees, as {pitch, yaw, roll}."))
	FRotator Rotation = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Location, rotation and scale of a component placed in the preview world."))
struct RENDERPROBE_API FRenderProbeTransform
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Location in centimeters, as {x, y, z}."))
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Rotation in degrees, as {pitch, yaw, roll}."))
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderProbe", meta = (ToolTip = "Scale per axis, as {x, y, z}."))
	FVector Scale = FVector::OneVector;
};
