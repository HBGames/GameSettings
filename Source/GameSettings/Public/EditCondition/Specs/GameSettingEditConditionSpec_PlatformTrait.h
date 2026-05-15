// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "EditCondition/GameSettingEditConditionSpec.h"
#include "GameplayTagContainer.h"

#include "GameSettingEditConditionSpec_PlatformTrait.generated.h"

#define UE_API GAMESETTINGS_API

UENUM(BlueprintType)
enum class EGameSettingPlatformPresence : uint8
{
	/** Predicate fires (setting is gated) when the trait is MISSING from the platform. */
	IfMissing,
	/** Predicate fires (setting is gated) when the trait is PRESENT on the platform. */
	IfPresent,
};

/**
 * Gate the owning setting on a CommonUI platform-trait tag. Wraps the
 * existing FWhenPlatformHasTrait factory so the runtime path is identical to
 * what code-side callers would write.
 *
 * The underlying FWhenPlatformHasTrait supports Disable and Kill semantics.
 * Hide is not meaningful for a platform gate (use Kill if the entry should
 * not appear at all on the wrong platform).
 */
UCLASS(MinimalAPI, DisplayName = "Platform Trait")
class UGameSettingEditConditionSpec_PlatformTrait : public UGameSettingEditConditionSpec
{
	GENERATED_BODY()

public:
	/** Platform trait tag (typically Platform.Trait.*). */
	UPROPERTY(EditAnywhere, Category = "Platform", meta = (Categories = "Platform.Trait"))
	FGameplayTag VisibilityTag;

	/** Should the predicate fire when the trait is missing or when it is present? */
	UPROPERTY(EditAnywhere, Category = "Platform")
	EGameSettingPlatformPresence When = EGameSettingPlatformPresence::IfMissing;

	UE_API virtual TSharedPtr<FGameSettingEditCondition> BuildCondition(UGameSettingRegistry& Registry, UGameSetting& Owner) const override;
	UE_API virtual FString DebugDescribe() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

#undef UE_API
