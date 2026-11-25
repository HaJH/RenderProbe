// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "Tests/Support/RenderProbeTestMaterials.h"
#include "Tests/Support/RenderProbeTestOutputDir.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeMaterialStatsTests
{

static constexpr const TCHAR* SwitchName = TEXT("RenderProbeTestSwitch");
static constexpr const TCHAR* SwitchOnLabel = TEXT("switchOn");
static constexpr const TCHAR* SwitchOffLabel = TEXT("switchOff");
static constexpr const TCHAR* BrokenLabel = TEXT("broken");
static constexpr const TCHAR* MaterialCompileFailureLog = TEXT("Failed to compile Material");
static constexpr const TCHAR* MissingTextureLog = TEXT("Missing input texture");
static constexpr int32 IgnoreAnyOccurrences = -1;

static bool HaveSameStatistics(const FRenderProbeMaterialStats& A, const FRenderProbeMaterialStats& B)
{
	return A.VertexShaderInstructions == B.VertexShaderInstructions
		&& A.PixelShaderInstructions == B.PixelShaderInstructions
		&& A.Samplers == B.Samplers
		&& A.VertexTextureSamples == B.VertexTextureSamples
		&& A.PixelTextureSamples == B.PixelTextureSamples
		&& A.VirtualTextureSamples == B.VirtualTextureSamples
		&& A.UvScalars == B.UvScalars
		&& A.InterpolatorScalars == B.InterpolatorScalars;
}

TEST_CLASS(RenderProbeMaterialStats, "RenderProbe.Integration.Stats")
{
	FRenderProbeTestOutputDir OutputDir;
	FRenderProbeJob Job;

	BEFORE_EACH()
	{
		Job.Profile.Settings.Emplace();
		Job.OutputDir = OutputDir.Get();
	}

	FRenderProbeResult RunStatsOnly(TMap<FString, FString> LabeledMaterials) const
	{
		FRenderProbeJob StatsJob = Job;
		FRenderProbeItem& Item = StatsJob.Items.AddDefaulted_GetRef();
		Item.Name = TEXT("statsOnly");
		Item.bStats = true;
		Item.Materials = MoveTemp(LabeledMaterials);

		const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
		return Runner->Run(StatsJob);
	}

	TEST_METHOD(StaticSwitchPermutationsReportDifferentStatistics)
	{
		const TStrongObjectPtr<UMaterial> Parent(RenderProbeTestMaterials::CreateStaticSwitchMaterial(SwitchName));
		const TStrongObjectPtr<UMaterialInstanceConstant> SwitchOn(RenderProbeTestMaterials::CreateInstanceWithStaticSwitch(Parent.Get(), SwitchName, true));
		const TStrongObjectPtr<UMaterialInstanceConstant> SwitchOff(RenderProbeTestMaterials::CreateInstanceWithStaticSwitch(Parent.Get(), SwitchName, false));
		ASSERT_THAT(IsNotNull(SwitchOn.Get()));
		ASSERT_THAT(IsNotNull(SwitchOff.Get()));

		TMap<FString, FString> Materials;
		Materials.Add(SwitchOnLabel, SwitchOn->GetPathName());
		Materials.Add(SwitchOffLabel, SwitchOff->GetPathName());
		const FRenderProbeResult Result = RunStatsOnly(MoveTemp(Materials));

		ASSERT_THAT(AreEqual(1, Result.Items.Num()));
		const FRenderProbeItemResult& Item = Result.Items[0];
		ASSERT_THAT(IsTrue(Item.Diagnostics.IsEmpty()));
		ASSERT_THAT(IsTrue(Result.bOk));
		const FRenderProbeMaterialStats* OnStats = Item.Stats.Find(SwitchOnLabel);
		const FRenderProbeMaterialStats* OffStats = Item.Stats.Find(SwitchOffLabel);
		ASSERT_THAT(IsNotNull(OnStats));
		ASSERT_THAT(IsNotNull(OffStats));
		ASSERT_THAT(IsTrue(OnStats->PixelShaderInstructions > 0));
		ASSERT_THAT(IsTrue(OffStats->PixelShaderInstructions > 0));
		ASSERT_THAT(IsFalse(HaveSameStatistics(*OnStats, *OffStats)));
	}

	TEST_METHOD(CompileFailureReportsCompileError)
	{
		TestRunner->AddExpectedMessagePlain(MaterialCompileFailureLog, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, IgnoreAnyOccurrences);
		TestRunner->AddExpectedMessagePlain(MissingTextureLog, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, IgnoreAnyOccurrences);
		const TStrongObjectPtr<UMaterial> Broken(RenderProbeTestMaterials::CreateMaterialMissingTexture());

		TMap<FString, FString> Materials;
		Materials.Add(BrokenLabel, Broken->GetPathName());
		const FRenderProbeResult Result = RunStatsOnly(MoveTemp(Materials));

		ASSERT_THAT(IsFalse(Result.bOk));
		ASSERT_THAT(AreEqual(1, Result.Items.Num()));
		const FRenderProbeItemResult& Item = Result.Items[0];
		ASSERT_THAT(IsTrue(Item.Stats.IsEmpty()));
		ASSERT_THAT(AreEqual(1, Item.Diagnostics.Num()));
		ASSERT_THAT(IsTrue(Item.Diagnostics[0].Kind == ERenderProbeDiagnosticKind::CompileError));
		ASSERT_THAT(AreEqual(FString(BrokenLabel), Item.Diagnostics[0].Slot));
		ASSERT_THAT(IsFalse(Item.Diagnostics[0].Detail.IsEmpty()));
	}
};

}

#endif
