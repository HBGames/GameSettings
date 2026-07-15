// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's UGameSettingPressAnyKey (Copyright Epic Games, Inc.).

#include "Widgets/Misc/GameSettingPressAnyKey.h"

#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingPressAnyKey)

class ICursor;

class FSettingsPressAnyKeyInputPreProcessor : public IInputProcessor
{
public:
	explicit FSettingsPressAnyKeyInputPreProcessor(bool bInCancelOnGamepadKey)
		: bCancelOnGamepadKey(bInCancelOnGamepadKey)
	{
	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override { }

	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		HandleKey(InKeyEvent.GetKey());
		return true;
	}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override
	{
		return true;
	}

	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override
	{
		// Swallow analog axes so a gamepad stick drift does not register as a key.
		return true;
	}

	virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		HandleKey(MouseEvent.GetEffectingButton());
		return true;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		return true;
	}

	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		HandleKey(MouseEvent.GetEffectingButton());
		return true;
	}

	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& InWheelEvent, const FPointerEvent* InGestureEvent) override
	{
		if (InWheelEvent.GetWheelDelta() != 0)
		{
			const FKey Key = InWheelEvent.GetWheelDelta() < 0 ? EKeys::MouseScrollDown : EKeys::MouseScrollUp;
			HandleKey(Key);
		}
		return true;
	}

	DECLARE_MULTICAST_DELEGATE(FSettingsPressAnyKeyInputPreProcessorCanceled);
	FSettingsPressAnyKeyInputPreProcessorCanceled OnKeySelectionCanceled;

	DECLARE_MULTICAST_DELEGATE_OneParam(FSettingsPressAnyKeyInputPreProcessorKeySelected, FKey);
	FSettingsPressAnyKeyInputPreProcessorKeySelected OnKeySelected;

private:
	void HandleKey(const FKey& Key)
	{
		if (Key == EKeys::LeftCommand || Key == EKeys::RightCommand)
		{
			// Ignore. The command keys are modifiers, not a binding on their own.
		}
		else if (Key == EKeys::Escape || Key.IsTouch() || (bCancelOnGamepadKey && Key.IsGamepadKey()))
		{
			OnKeySelectionCanceled.Broadcast();
		}
		else
		{
			OnKeySelected.Broadcast(Key);
		}
	}

	bool bCancelOnGamepadKey = false;
};

UGameSettingPressAnyKey::UGameSettingPressAnyKey(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

void UGameSettingPressAnyKey::NativeOnActivated()
{
	Super::NativeOnActivated();

	bKeySelected = false;

	InputProcessor = MakeShared<FSettingsPressAnyKeyInputPreProcessor>(bCancelOnGamepadKey);
	InputProcessor->OnKeySelected.AddUObject(this, &ThisClass::HandleKeySelected);
	InputProcessor->OnKeySelectionCanceled.AddUObject(this, &ThisClass::HandleKeySelectionCanceled);
	FSlateApplication::Get().RegisterInputPreProcessor(InputProcessor, 0);
}

void UGameSettingPressAnyKey::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);
	}
}

void UGameSettingPressAnyKey::HandleKeySelected(FKey InKey)
{
	if (!bKeySelected)
	{
		bKeySelected = true;
		Dismiss([this, InKey]() {
			OnKeySelected.Broadcast(InKey);
		});
	}
}

void UGameSettingPressAnyKey::HandleKeySelectionCanceled()
{
	if (!bKeySelected)
	{
		bKeySelected = true;
		Dismiss([this]() {
			OnKeySelectionCanceled.Broadcast();
		});
	}
}

void UGameSettingPressAnyKey::Dismiss(TFunction<void()> PostDismissCallback)
{
	// Delay a tick so we are done processing input before tearing down.
	FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateWeakLambda(this, [this, PostDismissCallback](float DeltaTime)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_UGameSettingPressAnyKey_Dismiss);

		FSlateApplication::Get().UnregisterInputPreProcessor(InputProcessor);

		DeactivateWidget();

		PostDismissCallback();

		return false;
	}));
}
