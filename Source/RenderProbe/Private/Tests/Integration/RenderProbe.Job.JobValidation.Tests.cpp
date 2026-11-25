// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Algo/AllOf.h"
#include "Algo/AnyOf.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "Tests/Support/RenderProbeTestOutputDir.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeJobValidationTests
{

static constexpr const TCHAR* UnloadedMeshPath = TEXT("/Engine/RenderProbeTests/NeverLoaded.NeverLoaded");
static constexpr const TCHAR* SubjectA = TEXT("subjectA");
static constexpr const TCHAR* SubjectB = TEXT("subjectB");

static FRenderProbeCamera MakeCamera(const TCHAR* Name)
{
	FRenderProbeCamera Camera;
	Camera.Name = Name;
	return Camera;
}

static FRenderProbeItem MakeMeshItem(const TCHAR* Name)
{
	FRenderProbeItem Item;
	Item.Name = Name;
	Item.Mesh = UnloadedMeshPath;
	return Item;
}

static FRenderProbeItem MakeStatsOnlyItem(const TCHAR* Name, const bool bStats)
{
	FRenderProbeItem Item;
	Item.Name = Name;
	Item.bStats = bStats;
	return Item;
}

static FRenderProbeCompare MakeCompare(const TCHAR* Name, const TCHAR* A, const TCHAR* B)
{
	FRenderProbeCompare Compare;
	Compare.Name = Name;
	Compare.A = A;
	Compare.B = B;
	return Compare;
}

/** True when nothing ran, every diagnostic is a top-level InvalidJob, and one of them mentions DetailFragment. */
static bool IsRejectedMentioning(const FRenderProbeResult& Result, const TCHAR* DetailFragment)
{
	const bool bOnlyInvalidJob = Algo::AllOf(Result.Diagnostics, [](const FRenderProbeDiagnostic& Diagnostic)
	{
		return Diagnostic.Kind == ERenderProbeDiagnosticKind::InvalidJob;
	});
	const bool bMentioned = Algo::AnyOf(Result.Diagnostics, [DetailFragment](const FRenderProbeDiagnostic& Diagnostic)
	{
		return Diagnostic.Detail.Contains(DetailFragment, ESearchCase::CaseSensitive);
	});
	return !Result.bOk && Result.Items.IsEmpty() && Result.Compare.IsEmpty() && bOnlyInvalidJob && bMentioned;
}

TEST_CLASS(RenderProbeJobValidation, "RenderProbe.Integration.Job")
{
	FRenderProbeTestOutputDir OutputDir;
	FRenderProbeJob Job;

	BEFORE_EACH()
	{
		Job.Profile.Settings.Emplace();
		Job.OutputDir = OutputDir.Get();
		Job.Cameras.Add(MakeCamera(TEXT("front")));
		Job.Items.Add(MakeMeshItem(SubjectA));
		Job.Items.Add(MakeMeshItem(SubjectB));
		Job.Compare.Add(MakeCompare(TEXT("ab"), SubjectA, SubjectB));
	}

	FRenderProbeResult RunJob() const
	{
		const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
		return Runner->Run(Job);
	}

	TEST_METHOD(ProfileWithBothAssetAndSettingsIsRejected)
	{
		Job.Profile.Asset = TEXT("/Engine/RenderProbeTests/Profile.Profile");
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("both asset and settings"))));
	}

	TEST_METHOD(ProfileWithNeitherAssetNorSettingsIsRejected)
	{
		Job.Profile.Settings.Reset();
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("neither asset nor settings"))));
	}

	TEST_METHOD(EmptyOutputDirIsRejected)
	{
		Job.OutputDir.Empty();
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("outputDir is empty"))));
	}

	TEST_METHOD(NameWithDisallowedCharacterIsRejected)
	{
		Job.Cameras.Add(MakeCamera(TEXT("side.view")));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("'side.view' uses characters"))));
	}

	TEST_METHOD(NameContainingDoubleUnderscoreIsRejected)
	{
		Job.Cameras.Add(MakeCamera(TEXT("side__left")));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("'side__left' contains '__'"))));
	}

	TEST_METHOD(DuplicateItemNameIsRejected)
	{
		Job.Items.Add(MakeMeshItem(SubjectA));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("items name 'subjectA' is used more than once"))));
	}

	TEST_METHOD(DuplicateCameraNameIsRejected)
	{
		Job.Cameras.Add(MakeCamera(TEXT("front")));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("cameras name 'front' is used more than once"))));
	}

	TEST_METHOD(DuplicateCompareNameIsRejected)
	{
		Job.Compare.Add(MakeCompare(TEXT("ab"), SubjectB, SubjectA));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("compare name 'ab' is used more than once"))));
	}

	TEST_METHOD(CompareTargetNotInItemsIsRejected)
	{
		Job.Compare.Add(MakeCompare(TEXT("ghost"), SubjectA, TEXT("missingSubject")));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("item 'missingSubject', which is not in items"))));
	}

	TEST_METHOD(CompareTargetWithoutMeshIsRejected)
	{
		Job.Items.Add(MakeStatsOnlyItem(TEXT("statsOnly"), true));
		Job.Compare.Add(MakeCompare(TEXT("toStats"), SubjectA, TEXT("statsOnly")));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("item 'statsOnly', which has no mesh"))));
	}

	TEST_METHOD(ItemWithoutMeshOrStatsIsRejected)
	{
		Job.Items.Add(MakeStatsOnlyItem(TEXT("idle"), false));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("item 'idle' has no mesh and stats is false"))));
	}

	TEST_METHOD(CapturePathsDifferingOnlyInCaseAreRejected)
	{
		Job.Items.Add(MakeMeshItem(TEXT("SubjectA")));
		const FRenderProbeResult Result = RunJob();
		ASSERT_THAT(IsTrue(IsRejectedMentioning(Result, TEXT("output paths collide ignoring case"))));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(Result, TEXT("SubjectA__front.png"))));
	}

	TEST_METHOD(CapturePathsJoinedAcrossSeparatorAreRejected)
	{
		Job.Items.Add(MakeMeshItem(TEXT("pair_")));
		Job.Items.Add(MakeMeshItem(TEXT("pair")));
		Job.Cameras.Add(MakeCamera(TEXT("cam")));
		Job.Cameras.Add(MakeCamera(TEXT("_cam")));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("pair___cam.png"))));
	}

	TEST_METHOD(DiffPathsJoinedAcrossSeparatorAreRejected)
	{
		Job.Cameras.Add(MakeCamera(TEXT("cam")));
		Job.Cameras.Add(MakeCamera(TEXT("_cam")));
		Job.Compare.Add(MakeCompare(TEXT("diffed_"), SubjectA, SubjectB));
		Job.Compare.Add(MakeCompare(TEXT("diffed"), SubjectA, SubjectB));
		ASSERT_THAT(IsTrue(IsRejectedMentioning(RunJob(), TEXT("diffed___cam.png"))));
	}
};

}

#endif
