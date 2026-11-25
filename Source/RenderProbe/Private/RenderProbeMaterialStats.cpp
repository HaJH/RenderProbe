// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeMaterialStats.h"

#include "AssetCompilingManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "MaterialEditingLibrary.h"
#include "MaterialShared.h"
#include "Materials/MaterialInterface.h"
#include "RenderProbeDiagnostic.h"
#include "RenderProbeResult.h"
#include "RHIShaderPlatform.h"

namespace RenderProbeMaterialStats
{
	static constexpr float CompilationPollIntervalSeconds = 0.05f;
	static constexpr const TCHAR* CompileErrorSeparator = TEXT("\n");

	FMaterialResource* FindResource(UMaterialInterface* Material)
	{
		return Material ? Material->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
	}

	static TSet<const FMaterialResource*> CollectUnfinished(const TArray<FMaterialResource*>& Resources)
	{
		TSet<const FMaterialResource*> Unfinished;
		for (const FMaterialResource* Resource : Resources)
		{
			if (!Resource->IsCompilationFinished())
			{
				Unfinished.Add(Resource);
			}
		}
		return Unfinished;
	}

	TSet<const FMaterialResource*> WaitForCompilation(const TArray<FMaterialResource*>& Resources, const double TimeoutSeconds)
	{
		for (FMaterialResource* Resource : Resources)
		{
			if (!Resource->IsGameThreadShaderMapComplete())
			{
				Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::High);
			}
		}

		const double Deadline = FPlatformTime::Seconds() + TimeoutSeconds;
		TSet<const FMaterialResource*> Unfinished = CollectUnfinished(Resources);
		while (!Unfinished.IsEmpty() && FPlatformTime::Seconds() < Deadline)
		{
			FAssetCompilingManager::Get().ProcessAsyncTasks();
			FPlatformProcess::Sleep(CompilationPollIntervalSeconds);
			Unfinished = CollectUnfinished(Resources);
		}
		return Unfinished;
	}

	static FRenderProbeMaterialStats ToResultStats(const UMaterialInterface& Material, const FMaterialStatistics& Statistics)
	{
		FRenderProbeMaterialStats Stats;
		Stats.Material = Material.GetPathName();
		Stats.VertexShaderInstructions = Statistics.NumVertexShaderInstructions;
		Stats.PixelShaderInstructions = Statistics.NumPixelShaderInstructions;
		Stats.Samplers = Statistics.NumSamplers;
		Stats.VertexTextureSamples = Statistics.NumVertexTextureSamples;
		Stats.PixelTextureSamples = Statistics.NumPixelTextureSamples;
		Stats.VirtualTextureSamples = Statistics.NumVirtualTextureSamples;
		Stats.UvScalars = Statistics.NumUVScalars;
		Stats.InterpolatorScalars = Statistics.NumInterpolatorScalars;
		return Stats;
	}

	void Measure(UMaterialInterface* Material, const FString& SlotKey, const TSet<const FMaterialResource*>& StillCompiling, FRenderProbeItemResult& OutItem)
	{
		if (!Material)
		{
			OutItem.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::StatsUnavailable, SlotKey, TEXT("slot has no material")));
			return;
		}

		const FMaterialResource* Resource = FindResource(Material);
		if (!Resource)
		{
			OutItem.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::StatsUnavailable, SlotKey,
				FString::Printf(TEXT("%s has no material resource for the editor shader platform"), *Material->GetPathName())));
			return;
		}

		if (StillCompiling.Contains(Resource))
		{
			OutItem.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::Timeout, SlotKey,
				FString::Printf(TEXT("%s was still compiling when the wait limit passed"), *Material->GetPathName())));
			return;
		}

		const TArray<FString>& CompileErrors = Resource->GetCompileErrors();
		if (!CompileErrors.IsEmpty())
		{
			OutItem.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::CompileError, SlotKey,
				FString::Printf(TEXT("%s: %s"), *Material->GetPathName(), *FString::Join(CompileErrors, CompileErrorSeparator))));
			return;
		}

		if (!Resource->GetGameThreadShaderMap())
		{
			OutItem.Diagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::StatsUnavailable, SlotKey,
				FString::Printf(TEXT("%s has no shader map"), *Material->GetPathName())));
			return;
		}

		OutItem.Stats.Add(SlotKey, ToResultStats(*Material, UMaterialEditingLibrary::GetStatistics(Material)));
	}
}
