// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ToolsetRegistry/ToolsetDefinition.h"

#include "RenderProbeToolset.generated.h"

/** Tools for rendering meshes and materials under fixed conditions and reading back images, material statistics and comparisons. */
UCLASS(BlueprintType, MinimalAPI)
class URenderProbeToolset : public UToolsetDefinition
{
	GENERATED_BODY()

public:
	/**
	 * Runs a render job: renders meshes and materials in an isolated preview world under a fixed scene profile, measures material shader statistics and compares captures.
	 * Captures, difference images and result.json are written to the job's output directory.
	 * The job and result formats are given by GetJobSchema.
	 * @param JobJson The render job as a JSON object.
	 * @return The result as a JSON object.
	 */
	UFUNCTION(meta = (AICallable), Category = "RenderProbe")
	static FString RunJob(const FString& JobJson);

	/**
	 * Runs a render job read from a JSON file, like RunJob.
	 * The job and result formats are given by GetJobSchema.
	 * @param JobFilePath Absolute path of the job JSON file.
	 * @return The result as a JSON object.
	 */
	UFUNCTION(meta = (AICallable), Category = "RenderProbe")
	static FString RunJobFile(const FString& JobFilePath);

	/**
	 * Returns the JSON Schema of the render job and of its result, as a JSON object with job and result members.
	 * @return The schemas as a JSON object.
	 */
	UFUNCTION(meta = (AICallable), Category = "RenderProbe")
	static FString GetJobSchema();
};
