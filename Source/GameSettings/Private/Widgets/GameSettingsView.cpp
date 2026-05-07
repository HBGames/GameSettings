// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Widgets/GameSettingsView.h"

#include "Engine/LocalPlayer.h"
#include "GameSettingsViewModelSubsystem.h"
#include "ViewModels/GameSettingsScreenViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsView)

UGameSettingsScreenViewModel* UGameSettingsView::GetScreenViewModel() const
{
	if (ULocalPlayer* LP = GetOwningLocalPlayer())
	{
		if (UGameSettingsViewModelSubsystem* Sub = LP->GetSubsystem<UGameSettingsViewModelSubsystem>())
		{
			return Sub->GetScreenViewModel();
		}
	}
	return nullptr;
}
