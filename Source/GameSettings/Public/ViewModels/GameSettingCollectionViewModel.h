// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "ViewModels/GameSettingViewModel.h"

#include "GameSettingCollectionViewModel.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * View model for a tab (UGameSettingCollection). Holds the child
 * UGameSettingViewModels as a FieldNotify array so a hierarchical layout
 * can bind to it. Most settings screens use the screen VM's flat
 * VisibleSettings array instead; this is here for projects that want
 * inline subgroup rendering.
 */
UCLASS(MinimalAPI, BlueprintType, DisplayName = "Game Setting Collection")
class UGameSettingCollectionViewModel : public UGameSettingViewModel
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, FieldNotify, Category = "Hierarchy")
	const TArray<UGameSettingViewModel*>& GetChildViewModels() const { return ChildViewModels; }

	/** Replace the entire child list. Broadcasts FieldNotify. */
	UE_API void SetChildViewModels(TArray<TObjectPtr<UGameSettingViewModel>> InChildren);

private:
	UPROPERTY()
	TArray<TObjectPtr<UGameSettingViewModel>> ChildViewModels;
};

#undef UE_API
