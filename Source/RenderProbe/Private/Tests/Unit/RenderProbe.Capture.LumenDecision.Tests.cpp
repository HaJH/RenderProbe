// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Engine/Scene.h"
#include "RenderProbeCaptureSetup.h"
#include "RenderProbeProfileSettings.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeLumenDecisionTests
{

static constexpr int32 ConvergenceCaptures = 3;
static constexpr int32 CaptureCountWithConvergence = ConvergenceCaptures + 1;
static constexpr int32 SingleCapture = 1;

TEST_CLASS(RenderProbeLumenDecision, "RenderProbe.Unit.Capture")
{
	FRenderProbeProfileSettings Settings;

	BEFORE_EACH()
	{
		Settings.Lumen.ConvergenceCaptures = ConvergenceCaptures;
	}

	TEST_METHOD(DisabledProfileWithoutOverridesUsesNoLumen)
	{
		Settings.Lumen.bEnabled = false;

		const FRenderProbeCaptureSetup Setup = RenderProbeCaptureSetup::Make(Settings);

		ASSERT_THAT(IsFalse(Setup.bUsesLumen));
		ASSERT_THAT(AreEqual(SingleCapture, Setup.CaptureCount));
		ASSERT_THAT(IsTrue(Setup.PostProcess.DynamicGlobalIlluminationMethod == EDynamicGlobalIlluminationMethod::None));
		ASSERT_THAT(IsTrue(Setup.PostProcess.ReflectionMethod == EReflectionMethod::None));
	}

	TEST_METHOD(EnabledProfileWithoutOverridesUsesLumenEverywhere)
	{
		Settings.Lumen.bEnabled = true;

		const FRenderProbeCaptureSetup Setup = RenderProbeCaptureSetup::Make(Settings);

		ASSERT_THAT(IsTrue(Setup.bUsesLumen));
		ASSERT_THAT(AreEqual(CaptureCountWithConvergence, Setup.CaptureCount));
		ASSERT_THAT(IsTrue(Setup.PostProcess.DynamicGlobalIlluminationMethod == EDynamicGlobalIlluminationMethod::Lumen));
		ASSERT_THAT(IsTrue(Setup.PostProcess.ReflectionMethod == EReflectionMethod::Lumen));
	}

	TEST_METHOD(LumenOverrideInPostProcessTurnsLumenOnForDisabledProfile)
	{
		Settings.Lumen.bEnabled = false;
		Settings.PostProcess.bOverride_DynamicGlobalIlluminationMethod = true;
		Settings.PostProcess.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::Lumen;

		const FRenderProbeCaptureSetup Setup = RenderProbeCaptureSetup::Make(Settings);

		ASSERT_THAT(IsTrue(Setup.bUsesLumen));
		ASSERT_THAT(AreEqual(CaptureCountWithConvergence, Setup.CaptureCount));
		ASSERT_THAT(IsTrue(Setup.PostProcess.ReflectionMethod == EReflectionMethod::Lumen));
	}

	TEST_METHOD(NonLumenOverridesInPostProcessTurnLumenOffForEnabledProfile)
	{
		Settings.Lumen.bEnabled = true;
		Settings.PostProcess.bOverride_DynamicGlobalIlluminationMethod = true;
		Settings.PostProcess.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::ScreenSpace;
		Settings.PostProcess.bOverride_ReflectionMethod = true;
		Settings.PostProcess.ReflectionMethod = EReflectionMethod::ScreenSpace;

		const FRenderProbeCaptureSetup Setup = RenderProbeCaptureSetup::Make(Settings);

		ASSERT_THAT(IsFalse(Setup.bUsesLumen));
		ASSERT_THAT(AreEqual(SingleCapture, Setup.CaptureCount));
		ASSERT_THAT(IsTrue(Setup.PostProcess.DynamicGlobalIlluminationMethod == EDynamicGlobalIlluminationMethod::ScreenSpace));
		ASSERT_THAT(IsTrue(Setup.PostProcess.ReflectionMethod == EReflectionMethod::ScreenSpace));
	}
};

}

#endif
