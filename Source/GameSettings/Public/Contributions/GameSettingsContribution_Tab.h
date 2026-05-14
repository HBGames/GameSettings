// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsTypedContribution.h"

#include "GameSettingsContribution_Tab.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Registers a top-level tab (a UGameSettingCollection) keyed by SettingId.
 * Other contributions reference SettingId as their ParentContainer to nest
 * under it - directly for rows, or as the parent of a Section which then
 * holds rows. Tabs themselves don't bind to anything; they're containers.
 */
UCLASS(MinimalAPI, DisplayName = "Game Setting Tab")
class UGameSettingsContribution_Tab : public UGameSettingsTypedContribution
{
	GENERATED_BODY()
public:
	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override;

	UE_API virtual FPrimaryAssetType GetContributionPrimaryAssetType() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	/** "GameSettingsTab" - one of the AllowedTypes for ParentContainer pickers (alongside GameSettingsSection). */
	static UE_API const FPrimaryAssetType ContributionPrimaryAssetType;
};

#undef UE_API
