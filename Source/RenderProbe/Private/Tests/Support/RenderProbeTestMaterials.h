// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class UMaterial;
class UMaterialInstanceConstant;

namespace RenderProbeTestMaterials
{
	UMaterial* CreateStaticSwitchMaterial(FName SwitchName);

	UMaterialInstanceConstant* CreateInstanceWithStaticSwitch(UMaterial* Parent, FName SwitchName, bool bValue);

	UMaterial* CreateMaterialMissingTexture();

	UMaterial* CreateTimeColorMaterial();

	UMaterial* CreateSkyAtmosphereLightDirectionMaterial();
}

#endif
