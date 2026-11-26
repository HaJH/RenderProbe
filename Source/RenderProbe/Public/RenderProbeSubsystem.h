// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"

#include "RenderProbeSubsystem.generated.h"

UCLASS()
class RENDERPROBE_API URenderProbeSubsystem : public UEditorSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "RenderProbe", meta = (ToolTip = "Runs a render job and returns its result. The result is also written to <outputDir>/result.json."))
	FRenderProbeResult RunJob(const FRenderProbeJob& Job);

	UFUNCTION(BlueprintCallable, Category = "RenderProbe", meta = (ToolTip = "Reads a render job from a JSON file, runs it and returns its result. The result is also written to <outputDir>/result.json."))
	FRenderProbeResult RunJobFile(const FString& JobFilePath);

	UFUNCTION(BlueprintCallable, Category = "RenderProbe", meta = (ToolTip = "Returns the JSON Schema of the job and of the result, as a JSON object with job and result members. Field descriptions come from the structure definitions."))
	FString GetJobSchema() const;
};
