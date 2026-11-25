// Copyright 2026 Jeonghyeon Ha. All Rights Reserved.

#include "RenderProbeJobValidation.h"

#include "RenderProbeJob.h"
#include "RenderProbeOutputPaths.h"

namespace RenderProbeJobValidation
{
	static void AddInvalidJob(TArray<FRenderProbeDiagnostic>& OutDiagnostics, FString Detail)
	{
		OutDiagnostics.Add(FRenderProbeDiagnostic::Make(ERenderProbeDiagnosticKind::InvalidJob, FString(), MoveTemp(Detail)));
	}

	static bool IsAllowedNameCharacter(const TCHAR Character)
	{
		return (Character >= TEXT('A') && Character <= TEXT('Z'))
			|| (Character >= TEXT('a') && Character <= TEXT('z'))
			|| (Character >= TEXT('0') && Character <= TEXT('9'))
			|| Character == TEXT('_')
			|| Character == TEXT('-');
	}

	static bool UsesOnlyAllowedCharacters(const FString& Name)
	{
		for (const TCHAR Character : Name)
		{
			if (!IsAllowedNameCharacter(Character))
			{
				return false;
			}
		}
		return true;
	}

	template <typename EntryType>
	static void ValidateListNames(const TCHAR* ListName, const TArray<EntryType>& Entries, TArray<FRenderProbeDiagnostic>& OutDiagnostics)
	{
		TArray<FString> SeenNames;
		for (const EntryType& Entry : Entries)
		{
			const FString& Name = Entry.Name;
			if (Name.IsEmpty())
			{
				AddInvalidJob(OutDiagnostics, FString::Printf(TEXT("%s has an entry with an empty name"), ListName));
				continue;
			}
			if (!UsesOnlyAllowedCharacters(Name))
			{
				AddInvalidJob(OutDiagnostics, FString::Printf(TEXT("%s name '%s' uses characters other than A-Z, a-z, 0-9, '_' and '-'"), ListName, *Name));
			}
			if (Name.Contains(RenderProbeOutputPaths::NameSeparator, ESearchCase::CaseSensitive))
			{
				AddInvalidJob(OutDiagnostics, FString::Printf(TEXT("%s name '%s' contains '%s'"), ListName, *Name, RenderProbeOutputPaths::NameSeparator));
			}
			const bool bDuplicate = SeenNames.ContainsByPredicate([&Name](const FString& Seen)
			{
				return Seen.Equals(Name, ESearchCase::CaseSensitive);
			});
			if (bDuplicate)
			{
				AddInvalidJob(OutDiagnostics, FString::Printf(TEXT("%s name '%s' is used more than once"), ListName, *Name));
			}
			SeenNames.Add(Name);
		}
	}

	static void ValidateProfile(const FRenderProbeProfileSource& Profile, TArray<FRenderProbeDiagnostic>& OutDiagnostics)
	{
		const bool bHasAsset = !Profile.Asset.IsEmpty();
		const bool bHasSettings = Profile.Settings.IsSet();
		if (bHasAsset && bHasSettings)
		{
			AddInvalidJob(OutDiagnostics, TEXT("profile has both asset and settings; give exactly one"));
		}
		else if (!bHasAsset && !bHasSettings)
		{
			AddInvalidJob(OutDiagnostics, TEXT("profile has neither asset nor settings; give exactly one"));
		}
	}

	static const FRenderProbeItem* FindItem(const FRenderProbeJob& Job, const FString& ItemName)
	{
		return Job.Items.FindByPredicate([&ItemName](const FRenderProbeItem& Item)
		{
			return Item.Name.Equals(ItemName, ESearchCase::CaseSensitive);
		});
	}

	static void ValidateCompareTarget(const FRenderProbeJob& Job, const FRenderProbeCompare& Compare, const FString& TargetName, TArray<FRenderProbeDiagnostic>& OutDiagnostics)
	{
		const FRenderProbeItem* Target = FindItem(Job, TargetName);
		if (!Target)
		{
			AddInvalidJob(OutDiagnostics, FString::Printf(TEXT("compare '%s' refers to item '%s', which is not in items"), *Compare.Name, *TargetName));
		}
		else if (Target->Mesh.IsEmpty())
		{
			AddInvalidJob(OutDiagnostics, FString::Printf(TEXT("compare '%s' refers to item '%s', which has no mesh"), *Compare.Name, *TargetName));
		}
	}

	static void ValidateItemsAndCompares(const FRenderProbeJob& Job, TArray<FRenderProbeDiagnostic>& OutDiagnostics)
	{
		for (const FRenderProbeItem& Item : Job.Items)
		{
			if (Item.Mesh.IsEmpty() && !Item.bStats)
			{
				AddInvalidJob(OutDiagnostics, FString::Printf(TEXT("item '%s' has no mesh and stats is false, so it would produce nothing"), *Item.Name));
			}
		}

		for (const FRenderProbeCompare& Compare : Job.Compare)
		{
			ValidateCompareTarget(Job, Compare, Compare.A, OutDiagnostics);
			ValidateCompareTarget(Job, Compare, Compare.B, OutDiagnostics);
		}
	}

	static void AddOutputPath(TMap<FString, FString>& PathsByLowerCase, const FString& Path, TArray<FRenderProbeDiagnostic>& OutDiagnostics)
	{
		const FString LowerCasePath = Path.ToLower();
		if (const FString* Existing = PathsByLowerCase.Find(LowerCasePath))
		{
			AddInvalidJob(OutDiagnostics, FString::Printf(TEXT("output paths collide ignoring case: '%s' and '%s'"), **Existing, *Path));
			return;
		}
		PathsByLowerCase.Add(LowerCasePath, Path);
	}

	static void ValidateOutputPaths(const FRenderProbeJob& Job, TArray<FRenderProbeDiagnostic>& OutDiagnostics)
	{
		TMap<FString, FString> PathsByLowerCase;
		for (const FRenderProbeItem& Item : Job.Items)
		{
			if (Item.Mesh.IsEmpty())
			{
				continue;
			}
			for (const FRenderProbeCamera& Camera : Job.Cameras)
			{
				AddOutputPath(PathsByLowerCase, RenderProbeOutputPaths::CapturePath(Job.OutputDir, Item.Name, Camera.Name), OutDiagnostics);
			}
		}
		for (const FRenderProbeCompare& Compare : Job.Compare)
		{
			for (const FRenderProbeCamera& Camera : Job.Cameras)
			{
				AddOutputPath(PathsByLowerCase, RenderProbeOutputPaths::DiffPath(Job.OutputDir, Compare.Name, Camera.Name), OutDiagnostics);
			}
		}
	}

	TArray<FRenderProbeDiagnostic> Validate(const FRenderProbeJob& Job)
	{
		TArray<FRenderProbeDiagnostic> Diagnostics;

		ValidateProfile(Job.Profile, Diagnostics);
		if (Job.OutputDir.IsEmpty())
		{
			AddInvalidJob(Diagnostics, TEXT("outputDir is empty"));
		}
		ValidateListNames(TEXT("items"), Job.Items, Diagnostics);
		ValidateListNames(TEXT("cameras"), Job.Cameras, Diagnostics);
		ValidateListNames(TEXT("compare"), Job.Compare, Diagnostics);
		ValidateItemsAndCompares(Job, Diagnostics);
		ValidateOutputPaths(Job, Diagnostics);

		return Diagnostics;
	}
}
