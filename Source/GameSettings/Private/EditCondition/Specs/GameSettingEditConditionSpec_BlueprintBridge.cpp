// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "EditCondition/Specs/GameSettingEditConditionSpec_BlueprintBridge.h"

#include "EditCondition/WhenCondition.h"
#include "GameSettingFilterState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingEditConditionSpec_BlueprintBridge)

TSharedPtr<FGameSettingEditCondition> UGameSettingEditConditionSpec_BlueprintBridge::BuildCondition(
	UGameSettingRegistry& Registry, UGameSetting& Owner) const
{
	TWeakObjectPtr<const ThisClass> WeakSpec(this);
	return MakeShared<FWhenCondition>(
		[WeakSpec](const ULocalPlayer* LocalPlayer, FGameSettingEditableState& InOutState)
		{
			const ThisClass* Spec = WeakSpec.Get();
			if (!Spec)
			{
				return;
			}
			FGameSettingEditableStateBP Wrapper(&InOutState);
			Spec->BP_EvaluateState(LocalPlayer, Wrapper);
		});
}

FString UGameSettingEditConditionSpec_BlueprintBridge::DebugDescribe() const
{
	return FString::Printf(TEXT("BP(%s)"), *GetClass()->GetName());
}

// --- BP helper library ----------------------------------------------------

void UGameSettingEditConditionLibrary::Disable(FGameSettingEditableStateBP& State, const FText& Reason)
{
	if (State.Native) State.Native->Disable(Reason);
}

void UGameSettingEditConditionLibrary::Hide(FGameSettingEditableStateBP& State, const FString& DevReason)
{
	if (State.Native) State.Native->Hide(DevReason);
}

void UGameSettingEditConditionLibrary::Kill(FGameSettingEditableStateBP& State, const FString& DevReason)
{
	if (State.Native) State.Native->Kill(DevReason);
}

void UGameSettingEditConditionLibrary::DisableOption(FGameSettingEditableStateBP& State, const FString& OptionValue)
{
	if (State.Native) State.Native->DisableOption(OptionValue);
}

void UGameSettingEditConditionLibrary::UnableToReset(FGameSettingEditableStateBP& State)
{
	if (State.Native) State.Native->UnableToReset();
}

void UGameSettingEditConditionLibrary::HideFromAnalytics(FGameSettingEditableStateBP& State)
{
	if (State.Native) State.Native->HideFromAnalytics();
}
