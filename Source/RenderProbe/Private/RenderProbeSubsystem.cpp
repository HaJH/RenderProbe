// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeSubsystem.h"

#include "RenderProbeJson.h"
#include "RenderProbeRunner.h"
#include "UObject/StrongObjectPtr.h"

FRenderProbeResult URenderProbeSubsystem::RunJob(const FRenderProbeJob& Job)
{
	const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
	return Runner->Run(Job);
}

FRenderProbeResult URenderProbeSubsystem::RunJobFile(const FString& JobFilePath)
{
	const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
	return Runner->RunJobFile(JobFilePath);
}

FString URenderProbeSubsystem::GetJobSchema() const
{
	return RenderProbeJson::JobSchemaString();
}
