// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameSettingRegistry.h"
#include "GameSettingsAutoContributor.h"
#include "GameSettingsContribution.h"
#include "GameSettingsDeveloperSettings.h"
#include "GameSettingsLog.h"
#include "GameSettingsModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsSubsystem)

void UGameSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Subscribe to discovery of new auto-contributors (e.g. a plugin
	// loading later) so they apply to this subsystem too.
	OnAutoContributorDiscoveredHandle = FGameSettingsModule::Get().OnAutoContributorDiscovered.AddUObject(
		this, &UGameSettingsSubsystem::ApplySingleAutoContributor);
}

void UGameSettingsSubsystem::Deinitialize()
{
	if (OnAutoContributorDiscoveredHandle.IsValid())
	{
		if (FGameSettingsModule* Module = FGameSettingsModule::GetPtr())
		{
			Module->OnAutoContributorDiscovered.Remove(OnAutoContributorDiscoveredHandle);
		}
		OnAutoContributorDiscoveredHandle.Reset();
	}

	RemoveAllAutoContributorHandles();
	AppliedAutoContributions.Empty();

	// GC handles cleanup; null the ref so any stale weak-pointer holders see it.
	Registry = nullptr;
	Super::Deinitialize();
}

UGameSettingRegistry* UGameSettingsSubsystem::GetOrCreateRegistry()
{
	if (Registry)
	{
		return Registry;
	}

	// Resolve the configured registry class. Falls back to the plain
	// UGameSettingRegistry if the developer setting hasn't been touched.
	const UGameSettingsDeveloperSettings* Settings = GetDefault<UGameSettingsDeveloperSettings>();
	UClass* RegistryClass = nullptr;
	if (Settings && Settings->RegistryClass.IsValid())
	{
		RegistryClass = Settings->RegistryClass.TryLoadClass<UGameSettingRegistry>();
		if (!RegistryClass)
		{
			UE_LOG(LogGameSettings, Warning,
				TEXT("UGameSettingsDeveloperSettings::RegistryClass '%s' failed to load; falling back to UGameSettingRegistry."),
				*Settings->RegistryClass.ToString());
		}
	}
	if (!RegistryClass)
	{
		RegistryClass = UGameSettingRegistry::StaticClass();
	}

	Registry = NewObject<UGameSettingRegistry>(this, RegistryClass);
	Registry->Initialize(GetLocalPlayer());

	ApplyAllKnownAutoContributors();

	return Registry;
}

void UGameSettingsSubsystem::SetRegistry(UGameSettingRegistry* InRegistry)
{
	if (!ensureMsgf(InRegistry, TEXT("UGameSettingsSubsystem::SetRegistry called with null registry")))
	{
		return;
	}
	if (!ensureMsgf(!Registry, TEXT("UGameSettingsSubsystem already has a registry; call UGameSettingRegistry::Regenerate() instead of swapping")))
	{
		return;
	}

	// Reparent the registry to the subsystem so it isn't garbage-collected
	// when the screen widget that built it is destroyed.
	if (InRegistry->GetOuter() != this)
	{
		InRegistry->Rename(nullptr, this, REN_DontCreateRedirectors | REN_DoNotDirty);
	}

	Registry = InRegistry;

	const ULocalPlayer* LP = GetLocalPlayer();
	UE_LOG(LogGameSettings, Verbose, TEXT("Registry %s assigned to GameSettingsSubsystem for LocalPlayer %s"),
		*Registry->GetName(),
		LP ? *LP->GetName() : TEXT("(null)"));

	ApplyAllKnownAutoContributors();
}

void UGameSettingsSubsystem::ApplyContribution(UGameSettingsContribution* Contribution, TArray<FGameSettingHandle>& OutHandles)
{
	if (!Contribution)
	{
		return;
	}

	UGameSettingRegistry* Reg = GetOrCreateRegistry();
	if (!Reg)
	{
		return;
	}

	Contribution->Apply(*Reg, OutHandles);
}

void UGameSettingsSubsystem::ApplyAllKnownAutoContributors()
{
	if (!Registry)
	{
		return;
	}

	for (UGameSettingsAutoContributor* Contributor : FGameSettingsModule::Get().GetAutoContributors())
	{
		ApplySingleAutoContributor(Contributor);
	}
}

void UGameSettingsSubsystem::ApplySingleAutoContributor(UGameSettingsAutoContributor* Contributor)
{
	if (!Contributor || !Registry)
	{
		return;
	}
	if (!Contributor->ShouldAutoContribute())
	{
		return;
	}

	// Skip if already applied (e.g. discovered twice during a sweep).
	const bool bAlreadyApplied = AppliedAutoContributions.ContainsByPredicate(
		[Contributor](const FAppliedAutoContribution& Entry) { return Entry.Contributor.Get() == Contributor; });
	if (bAlreadyApplied)
	{
		return;
	}

	FAppliedAutoContribution Entry;
	Entry.Contributor = Contributor;
	Contributor->Apply(*Registry, Entry.Handles);

	if (Entry.Handles.Num() > 0)
	{
		UE_LOG(LogGameSettings, Verbose, TEXT("Auto-contributor %s added %d setting(s) to LocalPlayer %s"),
			*Contributor->GetClass()->GetName(),
			Entry.Handles.Num(),
			GetLocalPlayer() ? *GetLocalPlayer()->GetName() : TEXT("(null)"));
	}

	AppliedAutoContributions.Add(MoveTemp(Entry));
}

void UGameSettingsSubsystem::RemoveAllAutoContributorHandles()
{
	if (!Registry)
	{
		return;
	}

	for (const FAppliedAutoContribution& Entry : AppliedAutoContributions)
	{
		for (const FGameSettingHandle& Handle : Entry.Handles)
		{
			Registry->RemoveByHandle(Handle);
		}
	}
}
