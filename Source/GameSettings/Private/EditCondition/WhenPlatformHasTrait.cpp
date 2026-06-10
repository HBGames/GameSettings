// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's GameSettings plugin (Copyright Epic Games, Inc.).

#include "EditCondition/WhenPlatformHasTrait.h"

#include "CommonUIVisibilitySubsystem.h"
#include "GameSettingsLog.h"

#define LOCTEXT_NAMESPACE "GameSetting"

TSharedRef<FWhenPlatformHasTrait> FWhenPlatformHasTrait::KillIfMissing(FGameplayTag InVisibilityTag, const FString& InKillReason)
{
	ensureMsgf(InVisibilityTag.IsValid(),
		TEXT("FWhenPlatformHasTrait::KillIfMissing: invalid VisibilityTag - this condition will kill its setting on every platform."));

	TSharedRef<FWhenPlatformHasTrait> Result = MakeShared<FWhenPlatformHasTrait>();
	Result->VisibilityTag = InVisibilityTag;
	Result->KillReason = ResolveKillReason(InKillReason, TEXT("KillIfMissing"), InVisibilityTag);
	Result->bTagDesired = true;

	return Result;
}

TSharedRef<FWhenPlatformHasTrait> FWhenPlatformHasTrait::DisableIfMissing(FGameplayTag InVisibilityTag, const FText& InDisableReason)
{
	ensureMsgf(InVisibilityTag.IsValid(),
		TEXT("FWhenPlatformHasTrait::DisableIfMissing: invalid VisibilityTag - this condition will disable its setting on every platform."));
	if (InDisableReason.IsEmpty())
	{
		UE_LOG(LogGameSettings, Warning,
			TEXT("FWhenPlatformHasTrait::DisableIfMissing(%s): empty DisableReason - the user will see no explanation for the disabled setting."),
			*InVisibilityTag.ToString());
	}

	TSharedRef<FWhenPlatformHasTrait> Result = MakeShared<FWhenPlatformHasTrait>();
	Result->VisibilityTag = InVisibilityTag;
	Result->DisableReason = InDisableReason;
	Result->bTagDesired = true;

	return Result;
}

TSharedRef<FWhenPlatformHasTrait> FWhenPlatformHasTrait::KillIfPresent(FGameplayTag InVisibilityTag, const FString& InKillReason)
{
	ensureMsgf(InVisibilityTag.IsValid(),
		TEXT("FWhenPlatformHasTrait::KillIfPresent: invalid VisibilityTag - this condition will never fire."));

	TSharedRef<FWhenPlatformHasTrait> Result = MakeShared<FWhenPlatformHasTrait>();
	Result->VisibilityTag = InVisibilityTag;
	Result->KillReason = ResolveKillReason(InKillReason, TEXT("KillIfPresent"), InVisibilityTag);
	Result->bTagDesired = false;

	return Result;
}

TSharedRef<FWhenPlatformHasTrait> FWhenPlatformHasTrait::DisableIfPresent(FGameplayTag InVisibilityTag, const FText& InDisableReason)
{
	ensureMsgf(InVisibilityTag.IsValid(),
		TEXT("FWhenPlatformHasTrait::DisableIfPresent: invalid VisibilityTag - this condition will never fire."));
	if (InDisableReason.IsEmpty())
	{
		UE_LOG(LogGameSettings, Warning,
			TEXT("FWhenPlatformHasTrait::DisableIfPresent(%s): empty DisableReason - the user will see no explanation for the disabled setting."),
			*InVisibilityTag.ToString());
	}

	TSharedRef<FWhenPlatformHasTrait> Result = MakeShared<FWhenPlatformHasTrait>();
	Result->VisibilityTag = InVisibilityTag;
	Result->DisableReason = InDisableReason;
	Result->bTagDesired = false;

	return Result;
}

FString FWhenPlatformHasTrait::ResolveKillReason(const FString& InKillReason, const TCHAR* FactoryName, FGameplayTag InVisibilityTag)
{
	if (!InKillReason.IsEmpty())
	{
		return InKillReason;
	}

	// An empty KillReason can't be passed through: GatherEditState uses
	// KillReason emptiness as its Kill-vs-Disable mode flag, so an empty
	// string would silently demote this Kill condition to a Disable.
	UE_LOG(LogGameSettings, Warning,
		TEXT("FWhenPlatformHasTrait::%s(%s): empty KillReason - substituting a generic one."),
		FactoryName, *InVisibilityTag.ToString());
	return FString::Printf(TEXT("Killed by platform-trait condition on %s (no reason given)"), *InVisibilityTag.ToString());
}

void FWhenPlatformHasTrait::GatherEditState(const ULocalPlayer* InLocalPlayer, FGameSettingEditableState& InOutEditState) const
{
	if (UCommonUIVisibilitySubsystem::GetChecked(InLocalPlayer)->HasVisibilityTag(VisibilityTag) != bTagDesired)
	{
		if (KillReason.IsEmpty())
		{
			InOutEditState.Disable(DisableReason);
		}
		else
		{
			InOutEditState.Kill(KillReason);
		}
	}
}

#undef LOCTEXT_NAMESPACE
