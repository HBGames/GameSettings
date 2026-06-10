// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameFeatureAction_AddViewBindings.h"

#include "GameSettingsLog.h"
#include "GameSettingsModule.h"
#include "Widgets/GameSettingsViewBindings.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameFeatureAction_AddViewBindings)

#define LOCTEXT_NAMESPACE "GameSettingsGameFeatures"

void UGameFeatureAction_AddViewBindings::OnGameFeatureActivating(FGameFeatureActivatingContext& Context)
{
	UGameSettingsViewBindings* Resolved = Bindings.LoadSynchronous();
	if (!Resolved)
	{
		UE_LOG(LogGameSettings, Warning,
			TEXT("GameFeatureAction_AddViewBindings: Bindings '%s' failed to load on activation."),
			*Bindings.ToString());
		return;
	}

	FGuid& OverrideHandle = ActiveOverrideHandles.FindOrAdd(Context);
	if (!ensure(!OverrideHandle.IsValid()))
	{
		// Double activation without a deactivate. Pop the stale override so it can't leak.
		FGameSettingsModule::Get().RemoveViewBindingsOverride(OverrideHandle);
	}

	ResolvedBindings = Resolved;
	OverrideHandle = FGameSettingsModule::Get().AddViewBindingsOverride(Resolved, Priority);
	UE_LOG(LogGameSettings, Verbose,
		TEXT("Pushed view-bindings override '%s' at priority %d"), *Resolved->GetName(), Priority);
}

void UGameFeatureAction_AddViewBindings::OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context)
{
	FGuid OverrideHandle;
	if (ActiveOverrideHandles.RemoveAndCopyValue(Context, OverrideHandle) && OverrideHandle.IsValid())
	{
		if (FGameSettingsModule* Module = FGameSettingsModule::GetPtr())
		{
			Module->RemoveViewBindingsOverride(OverrideHandle);
		}
	}

	if (ActiveOverrideHandles.IsEmpty())
	{
		ResolvedBindings = nullptr;
	}
}

#if WITH_EDITOR
EDataValidationResult UGameFeatureAction_AddViewBindings::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (Bindings.IsNull())
	{
		Context.AddError(LOCTEXT("AddViewBindings_Empty",
			"AddViewBindings action: Bindings soft pointer is empty. Either remove the action or assign an asset."));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
