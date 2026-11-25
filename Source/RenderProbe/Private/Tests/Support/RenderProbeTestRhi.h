// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "RHIGlobals.h"

namespace RenderProbeTestRhi
{
	inline constexpr const TCHAR* NoRhiWarning = TEXT("Skipped without a verdict: this test judges rendering and the process runs without an RHI (-nullrhi). Run it in an editor that renders.");

	/** True when an RHI renders; otherwise records a warning on Test so the caller can end without a verdict. */
	inline bool CanRender(FAutomationTestBase& Test)
	{
		if (!GUsingNullRHI)
		{
			return true;
		}
		Test.AddWarning(NoRhiWarning);
		return false;
	}
}

#endif
