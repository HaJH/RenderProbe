// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeCommandletArgs.h"

#include "Misc/Parse.h"
#include "RenderProbeCommandLineSwitches.h"

namespace RenderProbeCommandletArgsLocal
{
	static constexpr const TCHAR* JobSwitch = TEXT("-job=");
	static constexpr const TCHAR* OutSwitch = TEXT("-out=");
	static constexpr const TCHAR* SchemaSwitch = TEXT("-schema=");
	static constexpr TCHAR SwitchPrefix = TEXT('-');

	static TOptional<FString> FindValue(const FString& Params, const TCHAR* Switch, FString& OutError)
	{
		FString Value;
		if (!FParse::Value(*Params, Switch, Value))
		{
			return {};
		}
		if (Value.IsEmpty() || Value[0] == SwitchPrefix)
		{
			OutError = FString::Printf(TEXT("%s needs a value"), Switch);
		}
		return Value;
	}
}

namespace RenderProbeCommandletArgs
{
	FString GetUsage()
	{
		return FString::Printf(TEXT("Usage: -run=RenderProbe -job=<job JSON file> [-out=<output folder>] %s | -run=RenderProbe -schema=<schema file to write>"),
			RenderProbeCommandLineSwitches::AllowCommandletRendering);
	}

	bool Parse(const FString& Params, FRenderProbeCommandletArgs& OutArgs, FString& OutError)
	{
		using namespace RenderProbeCommandletArgsLocal;

		FString ValueError;
		const TOptional<FString> JobFilePath = FindValue(Params, JobSwitch, ValueError);
		const TOptional<FString> OutputDir = FindValue(Params, OutSwitch, ValueError);
		const TOptional<FString> SchemaFilePath = FindValue(Params, SchemaSwitch, ValueError);
		if (!ValueError.IsEmpty())
		{
			OutError = MoveTemp(ValueError);
			return false;
		}
		if (JobFilePath.IsSet() == SchemaFilePath.IsSet())
		{
			OutError = FString::Printf(TEXT("give exactly one of %s and %s"), JobSwitch, SchemaSwitch);
			return false;
		}
		if (SchemaFilePath.IsSet() && OutputDir.IsSet())
		{
			OutError = FString::Printf(TEXT("%s applies only with %s"), OutSwitch, JobSwitch);
			return false;
		}

		FRenderProbeCommandletArgs Args;
		if (SchemaFilePath.IsSet())
		{
			Args.Mode = ERenderProbeCommandletMode::WriteSchema;
			Args.SchemaFilePath = SchemaFilePath.GetValue();
		}
		else
		{
			Args.Mode = ERenderProbeCommandletMode::RunJob;
			Args.JobFilePath = JobFilePath.GetValue();
			Args.OutputDir = OutputDir;
		}
		OutArgs = MoveTemp(Args);
		return true;
	}
}
