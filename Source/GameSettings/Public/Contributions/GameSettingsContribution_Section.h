// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsRowContribution.h"

#include "GameSettingsContribution_Section.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Container that nests inside a Tab (or another Section) and groups related
 * row contributions under a labeled heading. Rows reference the section's
 * primary asset id as their ParentContainer to nest inside it instead of
 * landing directly under the tab.
 *
 * Sections inherit UGameSettingsRowContribution so they share the
 * ParentContainer / DisplayName / DescriptionRichText shape with rows. They
 * don't bind to any storage; they exist only as a grouping node in the tree.
 *
 * Arrival order between tabs, sections, and rows is irrelevant: the registry
 * holds anything whose parent hasn't shown up yet in a deferred-placement
 * queue and reparents it once the parent arrives.
 */
UCLASS(MinimalAPI, DisplayName = "Game Setting Section")
class UGameSettingsContribution_Section : public UGameSettingsRowContribution
{
	GENERATED_BODY()
public:
	UE_API virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override;

	UE_API virtual FPrimaryAssetType GetContributionPrimaryAssetType() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

	/** "GameSettingsSection" - used as the AllowedTypes filter on ParentContainer pickers and the registry id type. */
	static UE_API const FPrimaryAssetType ContributionPrimaryAssetType;
};

#undef UE_API
