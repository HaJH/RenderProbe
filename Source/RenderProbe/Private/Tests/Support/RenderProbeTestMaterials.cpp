// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Tests/Support/RenderProbeTestMaterials.h"

#if WITH_AUTOMATION_TESTS

#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionStaticSwitchParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialInstanceConstant.h"
#include "SceneTypes.h"
#include "UObject/Package.h"

namespace RenderProbeTestMaterials
{
	static constexpr const TCHAR* SwitchTrueInputName = TEXT("True");
	static constexpr const TCHAR* SwitchFalseInputName = TEXT("False");

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
}

#endif
