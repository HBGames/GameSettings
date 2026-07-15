// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Engine/LocalPlayer.h"
#include "GameFramework/SaveGame.h"
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

			// Evict entries for departed LocalPlayers (and values the GC
			// nulled) so old PIE sessions / split-screen leavers don't stay
			// rooted for process lifetime. Cheap: the map holds a handful of
			// entries per live player.
			for (auto It = Entries.CreateIterator(); It; ++It)
			{
				if (!It.Key().LocalPlayer.IsValid() || !It.Value())
				{
					It.RemoveCurrent();
				}
			}

			const FSaveGameCacheKey Key{LocalPlayer, SaveGameClass.Get(), SlotName};
			if (ULocalPlayerSaveGame* const* Existing = ObjectPtrDecay(Entries).Find(Key))
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
			// By reference: the collector may null or remap the pointer, and
			// a by-value pair would discard that fixup, leaving the map with
			// a dangling entry that FindOrLoad then hands back out.
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