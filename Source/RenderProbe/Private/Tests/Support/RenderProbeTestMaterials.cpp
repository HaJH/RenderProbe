// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Tests/Support/RenderProbeTestMaterials.h"

#if WITH_AUTOMATION_TESTS

#include "Engine/EngineTypes.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialInstanceConstant.h"
#include "SceneTypes.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace RenderProbeTestMaterials
{
	static constexpr const TCHAR* SwitchTrueInputName = TEXT("True");
	static constexpr const TCHAR* SwitchFalseInputName = TEXT("False");
	static constexpr const TCHAR* BinaryInputAName = TEXT("A");
	static constexpr const TCHAR* SkyAtmosphereLightDirectionClassPath = TEXT("/Script/Engine.MaterialExpressionSkyAtmosphereLightDirection");
	static constexpr float TimeColorOffset = 0.25f;
	static constexpr float SignedToUnitScale = 0.5f;
	static constexpr float SignedToUnitBias = 0.5f;

	template <typename ObjectType>
	static ObjectType* NewTransient(const TCHAR* BaseName)
	{
		UPackage* Package = GetTransientPackage();
		const FName Name = MakeUniqueObjectName(Package, ObjectType::StaticClass(), FName(BaseName));
		return NewObject<ObjectType>(Package, Name, RF_Transient);
	}

	template <typename ExpressionType>
	static ExpressionType* AddExpression(UMaterial* Material)
	{
		return CastChecked<ExpressionType>(UMaterialEditingLibrary::CreateMaterialExpression(Material, ExpressionType::StaticClass()));
	}

	UMaterial* CreateStaticSwitchMaterial(const FName SwitchName)
	{
		UMaterial* Material = NewTransient<UMaterial>(TEXT("M_RenderProbeTestSwitch"));

		UMaterialExpressionStaticSwitchParameter* Switch = AddExpression<UMaterialExpressionStaticSwitchParameter>(Material);
		Switch->ParameterName = SwitchName;
		Switch->DefaultValue = false;

		UMaterialExpressionTime* Time = AddExpression<UMaterialExpressionTime>(Material);
		UMaterialExpressionSine* Sine = AddExpression<UMaterialExpressionSine>(Material);
		UMaterialExpressionConstant3Vector* Constant = AddExpression<UMaterialExpressionConstant3Vector>(Material);

		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Time, FString(), Sine, FString()));
		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Sine, FString(), Switch, SwitchTrueInputName));
		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Constant, FString(), Switch, SwitchFalseInputName));
		verify(UMaterialEditingLibrary::ConnectMaterialProperty(Switch, FString(), MP_EmissiveColor));

		UMaterialEditingLibrary::RecompileMaterial(Material);
		return Material;
	}

	UMaterialInstanceConstant* CreateInstanceWithStaticSwitch(UMaterial* Parent, const FName SwitchName, const bool bValue)
	{
		UMaterialInstanceConstant* Instance = NewTransient<UMaterialInstanceConstant>(TEXT("MI_RenderProbeTestSwitch"));
		UMaterialEditingLibrary::SetMaterialInstanceParent(Instance, Parent);
		if (!UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(Instance, SwitchName, bValue))
		{
			return nullptr;
		}
		return Instance;
	}

	UMaterial* CreateMaterialMissingTexture()
	{
		UMaterial* Material = NewTransient<UMaterial>(TEXT("M_RenderProbeTestMissingTexture"));
		UMaterialExpressionTextureSample* Sample = AddExpression<UMaterialExpressionTextureSample>(Material);
		verify(UMaterialEditingLibrary::ConnectMaterialProperty(Sample, FString(), MP_BaseColor));
		UMaterialEditingLibrary::RecompileMaterial(Material);
		return Material;
	}

	static UMaterial* NewUnlitMaterial(const TCHAR* BaseName)
	{
		UMaterial* Material = NewTransient<UMaterial>(BaseName);
		Material->SetShadingModel(MSM_Unlit);
		return Material;
	}

	UMaterial* CreateTimeColorMaterial()
	{
		UMaterial* Material = NewUnlitMaterial(TEXT("M_RenderProbeTestTimeColor"));

		UMaterialExpressionTime* Time = AddExpression<UMaterialExpressionTime>(Material);
		UMaterialExpressionAdd* Offset = AddExpression<UMaterialExpressionAdd>(Material);
		Offset->ConstB = TimeColorOffset;
		UMaterialExpressionFrac* Frac = AddExpression<UMaterialExpressionFrac>(Material);

		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Time, FString(), Offset, BinaryInputAName));
		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Offset, FString(), Frac, FString()));
		verify(UMaterialEditingLibrary::ConnectMaterialProperty(Frac, FString(), MP_EmissiveColor));

		UMaterialEditingLibrary::RecompileMaterial(Material);
		return Material;
	}

	UMaterial* CreateSkyAtmosphereLightDirectionMaterial()
	{
		UMaterial* Material = NewUnlitMaterial(TEXT("M_RenderProbeTestSkyLightDirection"));

		UClass* DirectionClass = FindObject<UClass>(nullptr, SkyAtmosphereLightDirectionClassPath);
		check(DirectionClass);
		UMaterialExpression* Direction = UMaterialEditingLibrary::CreateMaterialExpression(Material, DirectionClass);
		UMaterialExpressionMultiply* Scale = AddExpression<UMaterialExpressionMultiply>(Material);
		Scale->ConstB = SignedToUnitScale;
		UMaterialExpressionAdd* Bias = AddExpression<UMaterialExpressionAdd>(Material);
		Bias->ConstB = SignedToUnitBias;

		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Direction, FString(), Scale, BinaryInputAName));
		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Scale, FString(), Bias, BinaryInputAName));
		verify(UMaterialEditingLibrary::ConnectMaterialProperty(Bias, FString(), MP_EmissiveColor));

		UMaterialEditingLibrary::RecompileMaterial(Material);
		return Material;
	}
}

#endif
