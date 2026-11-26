// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeJson.h"

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "JsonObjectConverter.h"
#include "JsonObjectStructInterface.h"
#include "JsonObjectWrapper.h"
#include "Policies/CondensedJsonPrintPolicy.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/Class.h"
#include "UObject/EnumProperty.h"
#include "UObject/PropertyOptional.h"
#include "UObject/UnrealType.h"

namespace RenderProbeJsonSchemaLocal
{
	static constexpr const TCHAR* ToolTipKey = TEXT("ToolTip");
	static constexpr const TCHAR* TypeField = TEXT("type");
	static constexpr const TCHAR* PropertiesField = TEXT("properties");
	static constexpr const TCHAR* ItemsField = TEXT("items");
	static constexpr const TCHAR* AdditionalPropertiesField = TEXT("additionalProperties");
	static constexpr const TCHAR* EnumField = TEXT("enum");
	static constexpr const TCHAR* DescriptionField = TEXT("description");
	static constexpr const TCHAR* ObjectType = TEXT("object");
	static constexpr const TCHAR* ArrayType = TEXT("array");
	static constexpr const TCHAR* StringType = TEXT("string");
	static constexpr const TCHAR* NumberType = TEXT("number");
	static constexpr const TCHAR* IntegerType = TEXT("integer");
	static constexpr const TCHAR* BooleanType = TEXT("boolean");
	static constexpr const TCHAR* JobField = TEXT("job");
	static constexpr const TCHAR* ResultField = TEXT("result");
	static constexpr const TCHAR* DescriptionSeparator = TEXT("\n");
	static constexpr TCHAR EnumValueLineFormat[] = TEXT("- %s: %s");
	static constexpr TCHAR RecursiveStructFormat[] = TEXT("%s contains itself, so its schema cannot be written inline");

	/** Properties FJsonObjectConverter leaves out when it writes a structure with no skip flags given. */
	static constexpr EPropertyFlags ConverterSkipFlags = CPF_Deprecated | CPF_Transient;

	static TSharedRef<FJsonObject> MakePropertySchema(const FProperty& Property, TArray<const UStruct*>& StructStack);

	static TSharedRef<FJsonObject> MakeTyped(const TCHAR* Type)
	{
		TSharedRef<FJsonObject> Schema = MakeShared<FJsonObject>();
		Schema->SetStringField(TypeField, Type);
		return Schema;
	}

	static void SetDescription(FJsonObject& Schema, const FString& Description)
	{
		if (!Description.IsEmpty())
		{
			Schema.SetStringField(DescriptionField, Description);
		}
	}

	static FString GetDescription(const FJsonObject& Schema)
	{
		FString Description;
		Schema.TryGetStringField(DescriptionField, Description);
		return Description;
	}

	static FString JoinDescriptions(const FString& First, const FString& Second)
	{
		if (First.IsEmpty() || First == Second)
		{
			return Second;
		}
		if (Second.IsEmpty())
		{
			return First;
		}
		return First + DescriptionSeparator + Second;
	}

	static TSharedRef<FJsonObject> MakeEnumSchema(const UEnum& Enum)
	{
		TSharedRef<FJsonObject> Schema = MakeTyped(StringType);

		TArray<FString> DescriptionLines;
		const FString EnumToolTip = Enum.GetMetaData(ToolTipKey);
		if (!EnumToolTip.IsEmpty())
		{
			DescriptionLines.Add(EnumToolTip);
		}

		TArray<TSharedPtr<FJsonValue>> Names;
		const int32 ValueCount = Enum.ContainsExistingMax() ? Enum.NumEnums() - 1 : Enum.NumEnums();
		for (int32 Index = 0; Index < ValueCount; ++Index)
		{
			const FString Name = Enum.GetAuthoredNameStringByIndex(Index);
			Names.Add(MakeShared<FJsonValueString>(Name));
			DescriptionLines.Add(FString::Printf(EnumValueLineFormat, *Name, *Enum.GetMetaData(ToolTipKey, Index)));
		}

		Schema->SetArrayField(EnumField, Names);
		SetDescription(*Schema, FString::Join(DescriptionLines, DescriptionSeparator));
		return Schema;
	}

	static TSharedRef<FJsonObject> MakeStructSchema(const UStruct& Struct, TArray<const UStruct*>& StructStack)
	{
		checkf(!StructStack.Contains(&Struct), RecursiveStructFormat, *Struct.GetName());
		StructStack.Push(&Struct);

		TSharedRef<FJsonObject> Schema = MakeTyped(ObjectType);
		SetDescription(*Schema, Struct.GetMetaData(ToolTipKey));

		const TSharedRef<FJsonObject> Properties = MakeShared<FJsonObject>();
		for (TFieldIterator<FProperty> It(&Struct); It; ++It)
		{
			const FProperty& Property = **It;
			if (Property.HasAnyPropertyFlags(ConverterSkipFlags))
			{
				continue;
			}
			Properties->SetObjectField(FJsonObjectConverter::StandardizeCase(Property.GetAuthoredName()), MakePropertySchema(Property, StructStack));
		}
		Schema->SetObjectField(PropertiesField, Properties);

		StructStack.Pop();
		return Schema;
	}

	static TSharedRef<FJsonObject> MakeStructValueSchema(const UScriptStruct& Struct, TArray<const UStruct*>& StructStack)
	{
		const UScriptStruct::ICppStructOps* CppStructOps = Struct.GetCppStructOps();
		const bool bWrittenAsOpaqueObject = &Struct == FJsonObjectWrapper::StaticStruct() || FJsonObjectStructInterfaceRegistry::HasStructConverterRegistered(&Struct);
		const bool bWrittenAsText = !bWrittenAsOpaqueObject && CppStructOps && CppStructOps->HasExportTextItem();
		if (!bWrittenAsOpaqueObject && !bWrittenAsText)
		{
			return MakeStructSchema(Struct, StructStack);
		}

		TSharedRef<FJsonObject> Schema = MakeTyped(bWrittenAsText ? StringType : ObjectType);
		SetDescription(*Schema, Struct.GetMetaData(ToolTipKey));
		return Schema;
	}

	/** Mirrors the property type dispatch of FJsonObjectConverter when it writes a value. */
	static TSharedRef<FJsonObject> MakeValueSchema(const FProperty& Property, TArray<const UStruct*>& StructStack)
	{
		if (const FOptionalProperty* OptionalProperty = CastField<FOptionalProperty>(&Property))
		{
			return MakeValueSchema(*OptionalProperty->GetValueProperty(), StructStack);
		}
		if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(&Property))
		{
			return MakeEnumSchema(*EnumProperty->GetEnum());
		}
		if (const FNumericProperty* NumericProperty = CastField<FNumericProperty>(&Property))
		{
			if (const UEnum* Enum = NumericProperty->GetIntPropertyEnum())
			{
				return MakeEnumSchema(*Enum);
			}
			return MakeTyped(NumericProperty->IsFloatingPoint() ? NumberType : IntegerType);
		}
		if (CastField<FBoolProperty>(&Property))
		{
			return MakeTyped(BooleanType);
		}
		if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(&Property))
		{
			TSharedRef<FJsonObject> Schema = MakeTyped(ArrayType);
			Schema->SetObjectField(ItemsField, MakeValueSchema(*ArrayProperty->Inner, StructStack));
			return Schema;
		}
		if (const FSetProperty* SetProperty = CastField<FSetProperty>(&Property))
		{
			TSharedRef<FJsonObject> Schema = MakeTyped(ArrayType);
			Schema->SetObjectField(ItemsField, MakeValueSchema(*SetProperty->ElementProp, StructStack));
			return Schema;
		}
		if (const FMapProperty* MapProperty = CastField<FMapProperty>(&Property))
		{
			TSharedRef<FJsonObject> Schema = MakeTyped(ObjectType);
			Schema->SetObjectField(AdditionalPropertiesField, MakeValueSchema(*MapProperty->ValueProp, StructStack));
			return Schema;
		}
		if (const FStructProperty* StructProperty = CastField<FStructProperty>(&Property))
		{
			return MakeStructValueSchema(*StructProperty->Struct, StructStack);
		}
		if (CastField<FObjectProperty>(&Property) && Property.HasAnyPropertyFlags(CPF_PersistentInstance))
		{
			return MakeTyped(ObjectType);
		}
		return MakeTyped(StringType);
	}

	static TSharedRef<FJsonObject> MakePropertySchema(const FProperty& Property, TArray<const UStruct*>& StructStack)
	{
		const TSharedRef<FJsonObject> Value = MakeValueSchema(Property, StructStack);
		TSharedRef<FJsonObject> Schema = Value;
		if (Property.ArrayDim > 1)
		{
			Schema = MakeTyped(ArrayType);
			Schema->SetObjectField(ItemsField, Value);
		}
		SetDescription(*Schema, JoinDescriptions(Property.GetMetaData(ToolTipKey), GetDescription(*Value)));
		return Schema;
	}
}

namespace RenderProbeJson
{
	FString JobSchemaString()
	{
		TArray<const UStruct*> StructStack;
		const TSharedRef<FJsonObject> Schemas = MakeShared<FJsonObject>();
		Schemas->SetObjectField(RenderProbeJsonSchemaLocal::JobField, RenderProbeJsonSchemaLocal::MakeStructSchema(*FRenderProbeJob::StaticStruct(), StructStack));
		Schemas->SetObjectField(RenderProbeJsonSchemaLocal::ResultField, RenderProbeJsonSchemaLocal::MakeStructSchema(*FRenderProbeResult::StaticStruct(), StructStack));

		FString SchemaString;
		const TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&SchemaString);
		verify(FJsonSerializer::Serialize(Schemas, Writer));
		return SchemaString;
	}
}
