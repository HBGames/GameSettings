// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "PlayerMappableKeySettings.h"

#include "GameSettingKeyBindingSettings.generated.h"

/**
 * Player mappable key settings with a sort order for the key bindings screen.
 * Set it as an Input Action's Player Mappable Key Settings and fill in SortOrder
 * next to the display name. Lower sorts first.
 */
UCLASS(MinimalAPI)
class UGameSettingKeyBindingSettings : public UPlayerMappableKeySettings
{
	GENERATED_BODY()
public:
	/** Lower sorts first. 0 sorts by display name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	int32 SortOrder = 0;
};
