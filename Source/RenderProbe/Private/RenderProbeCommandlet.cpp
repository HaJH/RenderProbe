// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeCommandlet.h"

#include "Misc/FileHelper.h"
#include "RenderProbeCommandletArgs.h"
#include "RenderProbeJson.h"
#include "RenderProbeLog.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "UObject/StrongObjectPtr.h"

namespace RenderProbeCommandletLocal
{
	static constexpr int32 JobSucceededExitCode = 0;
	static constexpr int32 JobFailedExitCode = 1;
	static constexpr int32 JobNotRunExitCode = 2;
}

URenderProbeCommandlet::URenderProbeCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
	UseCommandletResultAsExitCode = true;
	HelpUsage = RenderProbeCommandletArgs::Usage;
}

int32 URenderProbeCommandlet::Main(const FString& Params)
{
	FRenderProbeCommandletArgs Args;
	FString Error;
	if (!RenderProbeCommandletArgs::Parse(Params, Args, Error))
	{
		UE_LOG(LogRenderProbe, Error, TEXT("%s"), *Error);
		UE_LOG(LogRenderProbe, Display, TEXT("%s"), RenderProbeCommandletArgs::Usage);
		return RenderProbeCommandletLocal::JobNotRunExitCode;
	}

	switch (Args.Mode)
	{
	case ERenderProbeCommandletMode::RunJob:
		return RunJob(Args);
	case ERenderProbeCommandletMode::WriteSchema:
		return WriteSchema(Args);
	}
	checkNoEntry();
	return RenderProbeCommandletLocal::JobNotRunExitCode;
}

int32 URenderProbeCommandlet::RunJob(const FRenderProbeCommandletArgs& Args)
{
	const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
	const FRenderProbeResult Result = Runner->RunJobFile(Args.JobFilePath, Args.OutputDir);

	UE_LOG(LogRenderProbe, Display, TEXT("%s"), *RenderProbeJson::ResultToJsonString(Result));
	if (!Result.bOk)
	{
		UE_LOG(LogRenderProbe, Error, TEXT("job '%s' finished with diagnostics or dissimilar comparisons"), *Args.JobFilePath);
		return RenderProbeCommandletLocal::JobFailedExitCode;
	}
	return RenderProbeCommandletLocal::JobSucceededExitCode;
}

int32 URenderProbeCommandlet::WriteSchema(const FRenderProbeCommandletArgs& Args)
{
	if (!FFileHelper::SaveStringToFile(RenderProbeJson::JobSchemaString(), *Args.SchemaFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogRenderProbe, Error, TEXT("cannot write the job schema to '%s'"), *Args.SchemaFilePath);
		return RenderProbeCommandletLocal::JobNotRunExitCode;
	}
	return RenderProbeCommandletLocal::JobSucceededExitCode;
}
