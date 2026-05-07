// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsBinding.h"
#include "Contributions/GameSettingsTypedContribution.h"

#include "GameSettingsContribution_Discrete.generated.h"

#define UE_API GAMESETTINGS_API

/** One option in a discrete-choice setting. */
USTRUCT(BlueprintType)
struct FGameSettingsDiscreteOption
{
	GENERATED_BODY()

	/** Stored value (passed to the setter, returned from the getter). */
	UPROPERTY(EditAnywhere, Category = "Option")
	FString Value;

	/** Localized label displayed for this option. */
	UPROPERTY(EditAnywhere, Category = "Option")
	FText DisplayText;
};

/**
 * Multi-option setting (e.g. graphics quality preset, language picker)
 * bound through FGameSettingsBinding to a subsystem getter/setter pair.
 * The getter must return a string and the setter must accept one; values
 * round-trip through PropertyPathHelpers' string conversion.
 */
UCLASS(MinimalAPI, DisplayName = "Game Setting Discrete")
class UGameSettingsContribution_Discrete : public UGameSettingsTypedContribution
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, Category = "Identity")
	FGameplayTag ParentTab;

	/** Default option's stored value. Must match one of the entries in Options. */
	UPROPERTY(EditAnywhere, Category = "Value")
	FString DefaultValue;

	UPROPERTY(EditAnywhere, Category = "Value")
	TArray<FGameSettingsDiscreteOption> Options;

	UPROPERTY(EditAnywhere, Category = "Value")
	FGameSettingsBinding Binding;

	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

#undef UE_API
