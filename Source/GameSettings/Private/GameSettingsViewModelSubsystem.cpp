// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsViewModelSubsystem.h"

#include "Engine/LocalPlayer.h"
#include "GameSettingsSubsystem.h"
#include "ViewModels/GameSettingsScreenViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsViewModelSubsystem)

void UGameSettingsViewModelSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	// Take a dependency so the model-side subsystem is alive whenever we are.
	Collection.InitializeDependency(UGameSettingsSubsystem::StaticClass());
}

void UGameSettingsViewModelSubsystem::Deinitialize()
{
	if (ScreenViewModel)
	{
		ScreenViewModel->Shutdown();
	}
	ScreenViewModel = nullptr;
	Super::Deinitialize();
}

UGameSettingsScreenViewModel* UGameSettingsViewModelSubsystem::GetScreenViewModel()
{
	if (ScreenViewModel)
	{
		return ScreenViewModel;
	}

	ULocalPlayer* LP = GetLocalPlayer();
	UGameSettingsSubsystem* SettingsSubsystem = LP ? LP->GetSubsystem<UGameSettingsSubsystem>() : nullptr;
	if (!SettingsSubsystem)
	{
		return nullptr;
	}

	ScreenViewModel = NewObject<UGameSettingsScreenViewModel>(this);
	ScreenViewModel->Initialize(SettingsSubsystem);
	return ScreenViewModel;
}

UObject* UGameSettingsViewModelSubsystem::FindViewModelByClass(const UClass* ViewModelClass)
{
	if (!ViewModelClass)
	{
		return nullptr;
	}
	UGameSettingsScreenViewModel* VM = GetScreenViewModel();
	if (VM && VM->IsA(ViewModelClass))
	{
		return VM;
	}
	return nullptr;
}
