// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "EditCondition/Specs/GameSettingEditConditionSpec_PlatformTrait.h"

#include "EditCondition/WhenPlatformHasTrait.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingEditConditionSpec_PlatformTrait)

#define LOCTEXT_NAMESPACE "GameSettings"

TSharedPtr<FGameSettingEditCondition> UGameSettingEditConditionSpec_PlatformTrait::BuildCondition(
	UGameSettingRegistry& Registry, UGameSetting& Owner) const
{
	const bool bDisableOnly = (Action == EGameSettingEditAction::Disable);

	if (When == EGameSettingPlatformPresence::IfMissing)
	{
		return bDisableOnly
			? FWhenPlatformHasTrait::DisableIfMissing(VisibilityTag, DisableReason)
			: FWhenPlatformHasTrait::KillIfMissing(VisibilityTag, DevReason);
	}
	return bDisableOnly
		? FWhenPlatformHasTrait::DisableIfPresent(VisibilityTag, DisableReason)
		: FWhenPlatformHasTrait::KillIfPresent(VisibilityTag, DevReason);
}

FString UGameSettingEditConditionSpec_PlatformTrait::DebugDescribe() const
{
	return FString::Printf(TEXT("PlatformTrait(%s, %s)"),
		*VisibilityTag.ToString(),
		When == EGameSettingPlatformPresence::IfMissing ? TEXT("if-missing") : TEXT("if-present"));
}

#if WITH_EDITOR
EDataValidationResult UGameSettingEditConditionSpec_PlatformTrait::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!VisibilityTag.IsValid())
	{
		Context.AddError(LOCTEXT("PlatformTrait_NoTag",
			"PlatformTrait: VisibilityTag is required."));
		Result = EDataValidationResult::Invalid;
	}

	// Hide doesn't map cleanly onto FWhenPlatformHasTrait; reject it.
	if (Action == EGameSettingEditAction::Hide)
	{
		Context.AddError(LOCTEXT("PlatformTrait_NoHide",
			"PlatformTrait: Action == Hide is not supported. Use Kill (hide everywhere) or Disable (grey out)."));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
