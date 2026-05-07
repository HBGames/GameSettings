// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsModule.h"

#include "Algo/BinarySearch.h"
#include "GameSettingHandle.h"
#include "GameSettingsAutoContributor.h"
#include "GameSettingsLog.h"
#include "Widgets/GameSettingsViewBindings.h"

#include "UObject/UObjectHash.h"
#include "UObject/UObjectIterator.h"

#include <atomic>

DEFINE_LOG_CATEGORY(LogGameSettings);

namespace
{
	// Process-wide handle allocator. 0 is reserved for "invalid", so the
	// first allocated handle is 1.
	std::atomic<uint64> GNextSettingHandle{1};
}

FGameSettingHandle FGameSettingHandle::Generate()
{
	return FGameSettingHandle{GNextSettingHandle.fetch_add(1, std::memory_order_relaxed)};
}

FString FGameSettingHandle::ToString() const
{
	return FString::Printf(TEXT("FGameSettingHandle(%llu)"), Value);
}

void FGameSettingsModule::StartupModule()
{
	SweepAutoContributors();

	// New modules can introduce more contributor subclasses; sweep again
	// whenever the module manager fires.
	ModulesChangedHandle = FModuleManager::Get().OnModulesChanged().AddRaw(this, &FGameSettingsModule::OnModulesChanged);
}

void FGameSettingsModule::ShutdownModule()
{
	if (ModulesChangedHandle.IsValid())
	{
		FModuleManager::Get().OnModulesChanged().Remove(ModulesChangedHandle);
		ModulesChangedHandle.Reset();
	}

	OnAutoContributorDiscovered.Clear();
	OnViewBindingsOverridesChanged.Clear();
	AutoContributors.Reset();
	KnownContributorClasses.Reset();
	ViewBindingsOverrides.Reset();
}

FGuid FGameSettingsModule::AddViewBindingsOverride(UGameSettingsViewBindings* Bindings, int32 Priority)
{
	if (!Bindings)
	{
		return FGuid();
	}

	FViewBindingsOverride Entry;
	Entry.Handle = FGuid::NewGuid();
	Entry.Bindings = Bindings;
	Entry.Priority = Priority;

	// Insert preserving descending priority. UpperBound lets later entries
	// land after equally-prioritized earlier ones; they sort to the back of
	// their priority group, so older overrides win ties (insertion-order).
	const int32 InsertIndex = Algo::UpperBoundBy(ViewBindingsOverrides, Priority,
		[](const FViewBindingsOverride& E) { return E.Priority; },
		[](int32 A, int32 B) { return A > B; });
	ViewBindingsOverrides.Insert(MoveTemp(Entry), InsertIndex);

	const FGuid ResultHandle = ViewBindingsOverrides[InsertIndex].Handle;
	OnViewBindingsOverridesChanged.Broadcast();
	return ResultHandle;
}

void FGameSettingsModule::RemoveViewBindingsOverride(const FGuid& OverrideHandle)
{
	if (!OverrideHandle.IsValid())
	{
		return;
	}
	const int32 Removed = ViewBindingsOverrides.RemoveAll(
		[&OverrideHandle](const FViewBindingsOverride& E) { return E.Handle == OverrideHandle; });
	if (Removed > 0)
	{
		OnViewBindingsOverridesChanged.Broadcast();
	}
}

TArray<UGameSettingsViewBindings*> FGameSettingsModule::GetActiveViewBindings() const
{
	TArray<UGameSettingsViewBindings*> Out;
	Out.Reserve(ViewBindingsOverrides.Num());
	for (const FViewBindingsOverride& Entry : ViewBindingsOverrides)
	{
		if (UGameSettingsViewBindings* Live = Entry.Bindings.Get())
		{
			Out.Add(Live);
		}
	}
	return Out;
}

void FGameSettingsModule::OnModulesChanged(FName /*ModuleThatChanged*/, EModuleChangeReason ReasonForChange)
{
	if (ReasonForChange == EModuleChangeReason::ModuleLoaded)
	{
		SweepAutoContributors();
	}
}

void FGameSettingsModule::SweepAutoContributors()
{
	// CDO discovery via TObjectIterator. The base class itself (Abstract)
	// is filtered out because we walk subclasses through GetDerivedClasses.
	TArray<UClass*> Subclasses;
	GetDerivedClasses(UGameSettingsAutoContributor::StaticClass(), Subclasses, /*bRecursive*/ true);

	for (UClass* SubclassOfContributor : Subclasses)
	{
		if (!SubclassOfContributor)
		{
			continue;
		}
		if (SubclassOfContributor->HasAnyClassFlags(CLASS_Deprecated | CLASS_Abstract))
		{
			continue;
		}
		const TWeakObjectPtr<UClass> ClassWeak(SubclassOfContributor);
		if (KnownContributorClasses.Contains(ClassWeak))
		{
			continue;
		}

		UGameSettingsAutoContributor* CDO = SubclassOfContributor->GetDefaultObject<UGameSettingsAutoContributor>();
		if (!CDO)
		{
			continue;
		}

		KnownContributorClasses.Add(ClassWeak);
		AutoContributors.Add(CDO);

		UE_LOG(LogGameSettings, Log, TEXT("Discovered auto-contributor %s"), *SubclassOfContributor->GetName());

		OnAutoContributorDiscovered.Broadcast(CDO);
	}
}

IMPLEMENT_MODULE(FGameSettingsModule, GameSettings);
