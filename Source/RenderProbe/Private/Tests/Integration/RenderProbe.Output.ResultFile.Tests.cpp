// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderProbeDiagnostic.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "Tests/Support/RenderProbeTestOutputDir.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeResultFileTests
{

static constexpr const TCHAR* BlockingFileName = TEXT("blocking-file");
static constexpr const TCHAR* WriteResultStep = TEXT("writing result.json");

TEST_CLASS(RenderProbeResultFile, "RenderProbe.Integration.Output")
{
	FRenderProbeTestOutputDir OutputDir;

	TEST_METHOD(UnwritableResultFileIsReportedAtTopLevel)
	{
		const FString BlockingFilePath = FPaths::Combine(OutputDir.Get(), BlockingFileName);
		ASSERT_THAT(IsTrue(FFileHelper::SaveStringToFile(BlockingFileName, *BlockingFilePath)));

		FRenderProbeJob Job;
		Job.Profile.Settings.Emplace();
		Job.OutputDir = BlockingFilePath;
		const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
		const FRenderProbeResult Result = Runner->Run(Job);

		ASSERT_THAT(IsFalse(Result.bOk));
		ASSERT_THAT(AreEqual(1, Result.Diagnostics.Num()));
		const FRenderProbeDiagnostic& Diagnostic = Result.Diagnostics[0];
		ASSERT_THAT(IsTrue(Diagnostic.Kind == ERenderProbeDiagnosticKind::OutputFailed));
		ASSERT_THAT(IsTrue(Diagnostic.Detail.Contains(WriteResultStep, ESearchCase::CaseSensitive)));
		ASSERT_THAT(IsTrue(Diagnostic.Detail.Contains(BlockingFilePath, ESearchCase::CaseSensitive)));
	}
};

}

#endif
