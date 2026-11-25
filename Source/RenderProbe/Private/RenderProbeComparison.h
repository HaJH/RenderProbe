// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RenderProbeDiagnostic.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"

struct FImageTolerance;

struct FRenderProbeComparisonCaptures
{
	FString A;
	FString ARepeat;
	FString B;
	FString BRepeat;
};

struct FRenderProbeSubjectDiagnostic
{
	FString Subject;
	FRenderProbeDiagnostic Diagnostic;
};

struct FRenderProbeComparisonOutcome
{
	FRenderProbeCompareResult Result;
	TArray<FRenderProbeSubjectDiagnostic> SubjectDiagnostics;
	TArray<FRenderProbeDiagnostic> JobDiagnostics;
};

namespace RenderProbeComparison
{
	FImageTolerance ToEngineTolerance(ERenderProbeTolerance Tolerance);

	FRenderProbeComparisonOutcome Judge(const FRenderProbeCompare& Compare, const FString& CameraName, const FRenderProbeComparisonCaptures& Captures, const FString& DeltaPath);
}
