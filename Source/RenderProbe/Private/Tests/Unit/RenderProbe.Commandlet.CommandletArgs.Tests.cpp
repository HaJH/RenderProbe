// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeCommandletArgs.h"

#if WITH_AUTOMATION_TESTS

#include "CQTest.h"

namespace RenderProbeCommandletArgsTests
{

static constexpr const TCHAR* JobFilePath = TEXT("C:/Jobs/job.json");
static constexpr const TCHAR* OutputDir = TEXT("C:/Out/run");
static constexpr const TCHAR* SchemaFilePath = TEXT("C:/Out/schema.json");
static constexpr const TCHAR* QuotedJobFilePath = TEXT("C:/My Jobs/job.json");

TEST_CLASS(RenderProbeCommandletArgsParse, "RenderProbe.Unit.Commandlet")
{
	void AssertRejected(const FString& Params)
	{
		FRenderProbeCommandletArgs Args;
		FString Error;
		ASSERT_THAT(IsFalse(RenderProbeCommandletArgs::Parse(Params, Args, Error), *Params));
		ASSERT_THAT(IsFalse(Error.IsEmpty(), *Params));
	}

	TEST_METHOD(JobAloneRunsJobWithoutOutputOverride)
	{
		FRenderProbeCommandletArgs Args;
		FString Error;
		ASSERT_THAT(IsTrue(RenderProbeCommandletArgs::Parse(FString::Printf(TEXT("-job=%s -AllowCommandletRendering"), JobFilePath), Args, Error)));
		ASSERT_THAT(IsTrue(Args.Mode == ERenderProbeCommandletMode::RunJob));
		ASSERT_THAT(AreEqual(FString(JobFilePath), Args.JobFilePath));
		ASSERT_THAT(IsFalse(Args.OutputDir.IsSet()));
	}

	TEST_METHOD(JobWithOutSetsOutputOverride)
	{
		FRenderProbeCommandletArgs Args;
		FString Error;
		ASSERT_THAT(IsTrue(RenderProbeCommandletArgs::Parse(FString::Printf(TEXT("-job=%s -out=%s"), JobFilePath, OutputDir), Args, Error)));
		ASSERT_THAT(IsTrue(Args.Mode == ERenderProbeCommandletMode::RunJob));
		ASSERT_THAT(AreEqual(FString(JobFilePath), Args.JobFilePath));
		ASSERT_THAT(IsTrue(Args.OutputDir.IsSet()));
		ASSERT_THAT(AreEqual(FString(OutputDir), Args.OutputDir.GetValue()));
	}

	TEST_METHOD(QuotedJobPathKeepsSpaces)
	{
		FRenderProbeCommandletArgs Args;
		FString Error;
		ASSERT_THAT(IsTrue(RenderProbeCommandletArgs::Parse(FString::Printf(TEXT("-job=\"%s\""), QuotedJobFilePath), Args, Error)));
		ASSERT_THAT(AreEqual(FString(QuotedJobFilePath), Args.JobFilePath));
	}

	TEST_METHOD(SchemaAloneWritesSchema)
	{
		FRenderProbeCommandletArgs Args;
		FString Error;
		ASSERT_THAT(IsTrue(RenderProbeCommandletArgs::Parse(FString::Printf(TEXT("-schema=%s"), SchemaFilePath), Args, Error)));
		ASSERT_THAT(IsTrue(Args.Mode == ERenderProbeCommandletMode::WriteSchema));
		ASSERT_THAT(AreEqual(FString(SchemaFilePath), Args.SchemaFilePath));
	}

	TEST_METHOD(NoArgumentsAreRejected)
	{
		AssertRejected(FString());
		AssertRejected(TEXT("-AllowCommandletRendering"));
	}

	TEST_METHOD(OutWithoutJobIsRejected)
	{
		AssertRejected(FString::Printf(TEXT("-out=%s"), OutputDir));
	}

	TEST_METHOD(JobAndSchemaTogetherAreRejected)
	{
		AssertRejected(FString::Printf(TEXT("-job=%s -schema=%s"), JobFilePath, SchemaFilePath));
	}

	TEST_METHOD(SchemaWithOutIsRejected)
	{
		AssertRejected(FString::Printf(TEXT("-schema=%s -out=%s"), SchemaFilePath, OutputDir));
	}

	TEST_METHOD(MissingValuesAreRejected)
	{
		AssertRejected(TEXT("-job="));
		AssertRejected(TEXT("-schema="));
		AssertRejected(FString::Printf(TEXT("-job= -out=%s"), OutputDir));
		AssertRejected(FString::Printf(TEXT("-job=%s -out="), JobFilePath));
		AssertRejected(FString::Printf(TEXT("-job=%s -out= -AllowCommandletRendering"), JobFilePath));
	}
};

}

#endif
