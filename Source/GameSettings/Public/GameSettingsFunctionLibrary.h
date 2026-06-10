// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "GameSettingsFunctionLibrary.generated.h"

#define UE_API GAMESETTINGS_API

class ULocalPlayer;
class ULocalPlayerSaveGame;

/** Blueprint-facing helpers for the GameSettings plugin. */
UCLASS(MinimalAPI)
class UGameSettingsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Find (or load on first request) the per-player save game the settings
	 * system uses for SaveGame-backed bindings. Returns the same cached
	 * instance for a given player + class + slot, so values written here are
	 * the values the settings screen reads.
	 */
	UFUNCTION(BlueprintCallable, Category = "Game Settings")
	static UE_API ULocalPlayerSaveGame* FindOrLoadPersistentSaveGame(const ULocalPlayer* LocalPlayer, TSubclassOf<ULocalPlayerSaveGame> SaveGameClass, const FString& SlotName);
};

#undef UE_API
