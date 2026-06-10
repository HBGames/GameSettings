// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "Engine/DataAsset.h"
#include "Templates/SubclassOf.h"

#include "GameSettingsViewBindings.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSettingViewModel;

/**
 * One entry's worth of detail-view extension widgets. Wrapped in a struct
 * because UHT doesn't reflect TMap values that are themselves TArrays.
 */
USTRUCT(BlueprintType)
struct FGameSettingsDetailExtensionList
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "Bindings")
	TArray<TSoftClassPtr<UCommonUserWidget>> Extensions;
};

/**
 * Maps view-model class to entry widget class for the settings list,
 * plus per-VM-type detail extensions for the detail view.
 *
 * Lookup walks the VM class chain on a miss, so a binding for a base
 * class acts as a catch-all when no specific match is found. Projects
 * author one of these as a Data Asset and assign it to the list view.
 * GFP actions can push additional binding sets onto a runtime override
 * stack via UGameFeatureAction_AddViewBindings.
 */
UCLASS(MinimalAPI, BlueprintType)
class UGameSettingsViewBindings : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Entry widget class instantiated by the list view for each VM type. */
	UPROPERTY(EditDefaultsOnly, Category = "Bindings")
	TMap<TSubclassOf<UGameSettingViewModel>, TSoftClassPtr<UCommonUserWidget>> EntryWidgetForViewModel;

	/** Detail-view extension widgets shown alongside the focused setting. */
	UPROPERTY(EditDefaultsOnly, Category = "Bindings")
	TMap<TSubclassOf<UGameSettingViewModel>, FGameSettingsDetailExtensionList> DetailExtensions;

	/** Walk the VM class chain to find the entry widget. Returns null if nothing matches. */
	UE_API TSubclassOf<UCommonUserWidget> FindEntryWidget(TSubclassOf<UGameSettingViewModel> ViewModelClass) const;

	/** Walk the VM class chain to gather detail extensions. Returns class chain in derived-first order. */
	UFUNCTION(BlueprintCallable, Category = "Bindings")
	UE_API TArray<TSoftClassPtr<UCommonUserWidget>> GatherDetailExtensions(TSubclassOf<UGameSettingViewModel> ViewModelClass) const;

#if WITH_EDITOR
	UE_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	/**
	 * Resolve every authored entry-widget soft class once, on first consult.
	 * FindEntryWidget runs per generated row during list scroll, where a
	 * LoadSynchronous is a hitch risk; pre-resolving pays that cost a single
	 * time, and the hard refs in the resolved map keep the loaded classes
	 * from being GC'd and re-loaded mid-scroll.
	 */
	void ResolveEntryWidgets() const;

	/** Resolved entry classes by VM class. Mutable: built lazily from const lookups. */
	UPROPERTY(Transient)
	mutable TMap<TSubclassOf<UGameSettingViewModel>, TSubclassOf<UCommonUserWidget>> ResolvedEntryWidgets;

	/** True once ResolvedEntryWidgets has been built. */
	mutable bool bEntryWidgetsResolved = false;
};

#undef UE_API
