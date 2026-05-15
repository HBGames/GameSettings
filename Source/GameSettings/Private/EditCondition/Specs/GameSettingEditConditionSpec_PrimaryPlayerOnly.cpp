// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "EditCondition/Specs/GameSettingEditConditionSpec_PrimaryPlayerOnly.h"

#include "EditCondition/WhenPlayingAsPrimaryPlayer.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingEditConditionSpec_PrimaryPlayerOnly)

#define LOCTEXT_NAMESPACE "GameSettings"

TSharedPtr<FGameSettingEditCondition> UGameSettingEditConditionSpec_PrimaryPlayerOnly::BuildCondition(
	UGameSettingRegistry& Registry, UGameSetting& Owner) const
{
	return FWhenPlayingAsPrimaryPlayer::Get();
}

FString UGameSettingEditConditionSpec_PrimaryPlayerOnly::DebugDescribe() const
{
	return TEXT("PrimaryPlayerOnly");
}

#if WITH_EDITOR
EDataValidationResult UGameSettingEditConditionSpec_PrimaryPlayerOnly::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (Action != EGameSettingEditAction::Disable)
	{
		Context.AddError(LOCTEXT("PrimaryPlayerOnly_DisableOnly",
			"PrimaryPlayerOnly: only Action == Disable is supported (the underlying condition hard-codes Disable semantics)."));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
