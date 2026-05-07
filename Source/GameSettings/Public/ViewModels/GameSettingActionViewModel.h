// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "ViewModels/GameSettingViewModel.h"

#include "GameSettingActionViewModel.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * View model for a button (action) setting. Wraps UGameSettingAction.
 *
 * Bind a button widget's OnClicked to Execute(). The action setting
 * handles dispatch (custom delegate or NamedAction broadcast on the
 * registry's OnSettingNamedActionEvent).
 */
UCLASS(MinimalAPI, BlueprintType, DisplayName = "Game Setting Action")
class UGameSettingActionViewModel : public UGameSettingViewModel
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Display")
	const FText& GetActionText() const { return ActionText; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Action")
	FGameplayTag GetNamedAction() const { return NamedAction; }

	/** Fire the action. Bind to a button's OnClicked. */
	UFUNCTION(BlueprintCallable, Category = "Action")
	UE_API void Execute();

protected:
	UE_API virtual void RefreshFromSetting() override;

private:
	FText ActionText;
	FGameplayTag NamedAction;
};

#undef UE_API
