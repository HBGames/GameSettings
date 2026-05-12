// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsSubsystem.h"

#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "GameSettingRegistry.h"
#include "GameSettingsAssetDiscoverySubsystem.h"
#include "GameSettingsAutoContributor.h"
#include "GameSettingsContribution.h"
#include "GameSettingsDeveloperSettings.h"
#include "GameSettingsLog.h"
#include "GameSettingsModule.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsSubsystem)

void UGameSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	OnAutoContributorDiscoveredHandle = FGameSettingsModule::Get().OnAutoContributorDiscovered.AddUObject(
		this, &UGameSettingsSubsystem::ApplySingleAutoContributor);

	if (const ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UGameInstance* GameInstance = LP->GetGameInstance())
		{
			if (UGameSettingsAssetDiscoverySubsystem* Disco = GameInstance->GetSubsystem<UGameSettingsAssetDiscoverySubsystem>())
			{
				OnAssetContributionReadyHandle = Disco->OnContributionAssetReady.AddUObject(
					this, &UGameSettingsSubsystem::ApplyAssetContribution);
				OnAssetContributionRemovedHandle = Disco->OnContributionAssetRemoved.AddUObject(
					this, &UGameSettingsSubsystem::RemoveAssetContribution);
			}
		}
	}
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

	if (const ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UGameInstance* GameInstance = LP->GetGameInstance())
		{
			if (UGameSettingsAssetDiscoverySubsystem* Disco = GameInstance->GetSubsystem<UGameSettingsAssetDiscoverySubsystem>())
			{
				if (OnAssetContributionReadyHandle.IsValid())
				{
					Disco->OnContributionAssetReady.Remove(OnAssetContributionReadyHandle);
				}
				if (OnAssetContributionRemovedHandle.IsValid())
				{
					Disco->OnContributionAssetRemoved.Remove(OnAssetContributionRemovedHandle);
				}
			}
		}
	}
	OnAssetContributionReadyHandle.Reset();
	OnAssetContributionRemovedHandle.Reset();

	RemoveAllAppliedContributionHandles();
	AppliedContributions.Empty();

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

	ApplyAllKnownContributions();

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

	ApplyAllKnownContributions();
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

void UGameSettingsSubsystem::ApplyAllKnownContributions()
{
	if (!Registry)
	{
		return;
	}

	for (UGameSettingsAutoContributor* Contributor : FGameSettingsModule::Get().GetAutoContributors())
	{
		ApplySingleAutoContributor(Contributor);
	}

	if (const ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UGameInstance* GameInstance = LP->GetGameInstance())
		{
			if (UGameSettingsAssetDiscoverySubsystem* Disco = GameInstance->GetSubsystem<UGameSettingsAssetDiscoverySubsystem>())
			{
				for (UGameSettingsContribution* Contribution : Disco->GetKnownContributions())
				{
					ApplyAssetContribution(Contribution);
				}
			}
		}
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

	const bool bAlreadyApplied = AppliedContributions.ContainsByPredicate(
		[Contributor](const FAppliedContribution& Entry) { return Entry.Contribution.Get() == Contributor; });
	if (bAlreadyApplied)
	{
		return;
	}

	FAppliedContribution Entry;
	Entry.Contribution = Contributor;
	Contributor->Apply(*Registry, Entry.Handles);

	if (Entry.Handles.Num() > 0)
	{
		UE_LOG(LogGameSettings, Verbose, TEXT("Auto-contributor %s added %d setting(s) to LocalPlayer %s"),
			*Contributor->GetClass()->GetName(),
			Entry.Handles.Num(),
			GetLocalPlayer() ? *GetLocalPlayer()->GetName() : TEXT("(null)"));
	}

	AppliedContributions.Add(MoveTemp(Entry));
}

void UGameSettingsSubsystem::ApplyAssetContribution(UGameSettingsContribution* Contribution)
{
	if (!Contribution || !Registry)
	{
		return;
	}
	if (!Contribution->bEnabled)
	{
		return;
	}

	const bool bAlreadyApplied = AppliedContributions.ContainsByPredicate(
		[Contribution](const FAppliedContribution& Entry) { return Entry.Contribution.Get() == Contribution; });
	if (bAlreadyApplied)
	{
		return;
	}

	FAppliedContribution Entry;
	Entry.Contribution = Contribution;
	Contribution->Apply(*Registry, Entry.Handles);

	if (Entry.Handles.Num() > 0)
	{
		UE_LOG(LogGameSettings, Verbose, TEXT("Asset contribution %s added %d setting(s) to LocalPlayer %s"),
			*Contribution->GetName(),
			Entry.Handles.Num(),
			GetLocalPlayer() ? *GetLocalPlayer()->GetName() : TEXT("(null)"));
	}

	AppliedContributions.Add(MoveTemp(Entry));
}

void UGameSettingsSubsystem::RemoveAssetContribution(UGameSettingsContribution* Contribution)
{
	if (!Contribution || !Registry)
	{
		return;
	}

	for (int32 Index = AppliedContributions.Num() - 1; Index >= 0; --Index)
	{
		const FAppliedContribution& Entry = AppliedContributions[Index];
		if (Entry.Contribution.Get() != Contribution)
		{
			continue;
		}
		for (const FGameSettingHandle& Handle : Entry.Handles)
		{
			Registry->RemoveByHandle(Handle);
		}
		AppliedContributions.RemoveAt(Index);
	}
}

void UGameSettingsSubsystem::RemoveAllAppliedContributionHandles()
{
	if (!Registry)
	{
		return;
	}

	for (const FAppliedContribution& Entry : AppliedContributions)
	{
		for (const FGameSettingHandle& Handle : Entry.Handles)
		{
			Registry->RemoveByHandle(Handle);
		}
	}
}
