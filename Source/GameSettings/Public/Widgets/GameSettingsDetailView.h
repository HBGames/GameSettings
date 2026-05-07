// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"

#include "GameSettingsDetailView.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Detail panel for the focused setting.
 *
 * The BP child installs the MVVM extension with a viewmodel slot
 * (typically named "Setting") and binds it to the screen VM's
 * GetFocusedSetting via PropertyPath. Description, warning, and so on
 * bind to the focused VM's FieldNotify properties.
 *
 * Detail extensions (per-VM-type sub-widgets) are configured on the
 * UGameSettingsViewBindings asset; the BP author iterates them through
 * a UPanelWidget and the engine's panel-widget MVVM extension.
 */
UCLASS(MinimalAPI, Abstract, meta = (DisableNativeTick, Category = "Game Settings"))
class UGameSettingsDetailView : public UCommonUserWidget
{
	GENERATED_BODY()
};

#undef UE_API
