// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FRenderProbeJob;
struct FRenderProbeResult;

namespace RenderProbeJson
{
	RENDERPROBE_API bool JobFromJsonString(const FString& JsonString, FRenderProbeJob& OutJob, FString& OutError);

	RENDERPROBE_API FString JobToJsonString(const FRenderProbeJob& Job);

	RENDERPROBE_API FString ResultToJsonString(const FRenderProbeResult& Result);

	/** JSON object with "job" and "result" members, each the JSON Schema of the JSON that FJsonObjectConverter reads or writes for that structure. */
	RENDERPROBE_API FString JobSchemaString();
}
