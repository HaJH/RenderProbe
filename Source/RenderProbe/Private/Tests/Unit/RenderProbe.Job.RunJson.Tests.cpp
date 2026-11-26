// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeDiagnostic.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
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
};

}

#endif
