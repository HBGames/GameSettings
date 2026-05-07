// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsBinding.h"
#include "Contributions/GameSettingsTypedContribution.h"

#include "GameSettingsContribution_Toggle.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Boolean toggle setting bound through FGameSettingsBinding to a subsystem
 * getter/setter pair. Both functions must be UFUNCTION-reflected and take/return
 * a bool.
 */
UCLASS(MinimalAPI, DisplayName = "Game Setting Toggle")
class UGameSettingsContribution_Toggle : public UGameSettingsTypedContribution
{
	GENERATED_BODY()
public:
	/** Tag of the parent tab. If empty or unregistered, the setting is added at top level. */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FGameplayTag ParentTab;

	/** Default value if the user resets to default. */
	UPROPERTY(EditAnywhere, Category = "Value")
	bool bDefaultValue = false;

	UPROPERTY(EditAnywhere, Category = "Value")
	FGameSettingsBinding Binding;

	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

#undef UE_API
