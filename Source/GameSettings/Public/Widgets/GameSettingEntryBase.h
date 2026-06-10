// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "CommonUserWidget.h"

#include "GameSettingEntryBase.generated.h"

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
 * UGameSettingsViewBindings asset and wires the "Setting" slot via
 * UMVVMView::SetViewModel; the IUserObjectListEntry inheritance is
 * there so BP subclasses pass UListView::EntryWidgetClass's MustImplement
 * filter (it requires IUserListEntry, satisfied by IUserObjectListEntry).
 * BPs may optionally override BP_OnListItemObjectSet if they need a
 * graph-side hook in addition to the MVVM bindings.
 */
UCLASS(MinimalAPI, Abstract, meta = (DisableNativeTick))
class UGameSettingEntryBase : public UCommonUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()
};
