// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

#include "UObject/StrongObjectPtr.h"

class UMaterial;
class UMaterialInstanceConstant;

namespace RenderProbeTestMaterials
{
	TStrongObjectPtr<UMaterial> CreateStaticSwitchMaterial(FName SwitchName);

	TStrongObjectPtr<UMaterialInstanceConstant> CreateInstanceWithStaticSwitch(UMaterial* Parent, FName SwitchName, bool bValue);

	TStrongObjectPtr<UMaterial> CreateMaterialMissingTexture();

	TStrongObjectPtr<UMaterial> CreateConstantColorMaterial(const FLinearColor& Color);

	TStrongObjectPtr<UMaterial> CreateTimeColorMaterial();

	TStrongObjectPtr<UMaterial> CreateSkyAtmosphereLightDirectionMaterial();
}

#endif
