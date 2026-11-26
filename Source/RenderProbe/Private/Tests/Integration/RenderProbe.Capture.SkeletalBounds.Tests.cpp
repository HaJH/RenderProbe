// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Math/Box.h"
#include "Math/BoxSphereBounds.h"
#include "Math/IntPoint.h"
#include "Math/Transform.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/SphereElem.h"
#include "RenderProbeProfileSettings.h"
#include "RenderProbeStage.h"
#include "RenderProbeTransform.h"
#include "Tests/Support/RenderProbeTestRhi.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeSkeletalBoundsTests
{

static constexpr const TCHAR* SkeletalCubePath = TEXT("/Engine/EngineMeshes/SkeletalCube.SkeletalCube");
static constexpr int32 RootBoneIndex = 0;
static constexpr float BodyRadius = 1.f;
static constexpr double BoundsTolerance = 0.01;
static constexpr int32 SmallResolutionX = 64;
static constexpr int32 SmallResolutionY = 48;

static UPhysicsAsset* MakeSmallBodyPhysicsAsset(const USkeletalMesh& Mesh)
{
	UPhysicsAsset* Physics = NewObject<UPhysicsAsset>(GetTransientPackage());
	USkeletalBodySetup* Body = NewObject<USkeletalBodySetup>(Physics);
	Body->BoneName = Mesh.GetRefSkeleton().GetBoneName(RootBoneIndex);
	Body->AggGeom.SphereElems.Add(FKSphereElem(BodyRadius));
	Physics->SkeletalBodySetups.Add(Body);
	Physics->UpdateBodySetupIndexMap();
	Physics->UpdateBoundsBodiesArray();
	return Physics;
}

TEST_CLASS(RenderProbeSkeletalBounds, "RenderProbe.Integration.Capture")
{
	TEST_METHOD(SkeletalSubjectUsesMeshBoundsInsteadOfPhysicsBodies)
	{
		if (!RenderProbeTestRhi::CanRender(*TestRunner))
		{
			return;
		}

		USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, SkeletalCubePath);
		ASSERT_THAT(IsNotNull(Mesh));

		FRenderProbeStage Stage(FRenderProbeProfileSettings(), FRenderProbeStageAssets(), FIntPoint(SmallResolutionX, SmallResolutionY));
		USkeletalMeshComponent* Subject = Cast<USkeletalMeshComponent>(Stage.PlaceSubject(*Mesh, {}, FRenderProbeTransform()));
		ASSERT_THAT(IsNotNull(Subject));

		const TStrongObjectPtr<UPhysicsAsset> Physics(MakeSmallBodyPhysicsAsset(*Mesh));
		Subject->SetPhysicsAsset(Physics.Get(), true);
		Subject->InvalidateCachedBounds();
		Subject->UpdateBounds();

		const FTransform& ComponentTransform = Subject->GetComponentTransform();
		const FBoxSphereBounds MeshBounds = Mesh->GetBounds().TransformBy(ComponentTransform);
		const FBox BodyBox = Physics->CalcAABB(Subject, ComponentTransform);
		ASSERT_THAT(IsTrue(BodyBox.IsValid != 0));
		ASSERT_THAT(IsTrue(BodyBox.GetExtent().GetMax() < MeshBounds.BoxExtent.GetMax()));

		const FBoxSphereBounds SubjectBounds = Subject->Bounds;
		Stage.RemoveSubject(*Subject);
		ASSERT_THAT(IsTrue(SubjectBounds.Origin.Equals(MeshBounds.Origin, BoundsTolerance)));
		ASSERT_THAT(IsTrue(SubjectBounds.BoxExtent.Equals(MeshBounds.BoxExtent, BoundsTolerance)));
		ASSERT_THAT(IsNear(MeshBounds.SphereRadius, SubjectBounds.SphereRadius, BoundsTolerance));
	}
};

}

#endif
