// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameFeatureAction_RegisterGameSettings.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFeaturesSubsystem.h"
#include "GameSettingRegistry.h"
#include "GameSettingsContribution.h"
#include "GameSettingsLog.h"
#include "GameSettingsSubsystem.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_RegisterGameSettings)

#define LOCTEXT_NAMESPACE "GameSettingsGameFeatures"

void UGameFeatureAction_RegisterGameSettings::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	FPerContextData& ActiveData = ContextData.FindOrAdd(Context);
	if (!ensure(ActiveData.ActiveContributions.IsEmpty()) ||
		!ensure(ActiveData.LocalPlayerAddedHandles.IsEmpty()) ||
		!ensure(!ActiveData.GameInstanceStartHandle.IsValid()))
	{
		// Double activation without a deactivate. Tear the stale state down first.
		Reset(ActiveData);
	}

	// Catch game instances that start after activation (early-activated GFPs, PIE started while active).
	ActiveData.GameInstanceStartHandle = FWorldDelegates::OnStartGameInstance.AddUObject(
		this, &UGameFeatureAction_RegisterGameSettings::HandleGameInstanceStart, FGameFeatureStateChangeContext(Context));

	// Apply to game instances that already exist.
	if (GEngine)
	{
		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			if (Context.ShouldApplyToWorldContext(WorldContext))
			{
				AddToGameInstance(WorldContext.OwningGameInstance, ActiveData, Context);
			}
		}
	}
}

void UGameFeatureAction_RegisterGameSettings::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	FPerContextData* ActiveData = ContextData.Find(Context);
	if (ensure(ActiveData))
	{
		Reset(*ActiveData);
		ContextData.Remove(Context);
	}
}

void UGameFeatureAction_RegisterGameSettings::HandleGameInstanceStart(UGameInstance* GameInstance, FGameFeatureStateChangeContext ChangeContext)
{
	if (FWorldContext* WorldContext = GameInstance ? GameInstance->GetWorldContext() : nullptr)
	{
		if (ChangeContext.ShouldApplyToWorldContext(*WorldContext))
		{
			if (FPerContextData* ActiveData = ContextData.Find(ChangeContext))
			{
				AddToGameInstance(GameInstance, *ActiveData, ChangeContext);
			}
		}
	}
}

void UGameFeatureAction_RegisterGameSettings::HandleLocalPlayerAdded(ULocalPlayer* LocalPlayer, FGameFeatureStateChangeContext ChangeContext)
{
	if (FPerContextData* ActiveData = ContextData.Find(ChangeContext))
	{
		ApplyToLocalPlayer(LocalPlayer, *ActiveData);
	}
}

void UGameFeatureAction_RegisterGameSettings::AddToGameInstance(UGameInstance* GameInstance, FPerContextData& ActiveData, FGameFeatureStateChangeContext ChangeContext)
{
	if (!GameInstance || ActiveData.LocalPlayerAddedHandles.Contains(GameInstance))
	{
		return;
	}

	// Catch local players added after activation (split-screen joiners).
	ActiveData.LocalPlayerAddedHandles.Add(GameInstance, GameInstance->OnLocalPlayerAddedEvent.AddUObject(
		this, &UGameFeatureAction_RegisterGameSettings::HandleLocalPlayerAdded, ChangeContext));

	for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
	{
		ApplyToLocalPlayer(LocalPlayer, ActiveData);
	}
}

void UGameFeatureAction_RegisterGameSettings::ApplyToLocalPlayer(ULocalPlayer* LocalPlayer, FPerContextData& ActiveData)
{
	UGameSettingsSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UGameSettingsSubsystem>() : nullptr;
	if (!Subsystem)
	{
		return;
	}

	FActiveContribution Entry;
	Entry.Subsystem = Subsystem;

	for (UGameSettingsContribution* Contribution : Contributions)
	{
		if (Contribution)
		{
			Subsystem->ApplyContribution(Contribution, Entry.Handles);
		}
	}

	if (Entry.Handles.Num() > 0)
	{
		UE_LOG(LogGameSettings, Verbose,
			TEXT("GameFeatureAction_RegisterGameSettings activated %d handle(s) on LocalPlayer %s"),
			Entry.Handles.Num(),
			*LocalPlayer->GetName());

		ActiveData.ActiveContributions.Add(MoveTemp(Entry));
	}
}

void UGameFeatureAction_RegisterGameSettings::Reset(FPerContextData& ActiveData)
{
	if (ActiveData.GameInstanceStartHandle.IsValid())
	{
		FWorldDelegates::OnStartGameInstance.Remove(ActiveData.GameInstanceStartHandle);
		ActiveData.GameInstanceStartHandle.Reset();
	}

	for (TPair<TWeakObjectPtr<UGameInstance>, FDelegateHandle>& Pair : ActiveData.LocalPlayerAddedHandles)
	{
		if (UGameInstance* GameInstance = Pair.Key.Get())
		{
			GameInstance->OnLocalPlayerAddedEvent.Remove(Pair.Value);
		}
	}
	ActiveData.LocalPlayerAddedHandles.Empty();

	for (const FActiveContribution& Entry : ActiveData.ActiveContributions)
	{
		UGameSettingsSubsystem* Subsystem = Entry.Subsystem.Get();
		if (!Subsystem)
		{
			continue;
		}
		UGameSettingRegistry* Registry = Subsystem->GetRegistry();
		if (!Registry)
		{
			continue;
		}

		for (const FGameSettingHandle& Handle : Entry.Handles)
		{
			Registry->RemoveByHandle(Handle);
		}
	}
	ActiveData.ActiveContributions.Empty();
}

#if WITH_EDITOR
EDataValidationResult UGameFeatureAction_RegisterGameSettings::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	for (int32 Index = 0; Index < Contributions.Num(); ++Index)
	{
		if (!Contributions[Index])
		{
			Context.AddError(FText::Format(
				LOCTEXT("NullContribution", "Contributions[{0}] is null. Either remove the entry or assign a UGameSettingsContribution subclass."),
				FText::AsNumber(Index)));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
