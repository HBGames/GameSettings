// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingEditAction.generated.h"

/**
 * What an edit-condition spec does to its owning setting when its predicate
 * fails. Mirrored onto FGameSettingEditableState via the spec base's shared
 * ApplyActionToState helper.
 */
UENUM(BlueprintType)
enum class EGameSettingEditAction : uint8
{
	/** Hide visually only. DevReason is logged in non-shipping builds. */
	Hide,

	/** Visible but greyed out. DisableReason is shown to the user. */
	Disable,

	/** Hide + non-resetable + hidden-from-analytics. The strictest action. */
	Kill,
};
