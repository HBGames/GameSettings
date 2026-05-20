// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsBinding.h"
#include "Contributions/GameSettingsRowContribution.h"

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
class UGameSettingsDiscreteOptionsProvider;
class UGameSettingValueDiscreteDynamic;

UCLASS(MinimalAPI, DisplayName = "Game Setting Discrete")
class UGameSettingsContribution_Discrete : public UGameSettingsRowContribution
{
	GENERATED_BODY()
public:
	/**
	 * When true (default), Reset-To-Default uses the value the binding's getter
	 * returns on TargetClass's CDO - the C++-declared property default. Uncheck
	 * to use DefaultValue as an explicit override.
	 */
	UPROPERTY(EditAnywhere, Category = "Value")
	bool bUseClassDefaultValue = true;

	/** Explicit Reset-To-Default option value. Must match one of Options. Only used when bUseClassDefaultValue is false. */
	UPROPERTY(EditAnywhere, Category = "Value", meta = (EditCondition = "!bUseClassDefaultValue"))
	FString DefaultValue;

	/** Static option list. Ignored when OptionsProvider is set or when SettingClass self-manages. */
	UPROPERTY(EditAnywhere, Category = "Value")
	TArray<FGameSettingsDiscreteOption> Options;

	/**
	 * Optional runtime options provider. When set, generates the option list
	 * per LocalPlayer and overrides the static Options array.
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "Value")
	TObjectPtr<UGameSettingsDiscreteOptionsProvider> OptionsProvider;

	/**
	 * Concrete setting class to instantiate. Default is the generic
	 * UGameSettingValueDiscreteDynamic, which uses Options/OptionsProvider
	 * and FGameSettingsBinding. Pick a subclass to take over fully (e.g.
	 * to handle a "Custom" preset that bypasses the binding).
	 */
	UPROPERTY(EditAnywhere, Category = "Value", meta = (MetaClass = "/Script/GameSettings.GameSettingValueDiscreteDynamic"))
	TSubclassOf<UGameSettingValueDiscreteDynamic> SettingClass;

	UPROPERTY(EditAnywhere, Category = "Value")
	FGameSettingsBinding Binding;

	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override;

	UE_API virtual FPrimaryAssetType GetContributionPrimaryAssetType() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;

	/**
	 * A custom (non-default) SettingClass takes over the value entirely and
	 * self-manages options, so the binding / options / class-default fields
	 * are meaningless then - grey them out in the details panel.
	 */
	UE_API virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

	static UE_API const FPrimaryAssetType ContributionPrimaryAssetType;
};

#undef UE_API
