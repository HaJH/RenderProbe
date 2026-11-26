// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Engine/StaticMesh.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Math/Color.h"
#include "Math/Rotator.h"
#include "Math/Vector.h"
#include "RenderProbeJob.h"
#include "RenderProbeProfileSettings.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "Tests/Support/RenderProbeTestOutputDir.h"
#include "Tests/Support/RenderProbeTestRhi.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeSkyLightTests
{

static constexpr const TCHAR* CubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
static constexpr const TCHAR* PlaneMeshPath = TEXT("/Engine/BasicShapes/Plane.Plane");
static constexpr const TCHAR* SubjectName = TEXT("cube");
static constexpr const TCHAR* CameraName = TEXT("front");
static constexpr int32 SmallResolutionX = 64;
static constexpr int32 SmallResolutionY = 48;
static constexpr float DimSkyLightIntensity = 1.f;
static constexpr float BrightSkyLightIntensity = 8.f;
/** The sun sits just above the horizon behind the cube, so the face the camera sees gets no direct light. */
static constexpr float SunPitch = -10.f;
static constexpr float SunYaw = 180.f;
/** The floor touches the bottom of the cube, so its distance field lies right under the face the camera sees. */
static constexpr double FloorHeight = -50.0;
static constexpr double FloorScale = 20.0;
static constexpr int32 FloorRunCount = 2;
static constexpr double MinBrightToDimRatio = 1.1;

enum class EFloor : uint8
{
	None,
	Plane,
};

static FRenderProbeJob MakeBackLitCubeJob(const FString& OutputDir, const float SkyLightIntensity, const EFloor Floor)
{
	FRenderProbeJob Job;
	Job.OutputDir = OutputDir;
	Job.Resolution = FIntPoint(SmallResolutionX, SmallResolutionY);

	FRenderProbeRigSettings& Rig = Job.Profile.Settings.Emplace().Rig;
	Rig.SkyAtmosphere.bEnabled = true;
	Rig.DirectionalLight.bAtmosphereSunLight = true;
	Rig.DirectionalLight.Rotation = FRotator(SunPitch, SunYaw, 0.f);
	Rig.SkyLight.bEnabled = true;
	Rig.SkyLight.Intensity = SkyLightIntensity;
	if (Floor == EFloor::Plane)
	{
		Rig.Floor.bEnabled = true;
		Rig.Floor.Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(PlaneMeshPath));
		Rig.Floor.Transform.Location = FVector(0.0, 0.0, FloorHeight);
		Rig.Floor.Transform.Scale = FVector(FloorScale, FloorScale, 1.0);
	}

	FRenderProbeCamera& Camera = Job.Cameras.AddDefaulted_GetRef();
	Camera.Name = CameraName;

	FRenderProbeItem& Item = Job.Items.AddDefaulted_GetRef();
	Item.Name = SubjectName;
	Item.Mesh = CubeMeshPath;
	return Job;
}

static bool LoadCapture(const FString& OutputDir, const float SkyLightIntensity, const EFloor Floor, FImage& OutImage)
{
	const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
	const FRenderProbeResult Result = Runner->Run(MakeBackLitCubeJob(OutputDir, SkyLightIntensity, Floor));
	const FString* CapturePath = Result.Items.Num() == 1 ? Result.Items[0].Images.Find(CameraName) : nullptr;
	if (!CapturePath || !FImageUtils::LoadImage(**CapturePath, OutImage))
	{
		return false;
	}
	OutImage.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
	return true;
}

static int64 SumColorChannels(const FImage& Image)
{
	int64 Sum = 0;
	for (const FColor& Pixel : Image.AsBGRA8())
	{
		Sum += Pixel.R + Pixel.G + Pixel.B;
	}
	return Sum;
}

TEST_CLASS(RenderProbeSkyLight, "RenderProbe.Integration.Capture")
{
	FRenderProbeTestOutputDir DimOutputDir;
	FRenderProbeTestOutputDir BrightOutputDir;

	TEST_METHOD(CapturedSceneSkyLightBrightensFaceOutOfDirectLight)
	{
		if (!RenderProbeTestRhi::CanRender(*TestRunner))
		{
			return;
		}

		FImage DimImage;
		FImage BrightImage;
		ASSERT_THAT(IsTrue(LoadCapture(DimOutputDir.Get(), DimSkyLightIntensity, EFloor::None, DimImage)));
		ASSERT_THAT(IsTrue(LoadCapture(BrightOutputDir.Get(), BrightSkyLightIntensity, EFloor::None, BrightImage)));

		ASSERT_THAT(IsTrue(SumColorChannels(BrightImage) > SumColorChannels(DimImage)));
	}

	TEST_METHOD(CapturedSceneSkyLightBrightensFaceAboveDistanceFieldFloorOnEveryRun)
	{
		if (!RenderProbeTestRhi::CanRender(*TestRunner))
		{
			return;
		}

		for (int32 RunIndex = 0; RunIndex < FloorRunCount; ++RunIndex)
		{
			FImage DimImage;
			FImage BrightImage;
			ASSERT_THAT(IsTrue(LoadCapture(DimOutputDir.Get(), DimSkyLightIntensity, EFloor::Plane, DimImage)));
			ASSERT_THAT(IsTrue(LoadCapture(BrightOutputDir.Get(), BrightSkyLightIntensity, EFloor::Plane, BrightImage)));

			ASSERT_THAT(IsTrue(static_cast<double>(SumColorChannels(BrightImage)) > static_cast<double>(SumColorChannels(DimImage)) * MinBrightToDimRatio));
		}
	}
};

}

#endif
