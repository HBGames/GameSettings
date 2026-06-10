// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameSettingFilterState.h"
#include "GameplayTagContainer.h"

#define UE_API GAMESETTINGS_API

class ULocalPlayer;

//////////////////////////////////////////////////////////////////////
// FWhenPlatformHasTrait

// Edit condition for game settings that checks CommonUI's platform traits
// to determine whether or not to show a setting
class FWhenPlatformHasTrait : public FGameSettingEditCondition
{
public:
	static UE_API TSharedRef<FWhenPlatformHasTrait> KillIfMissing(FGameplayTag InVisibilityTag, const FString& InKillReason);
	static UE_API TSharedRef<FWhenPlatformHasTrait> DisableIfMissing(FGameplayTag InVisibilityTag, const FText& InDisableReason);

	static UE_API TSharedRef<FWhenPlatformHasTrait> KillIfPresent(FGameplayTag InVisibilityTag, const FString& InKillReason);
	static UE_API TSharedRef<FWhenPlatformHasTrait> DisableIfPresent(FGameplayTag InVisibilityTag, const FText& InDisableReason);

	//~FGameSettingEditCondition interface
	UE_API virtual void GatherEditState(const ULocalPlayer* InLocalPlayer, FGameSettingEditableState& InOutEditState) const override;
	//~End of FGameSettingEditCondition interface

private:
	/** Empty kill reasons get logged and replaced with a generic string (emptiness doubles as the Kill-vs-Disable mode flag). */
	static FString ResolveKillReason(const FString& InKillReason, const TCHAR* FactoryName, FGameplayTag InVisibilityTag);

	FGameplayTag VisibilityTag;
	bool bTagDesired;
	FString KillReason;
	FText DisableReason;
};

#undef UE_API
