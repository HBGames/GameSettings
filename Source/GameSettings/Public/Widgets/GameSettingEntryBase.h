// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"

#include "GameSettingEntryBase.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * Base class for settings entry widgets shown in UGameSettingsListView.
 *
 * UI artists derive Blueprint widgets from this class, one per VM type
 * they want to render (toggle, scalar slider, discrete picker, action
 * button, navigation button, ...). Each BP installs the MVVM extension
 * with a viewmodel slot named "Setting" expecting the appropriate
 * UGameSettingViewModel subclass.
 *
 * The list view picks the right BP class per VM type via the
 * UGameSettingsViewBindings asset.
 *
 * Empty in C++; the base exists purely as a typed root for the binding
 * asset's TSubclassOf field.
 */
UCLASS(MinimalAPI, Abstract, meta = (DisableNativeTick))
class UGameSettingEntryBase : public UCommonUserWidget
{
	GENERATED_BODY()
};

#undef UE_API
