// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Misc/Guid.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "Tests/Support/RenderProbeTestOutputDir.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeAssetLoadingTests
{

static constexpr const TCHAR* Label = TEXT("missing");

TEST_CLASS(RenderProbeAssetLoading, "RenderProbe.Integration.Asset")
{
	FRenderProbeTestOutputDir OutputDir;

	TEST_METHOD(NonexistentMaterialPathReportsMissingAsset)
	{
		const FString MissingPath = FString::Printf(TEXT("/Engine/RenderProbeMissing_%s"), *FGuid::NewGuid().ToString());

		FRenderProbeJob Job;
		Job.Profile.Settings.Emplace();
		Job.OutputDir = OutputDir.Get();
		FRenderProbeItem& Item = Job.Items.AddDefaulted_GetRef();
		Item.Name = TEXT("statsOnly");
		Item.bStats = true;
		Item.Materials.Add(Label, MissingPath);

		const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
		const FRenderProbeResult Result = Runner->Run(Job);

		ASSERT_THAT(IsFalse(Result.bOk));
		ASSERT_THAT(IsTrue(Result.Diagnostics.IsEmpty()));
		ASSERT_THAT(AreEqual(1, Result.Items.Num()));
		const FRenderProbeItemResult& ItemResult = Result.Items[0];
		ASSERT_THAT(IsTrue(ItemResult.Stats.IsEmpty()));
		ASSERT_THAT(AreEqual(1, ItemResult.Diagnostics.Num()));
		ASSERT_THAT(IsTrue(ItemResult.Diagnostics[0].Kind == ERenderProbeDiagnosticKind::MissingAsset));
		ASSERT_THAT(AreEqual(FString(Label), ItemResult.Diagnostics[0].Slot));
		ASSERT_THAT(IsTrue(ItemResult.Diagnostics[0].Detail.Contains(MissingPath)));
	}
};

}

#endif
