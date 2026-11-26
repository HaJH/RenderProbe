// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "ImageCore.h"
#include "ImageUtils.h"
#include "Math/Color.h"
#include "Math/Rotator.h"
#include "RenderProbeJob.h"
#include "RenderProbeProfileSettings.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "Tests/Support/RenderProbeTestOutputDir.h"
#include "Tests/Support/RenderProbeTestRhi.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeSkyLightTests
{

static constexpr const TCHAR* CubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
static constexpr const TCHAR* SubjectName = TEXT("cube");
static constexpr const TCHAR* CameraName = TEXT("front");
static constexpr int32 SmallResolutionX = 64;
static constexpr int32 SmallResolutionY = 48;
static constexpr float DimSkyLightIntensity = 1.f;
static constexpr float BrightSkyLightIntensity = 8.f;
/** The sun sits just above the horizon behind the cube, so the face the camera sees gets no direct light. */
static constexpr float SunPitch = -10.f;
static constexpr float SunYaw = 180.f;

static FRenderProbeJob MakeBackLitCubeJob(const FString& OutputDir, const float SkyLightIntensity)
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

	FRenderProbeCamera& Camera = Job.Cameras.AddDefaulted_GetRef();
	Camera.Name = CameraName;

	FRenderProbeItem& Item = Job.Items.AddDefaulted_GetRef();
	Item.Name = SubjectName;
	Item.Mesh = CubeMeshPath;
	return Job;
}

static bool LoadCapture(const FString& OutputDir, const float SkyLightIntensity, FImage& OutImage)
{
	const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
	const FRenderProbeResult Result = Runner->Run(MakeBackLitCubeJob(OutputDir, SkyLightIntensity));
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
		ASSERT_THAT(IsTrue(LoadCapture(DimOutputDir.Get(), DimSkyLightIntensity, DimImage)));
		ASSERT_THAT(IsTrue(LoadCapture(BrightOutputDir.Get(), BrightSkyLightIntensity, BrightImage)));

		ASSERT_THAT(IsTrue(SumColorChannels(BrightImage) > SumColorChannels(DimImage)));
	}
};

}

#endif
