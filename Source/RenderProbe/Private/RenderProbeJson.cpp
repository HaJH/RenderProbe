// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeJson.h"

#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "RenderProbeJob.h"
#include "RenderProbeResult.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace RenderProbeJson
{
	bool JobFromJsonString(const FString& JsonString, FRenderProbeJob& OutJob, FString& OutError)
	{
		TSharedPtr<FJsonObject> JsonObject;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
		if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
		{
			OutError = FString::Printf(TEXT("Job is not a JSON object: %s"), *Reader->GetErrorMessage());
			return false;
		}

		FRenderProbeJob Job;
		FText FailReason;
		constexpr bool bStrictMode = false;
		if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), FRenderProbeJob::StaticStruct(), &Job, 0, 0, bStrictMode, &FailReason))
		{
			OutError = FString::Printf(TEXT("Job JSON does not match the job structure: %s"), *FailReason.ToString());
			return false;
		}

		OutJob = MoveTemp(Job);
		return true;
	}

	FString JobToJsonString(const FRenderProbeJob& Job)
	{
		FString JsonString;
		verify(FJsonObjectConverter::UStructToJsonObjectString(Job, JsonString));
		return JsonString;
	}

	FString ResultToJsonString(const FRenderProbeResult& Result)
	{
		FString JsonString;
		verify(FJsonObjectConverter::UStructToJsonObjectString(Result, JsonString));
		return JsonString;
	}
}
