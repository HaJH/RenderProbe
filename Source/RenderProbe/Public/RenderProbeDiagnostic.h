// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "RenderProbeDiagnostic.generated.h"

UENUM(BlueprintType, meta = (ToolTip = "What went wrong. Job-wide kinds go to the result's top-level diagnostics; per-subject kinds go to that item's diagnostics."))
enum class ERenderProbeDiagnosticKind : uint8
{
	InvalidJob UMETA(ToolTip = "The job is malformed: profile has both or neither of asset and settings, a name breaks the naming rules or is duplicated, a comparison refers to a missing subject, a subject without a mesh has stats off, a resolution axis is not positive, a fitBounds camera's fov is outside (0, 180), or output paths collide. Nothing is rendered."),
	Nondeterministic UMETA(ToolTip = "Two captures for a comparison were not similar under that comparison's tolerance. Only that comparison and camera get no verdict; detail names both."),
	StatsUnavailable UMETA(ToolTip = "The material resource or shader map needed for statistics does not exist."),
	OutputFailed UMETA(ToolTip = "An output could not be produced. Reading the render target or writing a PNG is reported on that subject; writing result.json is reported at the top level and appears only in the returned result. detail holds the path and the failed step."),
	MissingAsset UMETA(ToolTip = "A mesh, material, level or profile path does not resolve to an asset."),
	CompileError UMETA(ToolTip = "Shader compilation failed. detail holds the compiler errors."),
	DefaultMaterial UMETA(ToolTip = "After waiting, a slot still renders with the engine default material."),
	Timeout UMETA(ToolTip = "Waiting for shader compilation or streaming passed the time limit."),
	UnsupportedLevel UMETA(ToolTip = "An environment level is a world partition map."),
};

USTRUCT(BlueprintType, meta = (ToolTip = "One diagnostic."))
struct RENDERPROBE_API FRenderProbeDiagnostic
{
	GENERATED_BODY()

	static FRenderProbeDiagnostic Make(const ERenderProbeDiagnosticKind InKind, FString InSlot, FString InDetail)
	{
		FRenderProbeDiagnostic Diagnostic;
		Diagnostic.Kind = InKind;
		Diagnostic.Slot = MoveTemp(InSlot);
		Diagnostic.Detail = MoveTemp(InDetail);
		return Diagnostic;
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Diagnostic kind."))
	ERenderProbeDiagnosticKind Kind = ERenderProbeDiagnosticKind::InvalidJob;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Material slot key the diagnostic is about (slot index, or label without a mesh). Empty when not about one slot."))
	FString Slot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RenderProbe", meta = (ToolTip = "Human-readable details: offending names, colliding paths, compiler errors."))
	FString Detail;
};
