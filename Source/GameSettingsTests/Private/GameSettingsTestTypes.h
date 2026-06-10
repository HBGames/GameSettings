// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingCollection.h"
#include "GameSettingRegistry.h"
#include "GameSettingValueBool.h"
#include "GameSettingsContribution.h"

#include "GameSettingsTestTypes.generated.h"

/**
 * Bool setting whose Apply() mutates another watched setting. Used by the
 * change-tracker tests to prove FGameSettingRegistryChangeTracker::ApplyChanges
 * survives a setting that dirties a sibling mid-transaction (the
 * bApplyingSettings re-entrancy guard).
 *
 * Lives in the GameSettingsTests module which is `UncookedOnly`, so this class
 * is never present in cooked client/server packages. No #if guard — UHT
 * rejects UCLASS inside preprocessor blocks (except WITH_EDITORONLY_DATA).
 */
UCLASS()
class UGameSettingsTest_CrossApplyBool : public UGameSettingValueBool
{
	GENERATED_BODY()

public:
	/** Watched sibling that Apply() flips. */
	UPROPERTY(Transient)
	TObjectPtr<UGameSettingValueBool> SettingToMutateOnApply;

protected:
	virtual void OnApply() override
	{
		Super::OnApply();
		if (SettingToMutateOnApply)
		{
			SettingToMutateOnApply->SetBoolValue(!SettingToMutateOnApply->GetBoolValue());
		}
	}
};

/**
 * Contribution that registers a fixed number of top-level collections.
 * Collections need no data sources, so the subsystem's WireSettingTree
 * Initialize pass runs cleanly without a real game LocalPlayer. Each Apply
 * creates fresh objects with batch-unique ids (a removed batch's objects are
 * MarkAsGarbage'd and can't be re-added).
 */
UCLASS()
class UGameSettingsTest_CollectionContribution : public UGameSettingsContribution
{
	GENERATED_BODY()

public:
	/** Collections registered per Apply call. */
	int32 NumCollections = 2;

	/** Id prefix so concurrent tests don't collide on SettingId. */
	FString IdPrefix = TEXT("TestContribution");

	/** How many times Apply has run; the dedup tests assert on this. */
	int32 ApplyCount = 0;

	virtual void Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles) override
	{
		++ApplyCount;
		for (int32 Index = 0; Index < NumCollections; ++Index)
		{
			UGameSettingCollection* Collection = NewObject<UGameSettingCollection>(&Registry);
			const FString IdName = FString::Printf(TEXT("%s_B%d_C%d"), *IdPrefix, ApplyCount, Index);
			Collection->SetSettingId(FPrimaryAssetId(FPrimaryAssetType(TEXT("GameSettingsTest")), FName(*IdName)));
			Collection->SetDisplayName(FText::FromString(IdName));
			OutHandles.Add(Registry.AddCollection(Collection));
		}
	}
};
