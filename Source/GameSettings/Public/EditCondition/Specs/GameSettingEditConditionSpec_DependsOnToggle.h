// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "EditCondition/GameSettingEditConditionSpec.h"
#include "UObject/PrimaryAssetId.h"

#include "GameSettingEditConditionSpec_DependsOnToggle.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Gate the owning setting on the value of another Toggle contribution.
 * Predicate fires when the target's bool value does not equal bRequiredValue.
 */
UCLASS(MinimalAPI, DisplayName = "Depends On Toggle")
class UGameSettingEditConditionSpec_DependsOnToggle : public UGameSettingEditConditionSpec
{
	GENERATED_BODY()

public:
	/** Target Toggle contribution whose value is read each refresh. */
	UPROPERTY(EditAnywhere, Category = "Dependency", meta = (AllowedTypes = "GameSettingsToggle"))
	FPrimaryAssetId TargetSetting;

	/** Required value of the target for the owning setting to remain enabled. */
	UPROPERTY(EditAnywhere, Category = "Dependency")
	bool bRequiredValue = true;

	UE_API virtual void GetSettingDependencies(TArray<FPrimaryAssetId>& OutIds) const override;
	UE_API virtual TSharedPtr<FGameSettingEditCondition> BuildCondition(UGameSettingRegistry& Registry, UGameSetting& Owner) const override;
	UE_API virtual FString DebugDescribe() const override;

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};

#undef UE_API
