// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "GameSettingsBindingValueType.generated.h"

#define UE_API GAMESETTINGS_API

class FProperty;

/**
 * Value shape a typed contribution expects on the getter/setter side of
 * its FGameSettingsBinding. Drives both the editor's function-dropdown
 * filter and the leaf contributions' save-time validation.
 */
UENUM(BlueprintType)
enum class EGameSettingsBindingValueType : uint8
{
	/** No type filter; matches every property type. */
	Unknown,

	/** UGameSettingsContribution_Toggle: bool. */
	Boolean,

	/** UGameSettingsContribution_Scalar: float, double, or any integer width. */
	Numeric,

	/** UGameSettingsContribution_Discrete: anything that round-trips through PropertyPathHelpers' string conversion. */
	Discrete,
};

namespace UE::GameSettings
{
	/**
	 * True when Prop's underlying type fits Expected. Unknown matches everything.
	 * Null Prop returns true only for Unknown.
	 */
	UE_API bool IsPropertyOfValueType(const FProperty* Prop, EGameSettingsBindingValueType Expected);
}

#undef UE_API
