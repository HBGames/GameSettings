// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's UKeyAlreadyBoundWarning (Copyright Epic Games, Inc.).

#pragma once

#include "Widgets/Misc/GameSettingPressAnyKey.h"

#include "KeyAlreadyBoundWarning.generated.h"

#define UE_API GAMESETTINGS_API

class UTextBlock;

/**
 * Press-any-key modal variant shown when the chosen key is already bound to
 * another action. Adds warning and cancel text blocks. Pressing the same key
 * again confirms the rebind, any other input cancels.
 */
UCLASS(MinimalAPI, Abstract)
class UKeyAlreadyBoundWarning : public UGameSettingPressAnyKey
{
	GENERATED_BODY()

public:
	UE_API void SetWarningText(const FText& InText);

	UE_API void SetCancelText(const FText& InText);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> WarningText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, BlueprintProtected = true, AllowPrivateAccess = true))
	TObjectPtr<UTextBlock> CancelText;
};

#undef UE_API
