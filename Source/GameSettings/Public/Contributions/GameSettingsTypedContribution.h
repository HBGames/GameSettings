// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingsContribution.h"

#include "GameSettingsTypedContribution.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSetting;

/**
 * Intermediate base for the typed designer-facing contribution subclasses
 * (Tab/Toggle/Scalar/Discrete/Action). Owns the shared display fields so
 * each leaf class only declares what's specific to it.
 *
 * Identity is the contribution's own FPrimaryAssetId (asset name + typed
 * primary asset type), surfaced through GetPrimaryAssetId(). Renaming the
 * asset moves the identity automatically via asset manager redirects.
 */
UCLASS(MinimalAPI, Abstract)
class UGameSettingsTypedContribution : public UGameSettingsContribution
{
	GENERATED_BODY()
public:
	/** Localized name shown in the settings list. Required. */
	UPROPERTY(EditAnywhere, Category = "Display")
	FText DisplayName;

	/** Rich-text description shown in the detail panel. Optional. */
	UPROPERTY(EditAnywhere, Category = "Display", DisplayName = "Description", meta = (MultiLine = "true"))
	FText DescriptionRichText;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
	/** Apply identity + display fields onto a freshly-NewObject'd setting. */
	UE_API void ConfigureBaseSetting(UGameSetting& Setting) const;
};

#undef UE_API
