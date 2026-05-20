// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "DataSource/GameSettingDataSourceFromLocalPlayerSaveGame.h"

#include "Engine/LocalPlayer.h"
#include "GameFramework/SaveGame.h"
#include "GameSettingsLog.h"
#include "UObject/GCObject.h"

namespace UE::GameSettings::Private
{
	struct FSaveGameCacheKey
	{
		TWeakObjectPtr<const ULocalPlayer> LocalPlayer;
		TWeakObjectPtr<const UClass> SaveGameClass;
		FString SlotName;

		bool operator==(const FSaveGameCacheKey& Other) const
		{
			return LocalPlayer == Other.LocalPlayer
				&& SaveGameClass == Other.SaveGameClass
				&& SlotName.Equals(Other.SlotName, ESearchCase::CaseSensitive);
		}

		friend uint32 GetTypeHash(const FSaveGameCacheKey& Key)
		{
			uint32 Hash = GetTypeHash(Key.LocalPlayer);
			Hash = HashCombine(Hash, GetTypeHash(Key.SaveGameClass));
			Hash = HashCombine(Hash, GetTypeHash(Key.SlotName));
			return Hash;
		}
	};

	class FSaveGameCache : public FGCObject
	{
	public:
		static FSaveGameCache& Get()
		{
			static FSaveGameCache Instance;
			return Instance;
		}

		ULocalPlayerSaveGame* FindOrLoad(const ULocalPlayer* LocalPlayer, TSubclassOf<ULocalPlayerSaveGame> SaveGameClass, const FString& SlotName)
		{
			if (!LocalPlayer || !SaveGameClass)
			{
				return nullptr;
			}

			const FSaveGameCacheKey Key{ LocalPlayer, SaveGameClass.Get(), SlotName };
			if (TObjectPtr<ULocalPlayerSaveGame>* Existing = Entries.Find(Key))
			{
				if (*Existing)
				{
					return *Existing;
				}
				Entries.Remove(Key);
			}

			ULocalPlayerSaveGame* Loaded = ULocalPlayerSaveGame::LoadOrCreateSaveGameForLocalPlayer(SaveGameClass, LocalPlayer, SlotName);
			if (Loaded)
			{
				Entries.Add(Key, Loaded);
			}
			return Loaded;
		}

		//~FGCObject
		virtual void AddReferencedObjects(FReferenceCollector& Collector) override
		{
			for (TPair<FSaveGameCacheKey, TObjectPtr<ULocalPlayerSaveGame>>& Pair : Entries)
			{
				Collector.AddReferencedObject(Pair.Value);
			}
		}
		virtual FString GetReferencerName() const override
		{
			return TEXT("UE::GameSettings::Private::FSaveGameCache");
		}
		//~End FGCObject

	private:
		FSaveGameCache() = default;

		TMap<FSaveGameCacheKey, TObjectPtr<ULocalPlayerSaveGame>> Entries;
	};
}

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
