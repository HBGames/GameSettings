// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsViewModelResolver.h"

#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "GameSettingsViewModelSubsystem.h"
#include "ViewModels/GameSettingsScreenViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsViewModelResolver)

UObject* UGameSettingsViewModelResolver::CreateInstance(const UClass* ExpectedType,
	const UUserWidget* UserWidget, const UMVVMView* /*View*/) const
{
	if (!UserWidget)
	{
		return nullptr;
	}

	const ULocalPlayer* LocalPlayer = UserWidget->GetOwningLocalPlayer();
	if (!LocalPlayer)
	{
		return nullptr;
	}

	UGameSettingsViewModelSubsystem* Subsystem = LocalPlayer->GetSubsystem<UGameSettingsViewModelSubsystem>();
	return Subsystem ? Subsystem->FindViewModelByClass(ExpectedType) : nullptr;
}

#if WITH_EDITOR
bool UGameSettingsViewModelResolver::DoesSupportViewModelClass(const UClass* Class) const
{
	return Class && Class->IsChildOf(UGameSettingsScreenViewModel::StaticClass());
}
#endif
