// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeJob.h"
#include "RenderProbeJson.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeJobJsonTests
{

static constexpr const TCHAR* SourceJobJson = TEXT(R"json({
	"profile": {
		"settings": {
			"rig": { "directionalLight": { "intensity": 4.5, "bAtmosphereSunLight": true } },
			"lumen": { "bEnabled": true, "convergenceCaptures": 3 }
		}
	},
	"outputDir": "C:/RenderProbeOut",
	"resolution": { "x": 512, "y": 256 },
	"waitTimeoutSeconds": 30,
	"cameras": [
		{ "name": "front", "mode": "FitBounds", "yaw": 15, "pitch": -5, "fov": 30 },
		{ "name": "top", "mode": "Explicit", "location": { "x": 0, "y": 0, "z": 500 }, "rotation": { "pitch": -90, "yaw": 0, "roll": 0 } }
	],
	"items": [
		{
			"name": "subjectA",
			"mesh": "/Engine/BasicShapes/Cube.Cube",
			"materials": { "0": "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial" },
			"transform": { "scale": { "x": 2, "y": 2, "z": 2 } },
			"bStats": true
		}
	],
	"compare": [ { "name": "selfCheck", "a": "subjectA", "b": "subjectA", "tolerance": "Zero" } ]
})json");

static constexpr double Tolerance = 1e-4;

TEST_CLASS(RenderProbeJobJson, "RenderProbe.Integration.Job")
{
	TEST_METHOD(RoundTripsThroughJsonString)
	{
		FRenderProbeJob Parsed;
		FString Error;
		ASSERT_THAT(IsTrue(RenderProbeJson::JobFromJsonString(SourceJobJson, Parsed, Error)));

		const FString Exported = RenderProbeJson::JobToJsonString(Parsed);
		ASSERT_THAT(IsTrue(Exported.Contains(TEXT("\"outputDir\""), ESearchCase::CaseSensitive)));

		FRenderProbeJob RoundTripped;
		ASSERT_THAT(IsTrue(RenderProbeJson::JobFromJsonString(Exported, RoundTripped, Error)));

		ASSERT_THAT(IsTrue(RoundTripped.Profile.Asset.IsEmpty()));
		ASSERT_THAT(IsTrue(RoundTripped.Profile.Settings.IsSet()));
		const FRenderProbeProfileSettings& Settings = RoundTripped.Profile.Settings.GetValue();
		ASSERT_THAT(IsNear(4.5f, Settings.Rig.DirectionalLight.Intensity, Tolerance));
		ASSERT_THAT(IsTrue(Settings.Rig.DirectionalLight.bAtmosphereSunLight));
		ASSERT_THAT(IsTrue(Settings.Lumen.bEnabled));
		ASSERT_THAT(AreEqual(3, Settings.Lumen.ConvergenceCaptures));

		ASSERT_THAT(AreEqual(FString(TEXT("C:/RenderProbeOut")), RoundTripped.OutputDir));
		ASSERT_THAT(IsTrue(RoundTripped.Resolution == FIntPoint(512, 256)));
		ASSERT_THAT(IsNear(30.0, RoundTripped.WaitTimeoutSeconds, Tolerance));

		ASSERT_THAT(AreEqual(2, RoundTripped.Cameras.Num()));
		ASSERT_THAT(AreEqual(FString(TEXT("front")), RoundTripped.Cameras[0].Name));
		ASSERT_THAT(IsTrue(RoundTripped.Cameras[0].Mode == ERenderProbeCameraMode::FitBounds));
		ASSERT_THAT(IsNear(15.f, RoundTripped.Cameras[0].Yaw, Tolerance));
		ASSERT_THAT(IsTrue(RoundTripped.Cameras[1].Mode == ERenderProbeCameraMode::Explicit));
		ASSERT_THAT(IsNear(500.0, RoundTripped.Cameras[1].Location.Z, Tolerance));
		ASSERT_THAT(IsNear(-90.0, RoundTripped.Cameras[1].Rotation.Pitch, Tolerance));

		ASSERT_THAT(AreEqual(1, RoundTripped.Items.Num()));
		const FRenderProbeItem& Item = RoundTripped.Items[0];
		ASSERT_THAT(AreEqual(FString(TEXT("subjectA")), Item.Name));
		ASSERT_THAT(AreEqual(FString(TEXT("/Engine/BasicShapes/Cube.Cube")), Item.Mesh));
		const FString* SlotMaterial = Item.Materials.Find(TEXT("0"));
		ASSERT_THAT(IsNotNull(SlotMaterial));
		ASSERT_THAT(AreEqual(FString(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")), *SlotMaterial));
		ASSERT_THAT(IsNear(2.0, Item.Transform.Scale.X, Tolerance));
		ASSERT_THAT(IsTrue(Item.bStats));

		ASSERT_THAT(AreEqual(1, RoundTripped.Compare.Num()));
		ASSERT_THAT(AreEqual(FString(TEXT("subjectA")), RoundTripped.Compare[0].A));
		ASSERT_THAT(AreEqual(FString(TEXT("subjectA")), RoundTripped.Compare[0].B));
		ASSERT_THAT(IsTrue(RoundTripped.Compare[0].Tolerance == ERenderProbeTolerance::Zero));
	}
};

}

#endif
