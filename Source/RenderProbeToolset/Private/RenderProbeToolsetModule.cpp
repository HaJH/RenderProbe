// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "Modules/ModuleManager.h"
#include "RenderProbeToolset.h"
#include "ToolsetRegistry/UToolsetRegistry.h"

class FRenderProbeToolsetModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		UToolsetRegistry::RegisterToolsetClass(URenderProbeToolset::StaticClass());
	}

	virtual void ShutdownModule() override
	{
		UToolsetRegistry::UnregisterToolsetClass(URenderProbeToolset::StaticClass());
	}
};

IMPLEMENT_MODULE(FRenderProbeToolsetModule, RenderProbeToolset)
