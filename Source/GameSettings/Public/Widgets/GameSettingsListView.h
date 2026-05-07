// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Components/ListView.h"

#include "GameSettingsListView.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSettingsViewBindings;

/**
 * List view that renders a UGameSettingsScreenViewModel's VisibleSettings
 * array. Each item is a UGameSettingViewModel; the list picks the entry
 * widget class per VM type via the bindings asset, then assigns the VM
 * to the entry's MVVM view as the "Setting" viewmodel slot.
 *
 * UI artists set Bindings on the BP child and bind the source array
 * (BP-side) to the screen VM's GetVisibleSettings.
 */
UCLASS(MinimalAPI, meta = (Category = "Game Settings"))
class UGameSettingsListView : public UListView
{
	GENERATED_BODY()
public:
	/** The bindings asset used to pick entry widget classes per VM type. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game Settings")
	TObjectPtr<UGameSettingsViewBindings> Bindings;

protected:
	UE_API virtual UUserWidget& OnGenerateEntryWidgetInternal(UObject* Item,
		TSubclassOf<UUserWidget> DesiredEntryClass,
		const TSharedRef<STableViewBase>& OwnerTable) override;
};

#undef UE_API
