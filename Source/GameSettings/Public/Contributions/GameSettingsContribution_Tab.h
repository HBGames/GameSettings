// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsTypedContribution.h"

#include "GameSettingsContribution_Tab.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Registers a top-level tab (a UGameSettingCollection) keyed by SettingId.
 * Other contributions reference SettingId as their ParentTab to nest
 * under it. Tabs themselves don't bind to anything; they're containers.
 */
UCLASS(MinimalAPI, DisplayName = "Game Setting Tab")
class UGameSettingsContribution_Tab : public UGameSettingsTypedContribution
{
	GENERATED_BODY()
public:
	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

#undef UE_API
