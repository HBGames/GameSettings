// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's UGameSettingPressAnyKey (Copyright Epic Games, Inc.).

#pragma once

#include "CommonActivatableWidget.h"

#include "GameSettingPressAnyKey.generated.h"

#define UE_API GAMESETTINGS_API

struct FKey;

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

	DECLARE_EVENT_OneParam(UGameSettingPressAnyKey, FOnKeySelected, FKey);
	FOnKeySelected OnKeySelected;

	DECLARE_EVENT(UGameSettingPressAnyKey, FOnKeySelectionCanceled);
	FOnKeySelectionCanceled OnKeySelectionCanceled;

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
