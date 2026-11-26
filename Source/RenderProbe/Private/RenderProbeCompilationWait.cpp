// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeCompilationWait.h"

#include "AssetCompilingManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "MaterialShared.h"
#include "Templates/Function.h"

namespace RenderProbeCompilationWait
{
	static constexpr float PollIntervalSeconds = 0.05f;

	static bool PollUntil(const TFunctionRef<bool()> IsDone, const double Deadline)
	{
		while (!IsDone())
		{
			if (FPlatformTime::Seconds() >= Deadline)
			{
				return false;
			}
			FAssetCompilingManager::Get().ProcessAsyncTasks();
			FPlatformProcess::Sleep(PollIntervalSeconds);
		}
		return true;
	}

	static TSet<const FMaterialResource*> CollectUnfinished(const TArray<FMaterialResource*>& Resources)
	{
		TSet<const FMaterialResource*> Unfinished;
		for (const FMaterialResource* Resource : Resources)
		{
			if (!Resource->IsCompilationFinished())
			{
				Unfinished.Add(Resource);
			}
		}
		return Unfinished;
	}

	TSet<const FMaterialResource*> WaitForMaterials(const TArray<FMaterialResource*>& Resources, const double Deadline)
	{
		for (FMaterialResource* Resource : Resources)
		{
			if (!Resource->IsGameThreadShaderMapComplete())
			{
				Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::High);
			}
		}

		TSet<const FMaterialResource*> Unfinished;
		PollUntil([&Unfinished, &Resources]()
		{
			Unfinished = CollectUnfinished(Resources);
			return Unfinished.IsEmpty();
		}, Deadline);
		return Unfinished;
	}

	bool WaitForAllAssets(const double Deadline)
	{
		return PollUntil([]() { return FAssetCompilingManager::Get().GetNumRemainingAssets() == 0; }, Deadline);
	}
}
