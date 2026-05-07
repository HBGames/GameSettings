// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingsContribution.h"
#include "GameplayTagContainer.h"

#include "GameSettingsTypedContribution.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSetting;

/**
 * Intermediate base for the typed designer-facing contribution subclasses
 * (Tab/Toggle/Scalar/Discrete/Action). Owns the shared identity and display
 * fields so each leaf class only declares what's specific to it.
 */
UCLASS(MinimalAPI, Abstract, EditInlineNew, DefaultToInstanced)
class UGameSettingsTypedContribution : public UGameSettingsContribution
{
	GENERATED_BODY()
public:
	/** Identity tag for this setting. Required. */
	UPROPERTY(EditAnywhere, Category = "Identity")
	FGameplayTag SettingId;

	/** Localized name shown in the settings list. Required. */
	UPROPERTY(EditAnywhere, Category = "Display")
	FText DisplayName;

	/** Rich-text description shown in the detail panel. Optional. */
	UPROPERTY(EditAnywhere, Category = "Display", meta = (MultiLine = "true"))
	FText DescriptionRichText;

protected:
	/** Apply identity + display fields onto a freshly-NewObject'd setting. */
	UE_API void ConfigureBaseSetting(UGameSetting& Setting) const;
};

#undef UE_API
