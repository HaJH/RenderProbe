// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeToolset.h"

#include "RenderProbeJson.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "UObject/StrongObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RenderProbeToolset)

FString URenderProbeToolset::RunJob(const FString& JobJson)
{
	const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
	return RenderProbeJson::ResultToJsonString(Runner->RunJson(JobJson));
}

FString URenderProbeToolset::RunJobFile(const FString& JobFilePath)
{
	const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
	return RenderProbeJson::ResultToJsonString(Runner->RunJobFile(JobFilePath));
}

FString URenderProbeToolset::GetJobSchema()
{
	return RenderProbeJson::JobSchemaString();
}
