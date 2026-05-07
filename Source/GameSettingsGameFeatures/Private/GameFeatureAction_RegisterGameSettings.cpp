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
	if (Contributions.IsEmpty() || !GEngine)
	{
		return;
	}

	for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
	{
		if (!Context.ShouldApplyToWorldContext(WorldContext))
		{
			continue;
		}

		UWorld* World = WorldContext.World();
		UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		if (!GameInstance)
		{
			continue;
		}

		for (ULocalPlayer* LocalPlayer : GameInstance->GetLocalPlayers())
		{
			UGameSettingsSubsystem* Subsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UGameSettingsSubsystem>() : nullptr;
			if (!Subsystem)
			{
				continue;
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

				ActiveContributions.Add(MoveTemp(Entry));
			}
		}
	}
}

void UGameFeatureAction_RegisterGameSettings::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	for (const FActiveContribution& Entry : ActiveContributions)
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
	ActiveContributions.Empty();
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
