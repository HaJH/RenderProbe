// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RenderProbeProfileSettings.h"

#include "RenderProbeProfile.generated.h"

UCLASS(BlueprintType, meta = (ToolTip = "Saved scene profile preset. A job refers to it by asset path in profile.asset."))
class RENDERPROBE_API URenderProbeProfile : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Scene profile settings, the same structure as profile.settings in a job."))
	FRenderProbeProfileSettings Settings;
};
