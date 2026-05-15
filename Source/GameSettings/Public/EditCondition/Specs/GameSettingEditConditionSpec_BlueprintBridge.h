// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "EditCondition/GameSettingEditConditionSpec.h"
#include "Templates/SharedPointer.h"

#include "GameSettingEditConditionSpec_BlueprintBridge.generated.h"

#define UE_API GAMESETTINGS_API

class FGameSettingEditableState;
class ULocalPlayer;

/**
 * BP-friendly mirror of FGameSettingEditableState. Exposes the four mutation
 * methods Blueprint authors need; the bridge spec marshals values back into
 * the native state after BP returns.
 *
 * The struct is intentionally lightweight - it holds a raw pointer to the
 * underlying native state during GatherEditState callbacks and is only valid
 * inside that call. Do not store or pass it outside the event.
 */
USTRUCT(BlueprintType)
struct FGameSettingEditableStateBP
{
	GENERATED_BODY()

	FGameSettingEditableStateBP() = default;
	explicit FGameSettingEditableStateBP(FGameSettingEditableState* InNative) : Native(InNative) {}

	FGameSettingEditableState* Native = nullptr;
};

/**
 * Spec subclass that delegates BuildCondition to a Blueprint event so
 * designers can author one-off conditions without C++. C++ subclasses should
 * extend UGameSettingEditConditionSpec directly; this class exists only as
 * the BP authoring entry point.
 *
 * Override BP_EvaluateState in a BP subclass. Call the BP-exposed helpers
 * on the supplied state to mutate it (Disable, Hide, Kill, DisableOption,
 * UnableToReset, HideFromAnalytics).
 */
UCLASS(MinimalAPI, Blueprintable, DisplayName = "Blueprint Condition Bridge")
class UGameSettingEditConditionSpec_BlueprintBridge : public UGameSettingEditConditionSpec
{
	GENERATED_BODY()

public:
	/**
	 * Override in a BP subclass. Invoked every time the owning setting's
	 * editable state is recomputed. Mutate State via the helper UFUNCTIONs
	 * on UGameSettingEditConditionLibrary.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Edit Condition")
	void BP_EvaluateState(const ULocalPlayer* LocalPlayer, UPARAM(ref) FGameSettingEditableStateBP& State) const;

	UE_API virtual TSharedPtr<FGameSettingEditCondition> BuildCondition(
		UGameSettingRegistry& Registry, UGameSetting& Owner) const override;
	UE_API virtual FString DebugDescribe() const override;
};

/**
 * BP-callable helpers for mutating FGameSettingEditableStateBP from inside
 * a Blueprint condition. Routes back to the native state pointer; safe to
 * call only inside BP_EvaluateState.
 */
UCLASS(MinimalAPI)
class UGameSettingEditConditionLibrary : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Edit Condition", meta = (DisplayName = "Disable"))
	static UE_API void Disable(UPARAM(ref) FGameSettingEditableStateBP& State, const FText& Reason);

	UFUNCTION(BlueprintCallable, Category = "Edit Condition", meta = (DisplayName = "Hide"))
	static UE_API void Hide(UPARAM(ref) FGameSettingEditableStateBP& State, const FString& DevReason);

	UFUNCTION(BlueprintCallable, Category = "Edit Condition", meta = (DisplayName = "Kill"))
	static UE_API void Kill(UPARAM(ref) FGameSettingEditableStateBP& State, const FString& DevReason);

	UFUNCTION(BlueprintCallable, Category = "Edit Condition", meta = (DisplayName = "Disable Option"))
	static UE_API void DisableOption(UPARAM(ref) FGameSettingEditableStateBP& State, const FString& OptionValue);

	UFUNCTION(BlueprintCallable, Category = "Edit Condition", meta = (DisplayName = "Unable To Reset"))
	static UE_API void UnableToReset(UPARAM(ref) FGameSettingEditableStateBP& State);

	UFUNCTION(BlueprintCallable, Category = "Edit Condition", meta = (DisplayName = "Hide From Analytics"))
	static UE_API void HideFromAnalytics(UPARAM(ref) FGameSettingEditableStateBP& State);
};

#undef UE_API
