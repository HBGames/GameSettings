// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "EditCondition/GameSettingEditConditionSpec.h"

#include "GameSettingEditConditionSpec_DisableDiscreteOption.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Disable specific options of the owning Discrete setting when an inner
 * Predicate fires. Intended for parental-controls / region-gating flows.
 *
 * Composition: Predicate is one of the four primitive specs (DependsOnToggle
 * / DependsOnDiscrete / DependsOnScalar / PlatformTrait). The Predicate's
 * Action / DisableReason fields are ignored; this outer spec calls
 * DisableOption on the editable state for each value in
 * OptionValuesToDisable when the predicate would have fired.
 *
 * This spec's own Action / DisableReason are also ignored (the disabled-
 * option list is the only output). Surface user-facing reasons via the
 * disabled-options tooltip in your discrete-option widget.
 */
UCLASS(MinimalAPI, DisplayName = "Disable Discrete Options When")
class UGameSettingEditConditionSpec_DisableDiscreteOption : public UGameSettingEditConditionSpec
{
	GENERATED_BODY()

public:
	/**
	 * Option Values (not display labels) to remove from this Discrete
	 * setting's selectable list when the Predicate fires.
	 */
	UPROPERTY(EditAnywhere, Category = "Disable")
	TArray<FString> OptionValuesToDisable;

	/**
	 * Inner condition. One of the four primitive specs; nested
	 * DisableDiscreteOption is rejected at validation time.
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "Disable")
	TObjectPtr<UGameSettingEditConditionSpec> Predicate;

	UE_API virtual void GetSettingDependencies(TArray<FPrimaryAssetId>& OutIds) const override;
	UE_API virtual TSharedPtr<FGameSettingEditCondition> BuildCondition(UGameSettingRegistry& Registry, UGameSetting& Owner) const override;
	UE_API virtual FString DebugDescribe() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

#undef UE_API
