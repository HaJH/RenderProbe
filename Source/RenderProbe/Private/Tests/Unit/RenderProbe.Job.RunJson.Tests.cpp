// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "RenderProbeDiagnostic.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "Templates/ValueOrError.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeRunJsonTests
{

static constexpr const TCHAR* MalformedJobJson = TEXT("{ \"outputDir\": ");

TEST_CLASS(RenderProbeRunJson, "RenderProbe.Unit.Job")
{
	TEST_METHOD(MalformedJsonGivesInvalidJobDiagnostic)
	{
		const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
		const FRenderProbeResult Result = Runner->RunJson(MalformedJobJson);

		ASSERT_THAT(IsFalse(Result.bOk));
		ASSERT_THAT(IsTrue(Result.Items.IsEmpty()));
		ASSERT_THAT(AreEqual(1, Result.Diagnostics.Num()));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Kind == ERenderProbeDiagnosticKind::InvalidJob));
		ASSERT_THAT(IsFalse(Result.Diagnostics[0].Detail.IsEmpty()));
	}

	TEST_METHOD(ReadJobJsonGivesInvalidJobResultForMalformedJson)
	{
		TValueOrError<FRenderProbeJob, FRenderProbeResult> Job = URenderProbeRunner::ReadJobJson(MalformedJobJson);

		ASSERT_THAT(IsTrue(Job.HasError()));
		const FRenderProbeResult& Result = Job.GetError();
		ASSERT_THAT(IsFalse(Result.bOk));
		ASSERT_THAT(AreEqual(1, Result.Diagnostics.Num()));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Kind == ERenderProbeDiagnosticKind::InvalidJob));
	}

	TEST_METHOD(ReadJobFileGivesInvalidJobResultForMissingFile)
	{
		const FString MissingPath = FPaths::Combine(FPaths::ProjectIntermediateDir(), FString::Printf(TEXT("RenderProbeMissingJob_%s.json"), *FGuid::NewGuid().ToString()));
		TValueOrError<FRenderProbeJob, FRenderProbeResult> Job = URenderProbeRunner::ReadJobFile(MissingPath);

		ASSERT_THAT(IsTrue(Job.HasError()));
		const FRenderProbeResult& Result = Job.GetError();
		ASSERT_THAT(IsFalse(Result.bOk));
		ASSERT_THAT(AreEqual(1, Result.Diagnostics.Num()));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Kind == ERenderProbeDiagnosticKind::InvalidJob));
		ASSERT_THAT(IsTrue(Result.Diagnostics[0].Detail.Contains(MissingPath)));
	}
};

}

#endif
