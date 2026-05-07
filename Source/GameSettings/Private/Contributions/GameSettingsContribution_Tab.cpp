// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Tab.h"

#include "GameSettingCollection.h"
#include "GameSettingRegistry.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContribution_Tab)

#define LOCTEXT_NAMESPACE "GameSettings"

void UGameSettingsContribution_Tab::Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
{
	if (!SettingId.IsValid() || DisplayName.IsEmpty())
	{
		return;
	}

	UGameSettingCollection* Tab = NewObject<UGameSettingCollection>(&Registry);
	ConfigureBaseSetting(*Tab);

	const FGameSettingHandle Handle = Registry.AddTab(Tab);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameSettingsContribution_Tab::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!SettingId.IsValid())
	{
		Context.AddError(LOCTEXT("Tab_NoTag", "Tab contribution: SettingId tag is required."));
		Result = EDataValidationResult::Invalid;
	}
	if (DisplayName.IsEmpty())
	{
		Context.AddError(LOCTEXT("Tab_NoName", "Tab contribution: DisplayName is required."));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
