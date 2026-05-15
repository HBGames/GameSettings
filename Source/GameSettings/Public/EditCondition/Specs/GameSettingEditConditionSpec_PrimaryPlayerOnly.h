// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "EditCondition/GameSettingEditConditionSpec.h"

#include "GameSettingEditConditionSpec_PrimaryPlayerOnly.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Disable the owning setting for non-primary players. Wraps the singleton
 * FWhenPlayingAsPrimaryPlayer condition. The underlying implementation hard-
 * codes Disable semantics; Hide and Kill are rejected at validation time.
 */
UCLASS(MinimalAPI, DisplayName = "Primary Player Only")
class UGameSettingEditConditionSpec_PrimaryPlayerOnly : public UGameSettingEditConditionSpec
{
	GENERATED_BODY()

public:
	UE_API virtual TSharedPtr<FGameSettingEditCondition> BuildCondition(UGameSettingRegistry& Registry, UGameSetting& Owner) const override;
	UE_API virtual FString DebugDescribe() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

#undef UE_API
