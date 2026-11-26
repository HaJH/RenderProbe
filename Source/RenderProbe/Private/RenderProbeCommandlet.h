// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "RenderProbeCommandlet.generated.h"

struct FRenderProbeCommandletArgs;

UCLASS()
class URenderProbeCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	explicit URenderProbeCommandlet(const FObjectInitializer& ObjectInitializer);

	virtual int32 Main(const FString& Params) override;

private:
	static int32 RunJob(const FRenderProbeCommandletArgs& Args);

	static int32 WriteSchema(const FRenderProbeCommandletArgs& Args);
};
