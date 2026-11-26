// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeMaterialStats.h"

#include "MaterialEditingLibrary.h"
#include "MaterialShared.h"
#include "Materials/MaterialInterface.h"
#include "RenderProbeDiagnostic.h"
#include "RenderProbeResult.h"
#include "RHIShaderPlatform.h"

namespace RenderProbeMaterialStats
{
	static constexpr const TCHAR* CompileErrorSeparator = TEXT("\n");

	FMaterialResource* FindResource(UMaterialInterface* Material)
	{
		return Material ? Material->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
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
