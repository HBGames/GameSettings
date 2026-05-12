// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingsContribution_Discrete.h"
#include "UObject/Object.h"

#include "GameSettingsDiscreteOptionsProvider.generated.h"

#define UE_API GAMESETTINGS_API

class ULocalPlayer;

/**
 * Plug-in interface for emitting option lists at runtime instead of
 * authoring them statically on a UGameSettingsContribution_Discrete.
 *
 * Subclass and override GenerateOptions to enumerate (audio devices,
 * supported cultures, scalability levels that survive a platform check,
 * whatever). Pick the subclass in the contribution asset's
 * OptionsProvider field; when set, it overrides the static Options array.
 *
 * Providers are inline-instanced and edited inside the contribution
 * asset, so they have no separate uasset lifetime.
 */
UCLASS(MinimalAPI, BlueprintType, Abstract, EditInlineNew, DefaultToInstanced)
class UGameSettingsDiscreteOptionsProvider : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Populate OutOptions. Called from UGameSettingsContribution_Discrete::Apply
	 * for each LocalPlayer that activates the contribution.
	 */
	UE_API virtual void GenerateOptions(const ULocalPlayer* LocalPlayer, TArray<FGameSettingsDiscreteOption>& OutOptions) const
		PURE_VIRTUAL(UGameSettingsDiscreteOptionsProvider::GenerateOptions, );
};

#undef UE_API
