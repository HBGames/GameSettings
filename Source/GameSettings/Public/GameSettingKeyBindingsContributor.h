// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingsAutoContributor.h"
#include "UObject/PrimaryAssetId.h"

#include "GameSettingKeyBindingsContributor.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Emits one UGameSettingValueKeyBinding per player mappable key, grouped by
 * display category. Runs as code because the available keys depend on which
 * input mapping contexts are active, so they cannot be a static asset. Covers
 * every device, so keyboard, mouse, gamepad, and VR all appear.
 *
 * Categories sort by display name. Rows sort by SortOrder then display name.
 * See UGameSettingKeyBindingMetadata to pin a row.
 *
 * The list is a snapshot from when the registry was built. New keys appear on
 * the next Regenerate, usually a settings reopen.
 */
UCLASS(MinimalAPI, BlueprintType, Config = Game, DefaultConfig)
class UGameSettingKeyBindingsContributor : public UGameSettingsAutoContributor
{
	GENERATED_BODY()
public:
	//~UGameSettingsContribution interface
	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override;
	//~End of UGameSettingsContribution interface

	/**
	 * Container the rows nest under. Invalid falls back to a native "Key Bindings"
	 * tab. Point it at an authored Tab or Section id to control the name, sort
	 * priority, icon, and gating like any other tab.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Key Bindings")
	FPrimaryAssetId ParentContainer;

	/** Sort priority of the fallback tab. Lower sorts first. Unused when ParentContainer is set. */
	UPROPERTY(Config, EditAnywhere, Category = "Key Bindings")
	int32 FallbackTabSortPriority = 0;
};

#undef UE_API
