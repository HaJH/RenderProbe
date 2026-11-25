// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeCaptureSetup.h"

#include "RenderProbeProfileSettings.h"

namespace RenderProbeCaptureSetup
{
	static bool UsesLumen(const FRenderProbeProfileSettings& Settings)
	{
		const FPostProcessSettings& PostProcess = Settings.PostProcess;
		const bool bLumenGlobalIllumination = PostProcess.bOverride_DynamicGlobalIlluminationMethod
			? PostProcess.DynamicGlobalIlluminationMethod == EDynamicGlobalIlluminationMethod::Lumen
			: Settings.Lumen.bEnabled;
		const bool bLumenReflections = PostProcess.bOverride_ReflectionMethod
			? PostProcess.ReflectionMethod == EReflectionMethod::Lumen
			: Settings.Lumen.bEnabled;
		return bLumenGlobalIllumination || bLumenReflections;
	}

	static FPostProcessSettings MakeDeterministicPostProcess(const FPostProcessSettings& Source, const bool bUsesLumen)
	{
		FPostProcessSettings PostProcess = Source;
		PostProcess.bOverride_AutoExposureMethod = true;
		PostProcess.AutoExposureMethod = AEM_Manual;
		if (!PostProcess.bOverride_AutoExposureApplyPhysicalCameraExposure)
		{
			PostProcess.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
			PostProcess.AutoExposureApplyPhysicalCameraExposure = false;
		}
		if (!PostProcess.bOverride_DynamicGlobalIlluminationMethod)
		{
			PostProcess.bOverride_DynamicGlobalIlluminationMethod = true;
			PostProcess.DynamicGlobalIlluminationMethod = bUsesLumen ? EDynamicGlobalIlluminationMethod::Lumen : EDynamicGlobalIlluminationMethod::None;
		}
		if (!PostProcess.bOverride_ReflectionMethod)
		{
			PostProcess.bOverride_ReflectionMethod = true;
			PostProcess.ReflectionMethod = bUsesLumen ? EReflectionMethod::Lumen : EReflectionMethod::None;
		}
		return PostProcess;
	}

	FRenderProbeCaptureSetup Make(const FRenderProbeProfileSettings& Settings)
	{
		FRenderProbeCaptureSetup Setup;
		Setup.bUsesLumen = UsesLumen(Settings);
		Setup.CaptureCount = (Setup.bUsesLumen ? Settings.Lumen.ConvergenceCaptures : 0) + 1;
		Setup.PostProcess = MakeDeterministicPostProcess(Settings.PostProcess, Setup.bUsesLumen);
		return Setup;
	}
}
