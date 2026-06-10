// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsTypedContribution.h"
#include "UObject/PrimaryAssetId.h"

#include "GameSettingsRowContribution.generated.h"

/**
 * Intermediate base for any contribution that nests under another contribution
 * - tabs at the top level, sections inside tabs, rows inside either. Owns the
 * shared ParentContainer field; the picker is filtered to Tab or Section
 * assets only.
 *
 * Tabs themselves derive directly from UGameSettingsTypedContribution because
 * they're top-level containers without a parent.
 */
UCLASS(MinimalAPI, Abstract)
class UGameSettingsRowContribution : public UGameSettingsTypedContribution
{
	GENERATED_BODY()
public:
	/**
	 * Primary asset id of the parent container - either a
	 * UGameSettingsContribution_Tab or a UGameSettingsContribution_Section.
	 * Tabs sit at the top level; sections nest inside tabs (or inside other
	 * sections); rows reference either as their parent. If unset, the
	 * contribution is registered at the top level. If set but the parent
	 * hasn't arrived yet, the registry defers placement until it does.
	 *
	 * WARNING: parent lookups are raw FPrimaryAssetId equality - rename
	 * redirects are not consulted. Renaming a container asset orphans every
	 * row that points at it: those rows sit in the registry's deferred-
	 * placement queue and warn, so a container rename must also update all
	 * referencing rows. (The CoreRedirect for the legacy "ParentTab" name
	 * only covers the property rename, not container asset renames.)
	 */
	UPROPERTY(EditAnywhere, Category = "Display",
		meta = (AllowedTypes = "GameSettingsTab,GameSettingsSection", DisplayPriority = 20))
	FPrimaryAssetId ParentContainer;
};
