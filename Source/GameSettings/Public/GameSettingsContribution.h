// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingHandle.h"
#include "UObject/Object.h"

#include "GameSettingsContribution.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSettingRegistry;

/**
 * A unit of settings contribution. Subclasses describe a setting (or group
 * of settings) and apply themselves to a registry.
 *
 * Three contribution paths consume this type:
 *   1. UGameFeatureAction_RegisterGameSettings, where GFPs put an Instanced
 *      array on their UGameFeatureData and the action applies it on
 *      activation.
 *   2. UGameSettingsAutoContributor, an auto-discovered CDO subclass
 *      applied at module startup.
 *   3. Direct C++. Call Apply() yourself with a registry of your choice.
 *
 * Apply() must push every allocated handle onto OutHandles. The caller
 * keeps those handles and removes by handle on teardown; that's how the
 * system avoids leaks across plugin unload.
 */
UCLASS(MinimalAPI, BlueprintType, Abstract, EditInlineNew, DefaultToInstanced)
class UGameSettingsContribution : public UObject
{
	GENERATED_BODY()
public:
	/**
	 * Add this contribution's settings to the registry. Append every
	 * resulting handle to OutHandles so the caller can later call
	 * UGameSettingRegistry::RemoveByHandle on teardown.
	 */
	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
		PURE_VIRTUAL(UGameSettingsContribution::Apply, );
};

#undef UE_API
