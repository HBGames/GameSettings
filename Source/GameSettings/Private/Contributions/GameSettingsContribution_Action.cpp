// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Action.h"

#include "GameSettingAction.h"
#include "GameSettingRegistry.h"
#include "GameSettingsLog.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContribution_Action)

#define LOCTEXT_NAMESPACE "GameSettings"

const FPrimaryAssetType UGameSettingsContribution_Action::ContributionPrimaryAssetType = FPrimaryAssetType(TEXT("GameSettingsAction"));

FPrimaryAssetType UGameSettingsContribution_Action::GetContributionPrimaryAssetType() const
{
	return ContributionPrimaryAssetType;
}

void UGameSettingsContribution_Action::Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
{
	if (!GetPrimaryAssetId().IsValid() || DisplayName.IsEmpty())
	{
		UE_LOG(LogGameSettings, Error, TEXT("Action contribution %s skipped: %s."),
			*GetPathName(),
			!GetPrimaryAssetId().IsValid() ? TEXT("primary asset id is invalid") : TEXT("DisplayName is empty"));
		return;
	}
	if (!NamedAction.IsValid())
	{
		UE_LOG(LogGameSettings, Error, TEXT("Action contribution %s skipped: NamedAction tag is not set (the screen handler routes on it)."),
			*GetPathName());
		return;
	}

	UGameSettingAction* Setting = NewObject<UGameSettingAction>(&Registry);
	ConfigureBaseSetting(*Setting);

	if (!ActionText.IsEmpty())
	{
		Setting->SetActionText(ActionText);
	}
	Setting->SetNamedAction(NamedAction);

	const FGameSettingHandle Handle = Registry.AddSetting(Setting, ParentContainer);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
		Registry.ApplyEditConditionSpecs(Setting, EditConditions);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameSettingsContribution_Action::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!NamedAction.IsValid())
	{
		Context.AddError(LOCTEXT("Action_NoNamedAction", "Action contribution: NamedAction tag is required (this is what the screen handler routes on)."));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
