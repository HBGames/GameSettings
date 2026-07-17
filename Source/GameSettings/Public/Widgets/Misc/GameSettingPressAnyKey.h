// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's UGameSettingPressAnyKey (Copyright Epic Games, Inc.).

#pragma once

#include "CommonActivatableWidget.h"
#include "InputCoreTypes.h"

#include "GameSettingPressAnyKey.generated.h"

#define UE_API GAMESETTINGS_API

/** Fires with the captured key once the press-any-key modal resolves. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGameSettingKeySelected, FKey, SelectedKey);

/** Fires when the capture is cancelled. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FGameSettingKeySelectionCanceled);

/**
 * Modal that captures the next key press and reports it. Used by a key binding
 * row to grab a replacement key.
 *
 * Device-agnostic. Escape and touch always cancel. Everything else, including
 * gamepad and VR controller buttons, is reported through OnKeySelected. A
 * keyboard-and-mouse screen can set bCancelOnGamepadKey so a stray gamepad
 * press backs out instead of binding. A gamepad or VR screen leaves it off.
 */
UCLASS(MinimalAPI, Abstract)
class UGameSettingPressAnyKey : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	UE_API UGameSettingPressAnyKey(const FObjectInitializer& Initializer);

	/** Fires with the captured key once the modal resolves. Bind this in the bind-row widget BP. */
	UPROPERTY(BlueprintAssignable, Category = "Key Binding")
	FGameSettingKeySelected OnKeySelected;

	/** Fires when capture is cancelled (Escape, touch, or a gamepad key when bCancelOnGamepadKey). */
	UPROPERTY(BlueprintAssignable, Category = "Key Binding")
	FGameSettingKeySelectionCanceled OnKeySelectionCanceled;

protected:
	UE_API virtual void NativeOnActivated() override;
	UE_API virtual void NativeOnDeactivated() override;

	UE_API void HandleKeySelected(FKey InKey);
	UE_API void HandleKeySelectionCanceled();

	UE_API void Dismiss(TFunction<void()> PostDismissCallback);

	/**
	 * When true a gamepad key cancels the capture instead of being reported.
	 * Leave false to rebind gamepad and VR controller buttons.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Key Binding")
	bool bCancelOnGamepadKey = false;

private:
	bool bKeySelected = false;
	TSharedPtr<class FSettingsPressAnyKeyInputPreProcessor> InputProcessor;
};

#undef UE_API
