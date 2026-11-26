// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "JsonObjectConverter.h"
#include "Math/IntRect.h"
#include "RenderProbeDiagnostic.h"
#include "RenderProbeJob.h"
#include "RenderProbeJson.h"
#include "RenderProbeProfileSettings.h"
#include "RenderProbeResult.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Class.h"
#include "UObject/PropertyOptional.h"
#include "UObject/UnrealType.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeJobSchemaTests
{

static constexpr const TCHAR* ToolTipKey = TEXT("ToolTip");
static constexpr const TCHAR* JobField = TEXT("job");
static constexpr const TCHAR* ResultField = TEXT("result");
static constexpr const TCHAR* TypeField = TEXT("type");
static constexpr const TCHAR* PropertiesField = TEXT("properties");
static constexpr const TCHAR* ItemsField = TEXT("items");
static constexpr const TCHAR* AdditionalPropertiesField = TEXT("additionalProperties");
static constexpr const TCHAR* EnumField = TEXT("enum");
static constexpr const TCHAR* DescriptionField = TEXT("description");
static constexpr const TCHAR* ObjectType = TEXT("object");
static constexpr const TCHAR* DiagnosticsKey = TEXT("diagnostics");
static constexpr const TCHAR* KindKey = TEXT("kind");
static constexpr const TCHAR* PathSeparator = TEXT(".");
static constexpr const TCHAR* ElementSuffix = TEXT("[]");
static constexpr const TCHAR* ListSeparator = TEXT(", ");
static constexpr const TCHAR* SampleKey = TEXT("0");
static constexpr const TCHAR* SampleText = TEXT("sample");
static constexpr EPropertyFlags ConverterSkipFlags = CPF_Deprecated | CPF_Transient;

struct FSchemaGaps
{
	TArray<FString> MissingProperties;
	TArray<FString> MissingDescriptions;
};

static TSharedPtr<FJsonObject> ParseSchemas()
{
	TSharedPtr<FJsonObject> Schemas;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(RenderProbeJson::JobSchemaString());
	FJsonSerializer::Deserialize(Reader, Schemas);
	return Schemas;
}

static const FJsonObject* FindObject(const FJsonObject& Object, const TCHAR* Field)
{
	const TSharedPtr<FJsonObject>* Found = nullptr;
	return Object.TryGetObjectField(Field, Found) ? Found->Get() : nullptr;
}

static bool IsPluginStruct(const UStruct& Struct)
{
	return Struct.GetOutermost() == FRenderProbeJob::StaticStruct()->GetOutermost();
}

static void CheckStruct(const UStruct& Struct, const FJsonObject& Schema, const FString& Path, FSchemaGaps& OutGaps);

static void CheckNestedStructs(const FProperty& Property, const FJsonObject& Schema, const FString& Path, FSchemaGaps& OutGaps)
{
	if (const FOptionalProperty* OptionalProperty = CastField<FOptionalProperty>(&Property))
	{
		CheckNestedStructs(*OptionalProperty->GetValueProperty(), Schema, Path, OutGaps);
		return;
	}

	const FProperty* ElementProperty = nullptr;
	const TCHAR* ElementField = nullptr;
	if (const FArrayProperty* ArrayProperty = CastField<FArrayProperty>(&Property))
	{
		ElementProperty = ArrayProperty->Inner;
		ElementField = ItemsField;
	}
	else if (const FMapProperty* MapProperty = CastField<FMapProperty>(&Property))
	{
		ElementProperty = MapProperty->ValueProp;
		ElementField = AdditionalPropertiesField;
	}
	if (ElementProperty)
	{
		const FJsonObject* ElementSchema = FindObject(Schema, ElementField);
		if (!ElementSchema)
		{
			OutGaps.MissingProperties.Add(Path + ElementSuffix);
			return;
		}
		CheckNestedStructs(*ElementProperty, *ElementSchema, Path + ElementSuffix, OutGaps);
		return;
	}

	const FStructProperty* StructProperty = CastField<FStructProperty>(&Property);
	if (StructProperty && IsPluginStruct(*StructProperty->Struct))
	{
		CheckStruct(*StructProperty->Struct, Schema, Path, OutGaps);
	}
}

static void CheckStruct(const UStruct& Struct, const FJsonObject& Schema, const FString& Path, FSchemaGaps& OutGaps)
{
	const FJsonObject* Properties = FindObject(Schema, PropertiesField);
	for (TFieldIterator<FProperty> It(&Struct); It; ++It)
	{
		const FProperty& Property = **It;
		if (Property.HasAnyPropertyFlags(ConverterSkipFlags))
		{
			continue;
		}

		const FString Key = FJsonObjectConverter::StandardizeCase(Property.GetAuthoredName());
		const FString PropertyPath = Path + PathSeparator + Key;
		const FJsonObject* PropertySchema = Properties ? FindObject(*Properties, *Key) : nullptr;
		if (!PropertySchema)
		{
			OutGaps.MissingProperties.Add(PropertyPath);
			continue;
		}

		FString Description;
		if (!PropertySchema->TryGetStringField(DescriptionField, Description) || Description.IsEmpty())
		{
			OutGaps.MissingDescriptions.Add(PropertyPath);
		}
		CheckNestedStructs(Property, *PropertySchema, PropertyPath, OutGaps);
	}
}

static FSchemaGaps FindSchemaGaps(const FJsonObject& Schemas)
{
	FSchemaGaps Gaps;
	const FJsonObject* JobSchema = FindObject(Schemas, JobField);
	const FJsonObject* ResultSchema = FindObject(Schemas, ResultField);
	if (!JobSchema || !ResultSchema)
	{
		Gaps.MissingProperties.Add(!JobSchema ? JobField : ResultField);
		return Gaps;
	}
	CheckStruct(*FRenderProbeJob::StaticStruct(), *JobSchema, JobField, Gaps);
	CheckStruct(*FRenderProbeResult::StaticStruct(), *ResultSchema, ResultField, Gaps);
	return Gaps;
}

static void CompareKeys(const FJsonValue& Value, const FJsonObject& Schema, const FString& Path, TArray<FString>& OutMismatches);

static void CompareObjectKeys(const FJsonObject& Object, const FJsonObject& Schema, const FString& Path, TArray<FString>& OutMismatches)
{
	if (const FJsonObject* Properties = FindObject(Schema, PropertiesField))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Object.Values)
		{
			const FJsonObject* FieldSchema = FindObject(*Properties, *Field.Key);
			if (!FieldSchema)
			{
				OutMismatches.Add(Path + PathSeparator + Field.Key);
				continue;
			}
			CompareKeys(*Field.Value, *FieldSchema, Path + PathSeparator + Field.Key, OutMismatches);
		}
		for (const TPair<FString, TSharedPtr<FJsonValue>>& SchemaField : Properties->Values)
		{
			if (!Object.HasField(SchemaField.Key))
			{
				OutMismatches.Add(Path + PathSeparator + SchemaField.Key);
			}
		}
		return;
	}

	if (const FJsonObject* ValueSchema = FindObject(Schema, AdditionalPropertiesField))
	{
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Field : Object.Values)
		{
			CompareKeys(*Field.Value, *ValueSchema, Path + PathSeparator + Field.Key, OutMismatches);
		}
		return;
	}

	FString Type;
	if (!Schema.TryGetStringField(TypeField, Type) || Type != ObjectType)
	{
		OutMismatches.Add(Path);
	}
}

static void CompareKeys(const FJsonValue& Value, const FJsonObject& Schema, const FString& Path, TArray<FString>& OutMismatches)
{
	if (Value.Type == EJson::Object)
	{
		CompareObjectKeys(*Value.AsObject(), Schema, Path, OutMismatches);
	}
	else if (Value.Type == EJson::Array)
	{
		const FJsonObject* ItemSchema = FindObject(Schema, ItemsField);
		if (!ItemSchema)
		{
			OutMismatches.Add(Path);
			return;
		}
		for (const TSharedPtr<FJsonValue>& Element : Value.AsArray())
		{
			CompareKeys(*Element, *ItemSchema, Path + ElementSuffix, OutMismatches);
		}
	}
	else if (Schema.HasField(PropertiesField) || Schema.HasField(ItemsField) || Schema.HasField(AdditionalPropertiesField))
	{
		OutMismatches.Add(Path);
	}
}

static FRenderProbeJob MakeFullJob()
{
	FRenderProbeJob Job;
	Job.Profile.Asset = SampleText;
	Job.Profile.Settings.Emplace();
	Job.Profile.Settings->EnvironmentLevels.AddDefaulted();
	Job.Cameras.AddDefaulted();
	FRenderProbeItem& Item = Job.Items.AddDefaulted_GetRef();
	Item.Materials.Add(SampleKey, SampleText);
	Job.Compare.AddDefaulted();
	return Job;
}

static FRenderProbeResult MakeFullResult()
{
	FRenderProbeResult Result;
	FRenderProbeItemResult& Item = Result.Items.AddDefaulted_GetRef();
	Item.Images.Add(SampleKey, SampleText);
	Item.Stats.Add(SampleKey, FRenderProbeMaterialStats());
	Item.Diagnostics.AddDefaulted();
	FRenderProbeCompareResult& Compare = Result.Compare.AddDefaulted_GetRef();
	Compare.Similar = true;
	Compare.GlobalDifference = 0.0;
	Compare.MaxLocalDifference = 0.0;
	Compare.DiffBounds = FIntRect();
	Result.Diagnostics.AddDefaulted();
	return Result;
}

TEST_CLASS(RenderProbeJobSchema, "RenderProbe.Unit.Job")
{
	TSharedPtr<FJsonObject> Schemas;

	BEFORE_EACH()
	{
		Schemas = ParseSchemas();
		ASSERT_THAT(IsTrue(Schemas.IsValid()));
	}

	TEST_METHOD(ListsEveryJobAndResultProperty)
	{
		const FSchemaGaps Gaps = FindSchemaGaps(*Schemas);
		ASSERT_THAT(IsTrue(Gaps.MissingProperties.IsEmpty(), *FString::Join(Gaps.MissingProperties, ListSeparator)));
	}

	TEST_METHOD(DescribesEveryJobAndResultProperty)
	{
		const FSchemaGaps Gaps = FindSchemaGaps(*Schemas);
		ASSERT_THAT(IsTrue(Gaps.MissingDescriptions.IsEmpty(), *FString::Join(Gaps.MissingDescriptions, ListSeparator)));
	}

	TEST_METHOD(DescribesEveryDiagnosticKind)
	{
		const FJsonObject* ResultSchema = FindObject(*Schemas, ResultField);
		ASSERT_THAT(IsNotNull(ResultSchema));
		const FJsonObject* ResultProperties = FindObject(*ResultSchema, PropertiesField);
		ASSERT_THAT(IsNotNull(ResultProperties));
		const FJsonObject* DiagnosticsSchema = FindObject(*ResultProperties, DiagnosticsKey);
		ASSERT_THAT(IsNotNull(DiagnosticsSchema));
		const FJsonObject* DiagnosticSchema = FindObject(*DiagnosticsSchema, ItemsField);
		ASSERT_THAT(IsNotNull(DiagnosticSchema));
		const FJsonObject* DiagnosticProperties = FindObject(*DiagnosticSchema, PropertiesField);
		ASSERT_THAT(IsNotNull(DiagnosticProperties));
		const FJsonObject* KindSchema = FindObject(*DiagnosticProperties, KindKey);
		ASSERT_THAT(IsNotNull(KindSchema));

		FString KindDescription;
		ASSERT_THAT(IsTrue(KindSchema->TryGetStringField(DescriptionField, KindDescription)));
		TArray<FString> KindNames;
		ASSERT_THAT(IsTrue(KindSchema->TryGetStringArrayField(EnumField, KindNames)));

		const UEnum* KindEnum = StaticEnum<ERenderProbeDiagnosticKind>();
		const int32 KindCount = KindEnum->ContainsExistingMax() ? KindEnum->NumEnums() - 1 : KindEnum->NumEnums();
		ASSERT_THAT(AreEqual(KindCount, KindNames.Num()));
		for (int32 Index = 0; Index < KindCount; ++Index)
		{
			const FString Name = KindEnum->GetAuthoredNameStringByIndex(Index);
			const FString ToolTip = KindEnum->GetMetaData(ToolTipKey, Index);
			ASSERT_THAT(IsFalse(ToolTip.IsEmpty(), *Name));
			ASSERT_THAT(IsTrue(KindNames.Contains(Name), *Name));
			ASSERT_THAT(IsTrue(KindDescription.Contains(ToolTip, ESearchCase::CaseSensitive), *Name));
		}
	}

	TEST_METHOD(KeysMatchConverterOutput)
	{
		const FJsonObject* JobSchema = FindObject(*Schemas, JobField);
		const FJsonObject* ResultSchema = FindObject(*Schemas, ResultField);
		ASSERT_THAT(IsNotNull(JobSchema));
		ASSERT_THAT(IsNotNull(ResultSchema));

		const TSharedPtr<FJsonObject> JobJson = FJsonObjectConverter::UStructToJsonObject(MakeFullJob());
		const TSharedPtr<FJsonObject> ResultJson = FJsonObjectConverter::UStructToJsonObject(MakeFullResult());
		ASSERT_THAT(IsTrue(JobJson.IsValid()));
		ASSERT_THAT(IsTrue(ResultJson.IsValid()));

		TArray<FString> Mismatches;
		CompareObjectKeys(*JobJson, *JobSchema, JobField, Mismatches);
		CompareObjectKeys(*ResultJson, *ResultSchema, ResultField, Mismatches);
		ASSERT_THAT(IsTrue(Mismatches.IsEmpty(), *FString::Join(Mismatches, ListSeparator)));
	}
};

}

#endif
