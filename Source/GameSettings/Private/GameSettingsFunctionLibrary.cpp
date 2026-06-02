// Copyright Mob Entertainment. All rights reserved.


#include "GameSettingsFunctionLibrary.h"

#include "GameSettingsSaveGameCache.h"

ULocalPlayerSaveGame* UGameSettingsFunctionLibrary::FindOrLoadPersistentSaveGame(const ULocalPlayer* LocalPlayer, TSubclassOf<ULocalPlayerSaveGame> SaveGameClass, const FString& SlotName)
{
	return UE::GameSettings::Private::FSaveGameCache::Get().FindOrLoad(LocalPlayer, SaveGameClass, SlotName);
}
