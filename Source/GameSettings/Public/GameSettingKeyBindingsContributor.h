// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingsAutoContributor.h"
#include "UObject/PrimaryAssetId.h"

#include "GameSettingKeyBindingsContributor.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Builds the input rebinding settings from the local player's Enhanced Input
 * user settings.
 *
 * Key bindings cannot be authored as a static data asset. The set of mappable
 * keys depends on which input mapping contexts are registered, which depends on
 * which game features are active. So this runs as a code auto-contributor: on
 * every local player it reads the current mappable key profiles and emits one
 * UGameSettingValueKeyBinding per action, grouped by display category, under a
 * Key Bindings collection.
 *
 * Device-agnostic. It enumerates every mapping with permissive query options,
 * so keyboard, mouse, gamepad, and VR controller bindings all appear.
 *
 * The list is a snapshot taken when the registry is built. A game feature that
 * registers new keys after that shows up on the next UGameSettingRegistry::Regenerate
 * (a settings screen reopen is the usual trigger).
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
	 * Where the Key Bindings collection lands. Invalid (default) makes it a top
	 * level tab. Set it to an existing tab or section id to nest the bindings
	 * under that container instead. Config-driven so a project can re-home it
	 * without a code change.
	 */
	UPROPERTY(Config, EditAnywhere, Category = "Key Bindings")
	FPrimaryAssetId ParentContainer;
};

#undef UE_API
