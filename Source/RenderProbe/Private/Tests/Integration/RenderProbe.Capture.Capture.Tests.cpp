// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Algo/AnyOf.h"
#include "HAL/FileManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Materials/Material.h"
#include "Math/Color.h"
#include "RenderProbeDiagnostic.h"
#include "RenderProbeJob.h"
#include "RenderProbeOutputPaths.h"
#include "RenderProbeResult.h"
#include "RenderProbeRunner.h"
#include "Tests/Support/RenderProbeTestMaterials.h"
#include "Tests/Support/RenderProbeTestOutputDir.h"
#include "Tests/Support/RenderProbeTestRhi.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeCaptureTests
{

static constexpr const TCHAR* CubeMeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
static constexpr const TCHAR* SubjectName = TEXT("cube");
static constexpr const TCHAR* CameraName = TEXT("front");
static constexpr const TCHAR* FirstSlotKey = TEXT("0");
static constexpr const TCHAR* WritePngStep = TEXT("writing the PNG");
static constexpr const TCHAR* MaterialCompileFailureLog = TEXT("Failed to compile Material");
static constexpr const TCHAR* MissingTextureLog = TEXT("Missing input texture");
static constexpr int32 IgnoreAnyOccurrences = -1;
static constexpr int32 SmallResolutionX = 64;
static constexpr int32 SmallResolutionY = 48;
static constexpr float CameraPitch = -20.f;
static constexpr float LightPitch = -30.f;
static constexpr float FirstLightYaw = 0.f;
static constexpr float SecondLightYaw = 90.f;

static FRenderProbeJob MakeCubeJob(const FString& OutputDir, const UMaterialInterface* Material)
{
	FRenderProbeJob Job;
	Job.Profile.Settings.Emplace();
	Job.OutputDir = OutputDir;
	Job.Resolution = FIntPoint(SmallResolutionX, SmallResolutionY);

	FRenderProbeCamera& Camera = Job.Cameras.AddDefaulted_GetRef();
	Camera.Name = CameraName;
	Camera.Pitch = CameraPitch;

	FRenderProbeItem& Item = Job.Items.AddDefaulted_GetRef();
	Item.Name = SubjectName;
	Item.Mesh = CubeMeshPath;
	if (Material)
	{
		Item.Materials.Add(FirstSlotKey, Material->GetPathName());
	}
	return Job;
}

static FRenderProbeResult RunJob(const FRenderProbeJob& Job)
{
	const TStrongObjectPtr<URenderProbeRunner> Runner(NewObject<URenderProbeRunner>());
	return Runner->Run(Job);
}

static const FString* FindCapture(const FRenderProbeResult& Result)
{
	return Result.Items.Num() == 1 ? Result.Items[0].Images.Find(CameraName) : nullptr;
}

static bool HaveSamePixels(const FImage& A, const FImage& B)
{
	return A.SizeX == B.SizeX && A.SizeY == B.SizeY && A.Format == B.Format && A.RawData == B.RawData;
}

TEST_CLASS(RenderProbeCapture, "RenderProbe.Integration.Capture")
{
	FRenderProbeTestOutputDir FirstOutputDir;
	FRenderProbeTestOutputDir SecondOutputDir;

	TEST_METHOD(WritesPngAtJobResolution)
	{
		if (!RenderProbeTestRhi::CanRender(*TestRunner))
		{
			return;
		}

		const TStrongObjectPtr<UMaterial> White(RenderProbeTestMaterials::CreateConstantColorMaterial(FLinearColor::White));

		const FRenderProbeResult Result = RunJob(MakeCubeJob(FirstOutputDir.Get(), White.Get()));

		ASSERT_THAT(AreEqual(1, Result.Items.Num()));
		ASSERT_THAT(IsTrue(Result.Items[0].Diagnostics.IsEmpty()));
		const FString* CapturePath = FindCapture(Result);
		ASSERT_THAT(IsNotNull(CapturePath));
		FImage Image;
		ASSERT_THAT(IsTrue(FImageUtils::LoadImage(**CapturePath, Image)));
		ASSERT_THAT(AreEqual(SmallResolutionX, Image.SizeX));
		ASSERT_THAT(AreEqual(SmallResolutionY, Image.SizeY));
	}

	TEST_METHOD(UnwritableCapturePathReportsOutputFailedWithoutImage)
	{
		if (!RenderProbeTestRhi::CanRender(*TestRunner))
		{
			return;
		}
		const FString CapturePath = RenderProbeOutputPaths::CapturePath(FirstOutputDir.Get(), SubjectName, CameraName);
		ASSERT_THAT(IsTrue(IFileManager::Get().MakeDirectory(*CapturePath, true)));

		const FRenderProbeResult Result = RunJob(MakeCubeJob(FirstOutputDir.Get(), nullptr));

		ASSERT_THAT(IsFalse(Result.bOk));
		ASSERT_THAT(AreEqual(1, Result.Items.Num()));
		ASSERT_THAT(IsNull(FindCapture(Result)));
		const bool bReportedOutputFailed = Algo::AnyOf(Result.Items[0].Diagnostics, [&CapturePath](const FRenderProbeDiagnostic& Diagnostic)
		{
			return Diagnostic.Kind == ERenderProbeDiagnosticKind::OutputFailed
				&& Diagnostic.Detail.Contains(WritePngStep, ESearchCase::CaseSensitive)
				&& Diagnostic.Detail.Contains(CapturePath, ESearchCase::CaseSensitive);
		});
		ASSERT_THAT(IsTrue(bReportedOutputFailed));
	}

	TEST_METHOD(SameJobTwiceWritesSamePixelsWithTimeDrivenMaterial)
	{
		if (!RenderProbeTestRhi::CanRender(*TestRunner))
		{
			return;
		}
		const TStrongObjectPtr<UMaterial> TimeColor(RenderProbeTestMaterials::CreateTimeColorMaterial());

		const FRenderProbeResult First = RunJob(MakeCubeJob(FirstOutputDir.Get(), TimeColor.Get()));
		const FRenderProbeResult Second = RunJob(MakeCubeJob(SecondOutputDir.Get(), TimeColor.Get()));

		const FString* FirstPath = FindCapture(First);
		const FString* SecondPath = FindCapture(Second);
		ASSERT_THAT(IsNotNull(FirstPath));
		ASSERT_THAT(IsNotNull(SecondPath));
		FImage FirstImage;
		FImage SecondImage;
		ASSERT_THAT(IsTrue(FImageUtils::LoadImage(**FirstPath, FirstImage)));
		ASSERT_THAT(IsTrue(FImageUtils::LoadImage(**SecondPath, SecondImage)));
		ASSERT_THAT(IsTrue(HaveSamePixels(FirstImage, SecondImage)));
	}

	TEST_METHOD(SkyAtmosphereLightDirectionFollowsDirectionalLightRotation)
	{
		if (!RenderProbeTestRhi::CanRender(*TestRunner))
		{
			return;
		}
		const TStrongObjectPtr<UMaterial> LightDirection(RenderProbeTestMaterials::CreateSkyAtmosphereLightDirectionMaterial());

		FRenderProbeJob FirstJob = MakeCubeJob(FirstOutputDir.Get(), LightDirection.Get());
		FRenderProbeRigSettings& Rig = FirstJob.Profile.Settings->Rig;
		Rig.SkyAtmosphere.bEnabled = true;
		Rig.SkyLight.bEnabled = false;
		Rig.DirectionalLight.bAtmosphereSunLight = true;
		Rig.DirectionalLight.Rotation = FRotator(LightPitch, FirstLightYaw, 0.f);

		FRenderProbeJob SecondJob = FirstJob;
		SecondJob.OutputDir = SecondOutputDir.Get();
		SecondJob.Profile.Settings->Rig.DirectionalLight.Rotation = FRotator(LightPitch, SecondLightYaw, 0.f);

		const FRenderProbeResult First = RunJob(FirstJob);
		const FRenderProbeResult Second = RunJob(SecondJob);

		const FString* FirstPath = FindCapture(First);
		const FString* SecondPath = FindCapture(Second);
		ASSERT_THAT(IsNotNull(FirstPath));
		ASSERT_THAT(IsNotNull(SecondPath));
		FImage FirstImage;
		FImage SecondImage;
		ASSERT_THAT(IsTrue(FImageUtils::LoadImage(**FirstPath, FirstImage)));
		ASSERT_THAT(IsTrue(FImageUtils::LoadImage(**SecondPath, SecondImage)));
		ASSERT_THAT(IsFalse(HaveSamePixels(FirstImage, SecondImage)));
	}

	TEST_METHOD(SlotFallingBackToEngineDefaultReportsDefaultMaterial)
	{
		if (!RenderProbeTestRhi::CanRender(*TestRunner))
		{
			return;
		}
		TestRunner->AddExpectedMessagePlain(MaterialCompileFailureLog, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, IgnoreAnyOccurrences);
		TestRunner->AddExpectedMessagePlain(MissingTextureLog, ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, IgnoreAnyOccurrences);
		const TStrongObjectPtr<UMaterial> Broken(RenderProbeTestMaterials::CreateMaterialMissingTexture());

		const FRenderProbeResult Result = RunJob(MakeCubeJob(FirstOutputDir.Get(), Broken.Get()));

		ASSERT_THAT(IsFalse(Result.bOk));
		ASSERT_THAT(AreEqual(1, Result.Items.Num()));
		const bool bReportedDefaultMaterial = Algo::AnyOf(Result.Items[0].Diagnostics, [](const FRenderProbeDiagnostic& Diagnostic)
		{
			return Diagnostic.Kind == ERenderProbeDiagnosticKind::DefaultMaterial && Diagnostic.Slot == FirstSlotKey;
		});
		ASSERT_THAT(IsTrue(bReportedDefaultMaterial));
	}
};

}

#endif
