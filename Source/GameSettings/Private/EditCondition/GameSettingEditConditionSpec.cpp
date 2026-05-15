// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "EditCondition/GameSettingEditConditionSpec.h"

#include "GameSettingFilterState.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingEditConditionSpec)

#define LOCTEXT_NAMESPACE "GameSettings"

void UGameSettingEditConditionSpec::ApplyActionToState(FGameSettingEditableState& State) const
{
	switch (Action)
	{
	case EGameSettingEditAction::Hide:
		State.Hide(DevReason);
		break;
	case EGameSettingEditAction::Disable:
		State.Disable(DisableReason);
		break;
	case EGameSettingEditAction::Kill:
		State.Kill(DevReason);
		break;
	default:
		break;
	}
}

#if WITH_EDITOR
EDataValidationResult UGameSettingEditConditionSpec::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (Action == EGameSettingEditAction::Disable && DisableReason.IsEmpty())
	{
		Context.AddError(LOCTEXT("EditCondition_NoDisableReason",
			"DisableReason is required when Action == Disable so the user sees a reason."));
		Result = EDataValidationResult::Invalid;
	}
	if (Action != EGameSettingEditAction::Disable && DevReason.IsEmpty())
	{
		Context.AddWarning(LOCTEXT("EditCondition_NoDevReason",
			"DevReason is empty. Provide one so dev-only logs and 'why is this hidden' overlays have context."));
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
