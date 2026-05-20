// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingDataSource.h"
#include "PropertyPathHelpers.h"
#include "Templates/SubclassOf.h"

#define UE_API GAMESETTINGS_API

class ULocalPlayer;
class ULocalPlayerSaveGame;

/**
 * Data source that resolves a property/function chain rooted at a
 * per-LocalPlayer ULocalPlayerSaveGame instance. The instance is loaded
 * lazily on first resolve via ULocalPlayerSaveGame::LoadOrCreateSaveGameForLocalPlayer
 * and cached in a process-wide map keyed by (LocalPlayer, SaveGameClass, SlotName)
 * so subsequent setters write back to the same instance.
 *
 * The cache is GC-rooted via FGCObject so live instances survive across
 * registry rebuilds.
 */
class FGameSettingDataSourceFromLocalPlayerSaveGame : public FGameSettingDataSource
{
public:
	UE_API FGameSettingDataSourceFromLocalPlayerSaveGame(
		TSubclassOf<ULocalPlayerSaveGame> InSaveGameClass,
		const FString& InSlotName,
		const TArray<FString>& InPathFromSaveGame);

	UE_API virtual bool Resolve(ULocalPlayer* InLocalPlayer) override;
	UE_API virtual FString GetValueAsString(ULocalPlayer* InLocalPlayer) const override;
	UE_API virtual void SetValue(ULocalPlayer* InLocalPlayer, const FString& Value) override;
	UE_API virtual FString ToString() const override;
	UE_API virtual void Persist(ULocalPlayer* InLocalPlayer) override;
	UE_API virtual FString GetPersistKey() const override;

private:
	UE_API ULocalPlayerSaveGame* ResolveSaveGame(ULocalPlayer* InLocalPlayer) const;

	TSubclassOf<ULocalPlayerSaveGame> SaveGameClass;
	FString SlotName;
	FCachedPropertyPath PropertyPath;
};

#undef UE_API
