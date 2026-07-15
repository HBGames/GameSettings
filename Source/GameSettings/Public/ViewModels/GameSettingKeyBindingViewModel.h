// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "InputCoreTypes.h"
#include "ViewModels/GameSettingViewModel.h"

#include "GameSettingKeyBindingViewModel.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSettingValueKeyBinding;

/**
 * View model for a key binding row. Wraps UGameSettingValueKeyBinding.
 *
 * Exposes the current key text for the primary and secondary slots as read
 * only FieldNotify fields, plus the primitives a bind row widget needs to run
 * a rebind: capture a key with UGameSettingPressAnyKey, check for a duplicate
 * with GetActionsBoundToKey, then commit with ChangeBinding. The presentation
 * flow lives in the widget, the same split the discrete and toggle VMs use.
 *
 * Device-agnostic. The slots hold whatever key is bound, keyboard, mouse,
 * gamepad, or VR controller button.
 */
UCLASS(MinimalAPI, BlueprintType, DisplayName = "Game Setting Key Binding")
class UGameSettingKeyBindingViewModel : public UGameSettingViewModel
{
	GENERATED_BODY()
public:
	/** Slot index used for the primary binding. */
	static constexpr int32 PrimarySlot = 0;

	/** Slot index used for the secondary binding. */
	static constexpr int32 SecondarySlot = 1;

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Value")
	const FText& GetPrimaryKeyText() const { return PrimaryKeyText; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Value")
	const FText& GetSecondaryKeyText() const { return SecondaryKeyText; }

	UFUNCTION(BlueprintCallable, FieldNotify, Category = "State")
	bool IsCustomized() const { return bIsCustomized; }

	/** Current key text for an arbitrary slot, for widgets that show more than two. */
	UFUNCTION(BlueprintCallable, Category = "Value")
	UE_API FText GetKeyTextForSlot(int32 Slot) const;

	/**
	 * Commit a rebind. Accepts any key including gamepad and VR controller
	 * buttons. Returns false only when the input user settings are unreachable.
	 */
	UFUNCTION(BlueprintCallable, Category = "Value")
	UE_API bool ChangeBinding(int32 Slot, FKey NewKey);

	/** Reset this action's bindings to their defaults. */
	UFUNCTION(BlueprintCallable, Category = "Value")
	UE_API void ResetBindingToDefault();

	/**
	 * Names of every action already bound to Key. Empty means the key is free.
	 * Feed the result to the already-bound warning before committing a rebind.
	 */
	UFUNCTION(BlueprintCallable, Category = "Value")
	UE_API TArray<FName> GetActionsBoundToKey(int32 Slot, FKey Key) const;

protected:
	//~UGameSettingViewModel interface
	UE_API virtual void RefreshFromSetting() override;
	//~End of UGameSettingViewModel interface

private:
	UGameSettingValueKeyBinding* GetKeyBinding() const;

	FText PrimaryKeyText;
	FText SecondaryKeyText;
	bool bIsCustomized = false;
};

#undef UE_API
