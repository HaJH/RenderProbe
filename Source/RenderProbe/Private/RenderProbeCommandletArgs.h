// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/Optional.h"

enum class ERenderProbeCommandletMode : uint8
{
	RunJob,
	WriteSchema,
};

struct FRenderProbeCommandletArgs
{
	ERenderProbeCommandletMode Mode = ERenderProbeCommandletMode::RunJob;
	FString JobFilePath;
	TOptional<FString> OutputDir;
	FString SchemaFilePath;
};

namespace RenderProbeCommandletArgs
{
	FString GetUsage();

	bool Parse(const FString& Params, FRenderProbeCommandletArgs& OutArgs, FString& OutError);
}
