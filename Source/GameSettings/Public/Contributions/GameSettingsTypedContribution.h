// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingsContribution.h"

#include "GameSettingsTypedContribution.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSetting;
class UGameSettingEditConditionSpec;

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

	/**
	 * Edit-condition specs applied to the runtime setting at registration
	 * time. Evaluated in array order as implicit AND - any spec can hide,
	 * disable, kill, or per-option-disable the setting. Cross-setting
	 * dependencies are resolved via the registry's deferred-wiring queue,
	 * so arrival order across contributions is safe.
	 *
	 * Add a spec with the + button, then pick a subclass:
	 *   - Depends On Toggle / Discrete / Scalar: gate on another setting's value
	 *   - Platform Trait: gate on a CommonUI Platform.Trait.* tag
	 *   - Primary Player Only: greyed out for splitscreen guests
	 *   - Disable Discrete Options When: hide specific options of THIS setting
	 *   - Blueprint Condition Bridge: BP-authored predicate for one-off rules
	 *
	 * See Docs/EDIT_CONDITIONS.md for the full reference.
	 */
	UPROPERTY(EditAnywhere, Instanced, Category = "Edit Conditions",
		meta = (DisplayName = "Edit Conditions",
			ToolTip = "Visibility / enabled rules applied in array order (implicit AND)."))
	TArray<TObjectPtr<UGameSettingEditConditionSpec>> EditConditions;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
	/** Apply identity + display fields onto a freshly-NewObject'd setting. */
	UE_API void ConfigureBaseSetting(UGameSetting& Setting) const;
};

#undef UE_API
