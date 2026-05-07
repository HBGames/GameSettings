// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "ViewModels/GameSettingActionViewModel.h"

#include "GameSettingAction.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingActionViewModel)

void UGameSettingActionViewModel::Execute()
{
	if (UGameSettingAction* Action = Cast<UGameSettingAction>(Setting))
	{
		Action->ExecuteAction();
	}
}

void UGameSettingActionViewModel::RefreshFromSetting()
{
	Super::RefreshFromSetting();

	UGameSettingAction* Action = Cast<UGameSettingAction>(Setting);
	if (!Action)
	{
		return;
	}

	const FText NewActionText = Action->GetActionText();
	if (!ActionText.EqualTo(NewActionText))
	{
		ActionText = NewActionText;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetActionText);
	}

	const FGameplayTag NewNamedAction = Action->GetNamedAction();
	if (NamedAction != NewNamedAction)
	{
		NamedAction = NewNamedAction;
		UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetNamedAction);
	}
}
