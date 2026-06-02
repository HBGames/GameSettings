// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "DataSource/GameSettingDataSourceFromLocalPlayerSaveGame.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/SaveGame.h"
#include "GameSettingsLog.h"
#include "GameSettingsSaveGameCache.h"


FGameSettingDataSourceFromLocalPlayerSaveGame::FGameSettingDataSourceFromLocalPlayerSaveGame(
	TSubclassOf<ULocalPlayerSaveGame> InSaveGameClass,
	const FString& InSlotName,
	const TArray<FString>& InPathFromSaveGame)
	: SaveGameClass(InSaveGameClass)
	, SlotName(InSlotName)
{
	FString JoinedPath;
	for (int32 Index = 0; Index < InPathFromSaveGame.Num(); ++Index)
	{
		if (Index > 0)
		{
			JoinedPath += TEXT(".");
		}
		JoinedPath += InPathFromSaveGame[Index];
	}
	PropertyPath = FCachedPropertyPath(JoinedPath);
}

bool FGameSettingDataSourceFromLocalPlayerSaveGame::Resolve(ULocalPlayer* InLocalPlayer)
{
	ULocalPlayerSaveGame* SaveGame = ResolveSaveGame(InLocalPlayer);
	if (!SaveGame)
	{
		return false;
	}
	return PropertyPath.Resolve(SaveGame);
}

FString FGameSettingDataSourceFromLocalPlayerSaveGame::GetValueAsString(ULocalPlayer* InLocalPlayer) const
{
	ULocalPlayerSaveGame* SaveGame = ResolveSaveGame(InLocalPlayer);
	if (!SaveGame)
	{
		return FString();
	}

	FString Out;
	PropertyPathHelpers::GetPropertyValueAsString(SaveGame, PropertyPath.ToString(), Out);
	return Out;
}

void FGameSettingDataSourceFromLocalPlayerSaveGame::SetValue(ULocalPlayer* InLocalPlayer, const FString& Value)
{
	ULocalPlayerSaveGame* SaveGame = ResolveSaveGame(InLocalPlayer);
	if (!SaveGame)
	{
		return;
	}
	PropertyPathHelpers::SetPropertyValueFromString(SaveGame, PropertyPath.ToString(), Value);
}

FString FGameSettingDataSourceFromLocalPlayerSaveGame::ToString() const
{
	return FString::Printf(TEXT("%s[%s].%s"),
		SaveGameClass ? *SaveGameClass->GetName() : TEXT("(null save game class)"),
		*SlotName,
		*PropertyPath.ToString());
}

void FGameSettingDataSourceFromLocalPlayerSaveGame::Persist(ULocalPlayer* InLocalPlayer)
{
	ULocalPlayerSaveGame* SaveGame = ResolveSaveGame(InLocalPlayer);
	if (!SaveGame)
	{
		return;
	}

	// Async save to the player's slot - identical to Lyra's
	// ULyraSettingsShared::SaveSettings (AsyncSaveGameToSlotForLocalPlayer).
	// Async is intentional: a failed settings save is non-fatal.
	SaveGame->AsyncSaveGameToSlotForLocalPlayer();
}

FString FGameSettingDataSourceFromLocalPlayerSaveGame::GetPersistKey() const
{
	// One save per (class, slot). Different slots are different files, so they
	// must each get their own save pass.
	return FString::Printf(TEXT("SaveGame:%s:%s"),
		SaveGameClass ? *SaveGameClass->GetName() : TEXT("(null)"),
		*SlotName);
}

ULocalPlayerSaveGame* FGameSettingDataSourceFromLocalPlayerSaveGame::ResolveSaveGame(ULocalPlayer* InLocalPlayer) const
{
	if (!InLocalPlayer || !SaveGameClass)
	{
		return nullptr;
	}
	return UE::GameSettings::Private::FSaveGameCache::Get().FindOrLoad(InLocalPlayer, SaveGameClass, SlotName);
}
