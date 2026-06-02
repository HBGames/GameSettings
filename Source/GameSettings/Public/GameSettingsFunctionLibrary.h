// Copyright Mob Entertainment. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameSettingsFunctionLibrary.generated.h"

class ULocalPlayer;
class ULocalPlayerSaveGame;

/**
 * 
 */
UCLASS()
class GAMESETTINGS_API UGameSettingsFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category="Game Settings")
	static ULocalPlayerSaveGame* FindOrLoadPersistentSaveGame(const ULocalPlayer* LocalPlayer, TSubclassOf<ULocalPlayerSaveGame> SaveGameClass, const FString& SlotName);

};
