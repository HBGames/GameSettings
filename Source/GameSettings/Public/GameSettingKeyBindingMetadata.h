// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "GameSettingKeyBindingMetadata.generated.h"

/**
 * Optional row ordering for the key bindings screen. Assign to an Input Action's
 * Player Mappable Key Settings Metadata. Lower SortOrder sorts first. Unset rows
 * sort by display name. Negatives pin above the sorted block, positives below.
 */
UCLASS(MinimalAPI, BlueprintType)
class UGameSettingKeyBindingMetadata : public UDataAsset
{
	GENERATED_BODY()
public:
	/** Lower sorts first. 0 sorts by display name. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Key Binding")
	int32 SortOrder = 0;
};
