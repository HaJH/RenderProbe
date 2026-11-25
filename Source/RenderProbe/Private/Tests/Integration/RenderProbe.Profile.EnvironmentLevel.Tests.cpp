// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Engine/World.h"
#include "Misc/Guid.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "Tests/Support/RenderProbeTestOutputDir.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeEnvironmentLevelTests
{

TEST_CLASS(RenderProbeEnvironmentLevel, "RenderProbe.Integration.Profile")
{
	FRenderProbeTestOutputDir OutputDir;

	TEST_METHOD(NonexistentLevelReportsMissingAsset)
	{
		const FString MissingLevelName = FString::Printf(TEXT("RenderProbeMissingLevel_%s"), *FGuid::NewGuid().ToString());
		const FString MissingLevelPath = FString::Printf(TEXT("/Engine/%s.%s"), *MissingLevelName, *MissingLevelName);

		FRenderProbeJob Job;
		Job.OutputDir = OutputDir.Get();
		FRenderProbeEnvironmentLevel& Level = Job.Profile.Settings.Emplace().EnvironmentLevels.AddDefaulted_GetRef();
		Level.Level = TSoftObjectPtr<UWorld>(FSoftObjectPath(MissingLevelPath));

		const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
		const FRenderProbeResult Result = Runner->Run(Job);

		ASSERT_THAT(IsFalse(Result.bOk));
		ASSERT_THAT(AreEqual(1, Result.Diagnostics.Num()));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Kind == ERenderProbeDiagnosticKind::MissingAsset));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Detail.Contains(MissingLevelPath)));
	}
};

}

#endif
