// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"

#include "GameSettingsView.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSettingsScreenViewModel;

/**
 * Top-level settings widget shell.
 *
 * Project widget BPs derive from this and install the MVVM extension
 * with a viewmodel slot pointing at UGameSettingsScreenViewModel,
 * resolved through UGameSettingsViewModelResolver. The C++ side does
 * nothing else; layout, bindings, and child widgets are all in BP.
 */
UCLASS(MinimalAPI, Abstract, meta = (DisableNativeTick, Category = "Game Settings"))
class UGameSettingsView : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	/** Convenience accessor for BP. Returns the resolved screen VM, or null if not yet bound. */
	UFUNCTION(BlueprintPure, Category = "Game Settings")
	UE_API UGameSettingsScreenViewModel* GetScreenViewModel() const;
};

#undef UE_API
