// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Math/IntRect.h"
#include "Misc/Optional.h"
#include "RenderProbeDiagnostic.h"

#include "RenderProbeResult.generated.h"

USTRUCT(BlueprintType, meta = (ToolTip = "Shader statistics of the material permutation a slot actually uses."))
struct RENDERPROBE_API FRenderProbeMaterialStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Path of the measured material."))
	FString Material;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Instruction count of the most expensive vertex shader."))
	int32 VertexShaderInstructions = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Instruction count of the most expensive pixel shader."))
	int32 PixelShaderInstructions = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Number of samplers the material requires."))
	int32 Samplers = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Number of textures sampled by the vertex shader."))
	int32 VertexTextureSamples = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Number of textures sampled by the pixel shader."))
	int32 PixelTextureSamples = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Number of virtual texture lookups."))
	int32 VirtualTextureSamples = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Number of interpolator scalars used for UVs."))
	int32 UvScalars = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Number of interpolator scalars used by custom interpolators."))
	int32 InterpolatorScalars = 0;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Result for one subject."))
struct RENDERPROBE_API FRenderProbeItemResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Subject name from the job."))
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Capture file path per camera name."))
	TMap<FString, FString> Images;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Material statistics per slot key (slot index, or label without a mesh). Slots that could not be measured are absent and have a diagnostic instead."))
	TMap<FString, FRenderProbeMaterialStats> Stats;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Diagnostics about this subject."))
	TArray<FRenderProbeDiagnostic> Diagnostics;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Result of one comparison for one camera. Without a verdict, similar and the numbers are absent."))
struct RENDERPROBE_API FRenderProbeCompareResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Comparison name from the job."))
	FString Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "First subject name."))
	FString A;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Second subject name."))
	FString B;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Camera name."))
	FString Camera;

	UPROPERTY(EditAnywhere, Category = "RenderProbe", meta = (ToolTip = "Whether the two captures are similar under the comparison's tolerance. Absent when no verdict could be made (nondeterministic capture or missing image)."))
	TOptional<bool> Similar;

	UPROPERTY(EditAnywhere, Category = "RenderProbe", meta = (ToolTip = "Global difference over the whole image, from 0 to 1."))
	TOptional<double> GlobalDifference;

	UPROPERTY(EditAnywhere, Category = "RenderProbe", meta = (ToolTip = "Largest per-pixel difference inside diffBounds, from 0 to 1."))
	TOptional<double> MaxLocalDifference;

	UPROPERTY(EditAnywhere, Category = "RenderProbe", meta = (ToolTip = "Bounding box in pixels of the differing pixels, as {min, max}. Absent when no pixel differs."))
	TOptional<FIntRect> DiffBounds;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Path of the difference image, <outputDir>/diff/<compare>__<camera>.png. Empty when no pixel differs."))
	FString DeltaImage;
};

USTRUCT(BlueprintType, meta = (ToolTip = "Result of a render job. Also written to <outputDir>/result.json."))
struct RENDERPROBE_API FRenderProbeResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "False when any diagnostic exists or any comparison is not similar."))
	bool bOk = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Results per subject, in job order."))
	TArray<FRenderProbeItemResult> Items;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Comparison results per comparison and camera."))
	TArray<FRenderProbeCompareResult> Compare;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Job-wide diagnostics."))
	TArray<FRenderProbeDiagnostic> Diagnostics;
};
