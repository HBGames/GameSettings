// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsFunctionLibrary.h"

#include "GameSettingsSaveGameCache.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsFunctionLibrary)

ULocalPlayerSaveGame* UGameSettingsFunctionLibrary::FindOrLoadPersistentSaveGame(const ULocalPlayer* LocalPlayer, TSubclassOf<ULocalPlayerSaveGame> SaveGameClass, const FString& SlotName)
{
	return UE::GameSettings::Private::FSaveGameCache::Get().FindOrLoad(LocalPlayer, SaveGameClass, SlotName);
}
