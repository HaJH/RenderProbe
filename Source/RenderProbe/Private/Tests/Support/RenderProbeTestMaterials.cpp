// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Tests/Support/RenderProbeTestMaterials.h"

#if WITH_AUTOMATION_TESTS

#include "Engine/EngineTypes.h"
#include "MaterialEditingLibrary.h"
#include "Math/Color.h"
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
	static constexpr const TCHAR* ConstantColorMaterialName = TEXT("M_RenderProbeTestConstantColor");
	static constexpr const TCHAR* SkyAtmosphereLightDirectionClassPath = TEXT("/Script/Engine.MaterialExpressionSkyAtmosphereLightDirection");
	static constexpr float TimeColorOffset = 0.25f;
	static constexpr float SignedToUnitScale = 0.5f;
	static constexpr float SignedToUnitBias = 0.5f;

	template <typename ObjectType>
	static TStrongObjectPtr<ObjectType> NewTransient(const TCHAR* BaseName)
	{
		UPackage* Package = GetTransientPackage();
		const FName Name = MakeUniqueObjectName(Package, ObjectType::StaticClass(), FName(BaseName));
		return TStrongObjectPtr<ObjectType>(NewObject<ObjectType>(Package, Name, RF_Transient));
	}

	template <typename ExpressionType>
	static ExpressionType* AddExpression(UMaterial* Material)
	{
		return CastChecked<ExpressionType>(UMaterialEditingLibrary::CreateMaterialExpression(Material, ExpressionType::StaticClass()));
	}

	TStrongObjectPtr<UMaterial> CreateStaticSwitchMaterial(const FName SwitchName)
	{
		TStrongObjectPtr<UMaterial> Material = NewTransient<UMaterial>(TEXT("M_RenderProbeTestSwitch"));

		UMaterialExpressionStaticSwitchParameter* Switch = AddExpression<UMaterialExpressionStaticSwitchParameter>(Material.Get());
		Switch->ParameterName = SwitchName;
		Switch->DefaultValue = false;

		UMaterialExpressionTime* Time = AddExpression<UMaterialExpressionTime>(Material.Get());
		UMaterialExpressionSine* Sine = AddExpression<UMaterialExpressionSine>(Material.Get());
		UMaterialExpressionConstant3Vector* Constant = AddExpression<UMaterialExpressionConstant3Vector>(Material.Get());

		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Time, FString(), Sine, FString()));
		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Sine, FString(), Switch, SwitchTrueInputName));
		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Constant, FString(), Switch, SwitchFalseInputName));
		verify(UMaterialEditingLibrary::ConnectMaterialProperty(Switch, FString(), MP_EmissiveColor));

		UMaterialEditingLibrary::RecompileMaterial(Material.Get());
		return Material;
	}

	TStrongObjectPtr<UMaterialInstanceConstant> CreateInstanceWithStaticSwitch(UMaterial* Parent, const FName SwitchName, const bool bValue)
	{
		TStrongObjectPtr<UMaterialInstanceConstant> Instance = NewTransient<UMaterialInstanceConstant>(TEXT("MI_RenderProbeTestSwitch"));
		UMaterialEditingLibrary::SetMaterialInstanceParent(Instance.Get(), Parent);
		if (!UMaterialEditingLibrary::SetMaterialInstanceStaticSwitchParameterValue(Instance.Get(), SwitchName, bValue))
		{
			return nullptr;
		}
		return Instance;
	}

	TStrongObjectPtr<UMaterial> CreateMaterialMissingTexture()
	{
		TStrongObjectPtr<UMaterial> Material = NewTransient<UMaterial>(TEXT("M_RenderProbeTestMissingTexture"));
		UMaterialExpressionTextureSample* Sample = AddExpression<UMaterialExpressionTextureSample>(Material.Get());
		verify(UMaterialEditingLibrary::ConnectMaterialProperty(Sample, FString(), MP_BaseColor));
		UMaterialEditingLibrary::RecompileMaterial(Material.Get());
		return Material;
	}

	static TStrongObjectPtr<UMaterial> NewUnlitMaterial(const TCHAR* BaseName)
	{
		TStrongObjectPtr<UMaterial> Material = NewTransient<UMaterial>(BaseName);
		Material->SetShadingModel(MSM_Unlit);
		return Material;
	}

	TStrongObjectPtr<UMaterial> CreateConstantColorMaterial(const FLinearColor& Color)
	{
		TStrongObjectPtr<UMaterial> Material = NewUnlitMaterial(ConstantColorMaterialName);

		UMaterialExpressionConstant3Vector* Constant = AddExpression<UMaterialExpressionConstant3Vector>(Material.Get());
		Constant->Constant = Color;
		verify(UMaterialEditingLibrary::ConnectMaterialProperty(Constant, FString(), MP_EmissiveColor));

		UMaterialEditingLibrary::RecompileMaterial(Material.Get());
		return Material;
	}

	TStrongObjectPtr<UMaterial> CreateTimeColorMaterial()
	{
		TStrongObjectPtr<UMaterial> Material = NewUnlitMaterial(TEXT("M_RenderProbeTestTimeColor"));

		UMaterialExpressionTime* Time = AddExpression<UMaterialExpressionTime>(Material.Get());
		UMaterialExpressionAdd* Offset = AddExpression<UMaterialExpressionAdd>(Material.Get());
		Offset->ConstB = TimeColorOffset;
		UMaterialExpressionFrac* Frac = AddExpression<UMaterialExpressionFrac>(Material.Get());

		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Time, FString(), Offset, BinaryInputAName));
		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Offset, FString(), Frac, FString()));
		verify(UMaterialEditingLibrary::ConnectMaterialProperty(Frac, FString(), MP_EmissiveColor));

		UMaterialEditingLibrary::RecompileMaterial(Material.Get());
		return Material;
	}

	TStrongObjectPtr<UMaterial> CreateSkyAtmosphereLightDirectionMaterial()
	{
		TStrongObjectPtr<UMaterial> Material = NewUnlitMaterial(TEXT("M_RenderProbeTestSkyLightDirection"));

		UClass* DirectionClass = FindObject<UClass>(nullptr, SkyAtmosphereLightDirectionClassPath);
		check(DirectionClass);
		UMaterialExpression* Direction = UMaterialEditingLibrary::CreateMaterialExpression(Material.Get(), DirectionClass);
		UMaterialExpressionMultiply* Scale = AddExpression<UMaterialExpressionMultiply>(Material.Get());
		Scale->ConstB = SignedToUnitScale;
		UMaterialExpressionAdd* Bias = AddExpression<UMaterialExpressionAdd>(Material.Get());
		Bias->ConstB = SignedToUnitBias;

		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Direction, FString(), Scale, BinaryInputAName));
		verify(UMaterialEditingLibrary::ConnectMaterialExpressions(Scale, FString(), Bias, BinaryInputAName));
		verify(UMaterialEditingLibrary::ConnectMaterialProperty(Bias, FString(), MP_EmissiveColor));

		UMaterialEditingLibrary::RecompileMaterial(Material.Get());
		return Material;
	}
}

#endif
