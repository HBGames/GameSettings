// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "EditCondition/GameSettingEditConditionSpec.h"
#include "UObject/PrimaryAssetId.h"

#include "GameSettingEditConditionSpec_DependsOnDiscrete.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Gate the owning setting on the option-string value of another Discrete
 * contribution. The match is on the option's underlying Value (not its
 * DisplayText), so localization changes are safe.
 *
 * Predicate fires when:
 *   - bInvertMatch == false and the target's value is NOT in RequiredValues
 *   - bInvertMatch == true and the target's value IS in RequiredValues
 */
UCLASS(MinimalAPI, DisplayName = "Depends On Discrete")
class UGameSettingEditConditionSpec_DependsOnDiscrete : public UGameSettingEditConditionSpec
{
	GENERATED_BODY()

public:
	/** Target Discrete contribution whose value is read each refresh. */
	UPROPERTY(EditAnywhere, Category = "Dependency", meta = (AllowedTypes = "GameSettingsDiscrete"))
	FPrimaryAssetId TargetSetting;

	/** Accepted option Values (any-of). Matched against UGameSettingValueDiscreteDynamic::GetValueAsString. */
	UPROPERTY(EditAnywhere, Category = "Dependency")
	TArray<FString> RequiredValues;

	/** Invert the predicate: when true, the setting is gated when the target IS in RequiredValues. */
	UPROPERTY(EditAnywhere, Category = "Dependency")
	bool bInvertMatch = false;

	UE_API virtual void GetSettingDependencies(TArray<FPrimaryAssetId>& OutIds) const override;
	UE_API virtual TSharedPtr<FGameSettingEditCondition> BuildCondition(UGameSettingRegistry& Registry, UGameSetting& Owner) const override;
	UE_API virtual FString DebugDescribe() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

#undef UE_API
