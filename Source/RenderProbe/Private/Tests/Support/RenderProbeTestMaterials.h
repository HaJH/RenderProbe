// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class UMaterial;
class UMaterialInstanceConstant;

namespace RenderProbeTestMaterials
{
	/** Transient material whose emissive is a time-driven sine when the switch is true and a constant when false. */
	UMaterial* CreateStaticSwitchMaterial(FName SwitchName);

	/** Transient instance of Parent with the static switch overridden; null when the switch cannot be set. */
	UMaterialInstanceConstant* CreateInstanceWithStaticSwitch(UMaterial* Parent, FName SwitchName, bool bValue);

	/** Transient material whose base color samples a texture sample node without a texture, so it fails to compile. */
	UMaterial* CreateMaterialMissingTexture();

	/** Transient unlit material whose emissive is frac(time + offset), so any passing of material time changes its color. */
	UMaterial* CreateTimeColorMaterial();

	/** Transient unlit material whose emissive is the first atmosphere light direction mapped from -1..1 to 0..1. */
	UMaterial* CreateSkyAtmosphereLightDirectionMaterial();
}

#endif
