// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "HAL/FileManager.h"
#include "Math/Color.h"
#include "Math/IntPoint.h"
#include "Math/IntRect.h"
#include "Materials/Material.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "Tests/Support/RenderProbeTestMaterials.h"
#include "Tests/Support/RenderProbeTestOutputDir.h"
#include "Tests/Support/RenderProbeTestRhi.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeCompareRenderTests
{

static constexpr const TCHAR* CubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
static constexpr const TCHAR* FirstSubjectName = TEXT("first");
static constexpr const TCHAR* SecondSubjectName = TEXT("second");
static constexpr const TCHAR* CameraName = TEXT("front");
static constexpr const TCHAR* CompareName = TEXT("pair");
static constexpr const TCHAR* FirstSlotKey = TEXT("0");
static constexpr int32 SmallResolutionX = 64;
static constexpr int32 SmallResolutionY = 48;
static constexpr float CameraPitch = -20.f;
static const FIntPoint ImageCenter(SmallResolutionX / 2, SmallResolutionY / 2);

static FRenderProbeItem MakeCubeItem(const TCHAR* Name, const UMaterialInterface& Material)
{
	FRenderProbeItem Item;
	Item.Name = Name;
	Item.Mesh = CubeMeshPath;
	Item.Materials.Add(FirstSlotKey, Material.GetPathName());
	return Item;
}

static FRenderProbeJob MakeCompareJob(const FString& OutputDir, const UMaterialInterface& FirstMaterial, const UMaterialInterface& SecondMaterial)
{
	FRenderProbeJob Job;
	Job.Profile.Settings.Emplace();
	Job.OutputDir = OutputDir;
	Job.Resolution = FIntPoint(SmallResolutionX, SmallResolutionY);

	FRenderProbeCamera& Camera = Job.Cameras.AddDefaulted_GetRef();
	Camera.Name = CameraName;
	Camera.Pitch = CameraPitch;

	Job.Items.Add(MakeCubeItem(FirstSubjectName, FirstMaterial));
	Job.Items.Add(MakeCubeItem(SecondSubjectName, SecondMaterial));

	FRenderProbeCompare& Compare = Job.Compare.AddDefaulted_GetRef();
	Compare.Name = CompareName;
	Compare.A = FirstSubjectName;
	Compare.B = SecondSubjectName;
	Compare.Tolerance = ERenderProbeTolerance::Low;
	return Job;
}

static FRenderProbeResult RunJob(const FRenderProbeJob& Job)
{
	const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
	return Runner->Run(Job);
}

TEST_CLASS(RenderProbeCompareRender, "RenderProbe.Integration.Compare")
{
	FRenderProbeTestOutputDir OutputDir;

	TEST_METHOD(SubjectsWithTheSameMaterialAreSimilar)
	{
		if (!RenderProbeTestRhi::CanRender(*TestRunner))
		{
			return;
		}
		const TStrongObjectPtr<UMaterial> Red(RenderProbeTestMaterials::CreateConstantColorMaterial(FLinearColor::Red));

		const FRenderProbeResult Result = RunJob(MakeCompareJob(OutputDir.Get(), *Red, *Red));

		ASSERT_THAT(AreEqual(1, Result.Compare.Num()));
		const FRenderProbeCompareResult& Compare = Result.Compare[0];
		ASSERT_THAT(AreEqual(FString(CameraName), Compare.Camera));
		ASSERT_THAT(IsTrue(Compare.Similar.IsSet() && Compare.Similar.GetValue()));
		ASSERT_THAT(IsTrue(Result.bOk));
	}

	TEST_METHOD(SubjectsWithDifferentMaterialsDifferOnTheSubject)
	{
		if (!RenderProbeTestRhi::CanRender(*TestRunner))
		{
			return;
		}
		const TStrongObjectPtr<UMaterial> Red(RenderProbeTestMaterials::CreateConstantColorMaterial(FLinearColor::Red));
		const TStrongObjectPtr<UMaterial> Green(RenderProbeTestMaterials::CreateConstantColorMaterial(FLinearColor::Green));

		const FRenderProbeResult Result = RunJob(MakeCompareJob(OutputDir.Get(), *Red, *Green));

		ASSERT_THAT(IsFalse(Result.bOk));
		ASSERT_THAT(AreEqual(1, Result.Compare.Num()));
		const FRenderProbeCompareResult& Compare = Result.Compare[0];
		ASSERT_THAT(IsTrue(Compare.Similar.IsSet() && !Compare.Similar.GetValue()));
		ASSERT_THAT(IsTrue(Compare.DiffBounds.IsSet()));
		const FIntRect& Bounds = Compare.DiffBounds.GetValue();
		const FIntRect WholeImage(0, 0, SmallResolutionX, SmallResolutionY);
		ASSERT_THAT(IsTrue(WholeImage.Contains(Bounds)));
		ASSERT_THAT(IsTrue(Bounds != WholeImage));
		ASSERT_THAT(IsTrue(Bounds.Contains(ImageCenter)));
		ASSERT_THAT(IsTrue(IFileManager::Get().FileExists(*Compare.DeltaImage)));
	}
};

}

#endif
