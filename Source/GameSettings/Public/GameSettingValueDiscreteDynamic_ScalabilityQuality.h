// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingValueDiscreteDynamic.h"

#include "GameSettingValueDiscreteDynamic_ScalabilityQuality.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Overall scalability quality picker (Low/Medium/High/Epic + optional
 * Custom). Ports Lyra's ULyraSettingValueDiscrete_OverallQuality logic
 * with no project-specific dependencies: pulls the max level from the
 * engine's UGameUserSettings and routes Set/Get through
 * SetOverallScalabilityLevel / GetOverallScalabilityLevel.
 *
 * "Custom" appears in the option list and as the selected entry whenever
 * GetOverallScalabilityLevel returns INDEX_NONE (i.e. the per-feature
 * scalability values disagree, so no preset matches). Picking Custom from
 * the menu leaves the per-feature settings alone.
 *
 * Use by setting this as the SettingClass on a UGameSettingsContribution_Discrete.
 * The contribution's Options and Binding fields are ignored; this class
 * self-manages both.
 */
UCLASS(MinimalAPI)
class UGameSettingValueDiscreteDynamic_ScalabilityQuality : public UGameSettingValueDiscreteDynamic
{
	GENERATED_BODY()

public:
	UE_API UGameSettingValueDiscreteDynamic_ScalabilityQuality();

	//~UGameSettingValue
	UE_API virtual void StoreInitial() override;
	UE_API virtual void ResetToDefault() override;
	UE_API virtual void RestoreToInitial() override;
	//~End UGameSettingValue

	//~UGameSettingValueDiscrete
	UE_API virtual void SetDiscreteOptionByIndex(int32 Index) override;
	UE_API virtual int32 GetDiscreteOptionIndex() const override;
	UE_API virtual TArray<FText> GetDiscreteOptions() const override;
	//~End UGameSettingValueDiscrete

	/**
	 * Override to cap the highest quality level the player can pick. Default
	 * implementation returns -1 (no cap; all four presets available). Project
	 * subclasses can return a platform-specific value.
	 */
	UE_API virtual int32 GetMaxSupportedQualityLevel() const;

protected:
	//~UGameSettingValue
	UE_API virtual void OnInitialized() override;
	//~End UGameSettingValue

	int32 GetCustomOptionIndex() const;
	int32 GetCurrentScalabilityLevel() const;

	/** Pure preset options after the max-level filter. */
	TArray<FText> Options;

	/** Same plus a trailing "Custom" entry; surfaced when the live state matches no preset. */
	TArray<FText> OptionsWithCustom;
};

#undef UE_API
