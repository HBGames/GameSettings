// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "View/MVVMViewModelContextResolver.h"

#include "GameSettingsViewModelResolver.generated.h"

#define UE_API GAMESETTINGS_API

/**
 * MVVM resolver that returns the per-LocalPlayer screen VM.
 *
 * UI artists pick this resolver in their settings widget BP's MVVM
 * panel (context creation type "Resolver"), select
 * UGameSettingsScreenViewModel as the expected type, and the resolver
 * walks Widget -> OwningLocalPlayer -> UGameSettingsViewModelSubsystem
 * -> screen VM. No per-widget C++ wiring needed.
 */
UCLASS(MinimalAPI, DisplayName = "Game Settings")
class UGameSettingsViewModelResolver : public UMVVMViewModelContextResolver
{
	GENERATED_BODY()
public:
	UE_API virtual UObject* CreateInstance(const UClass* ExpectedType,
		const UUserWidget* UserWidget, const UMVVMView* View) const override;

#if WITH_EDITOR
	UE_API virtual bool DoesSupportViewModelClass(const UClass* Class) const override;
#endif
};

#undef UE_API
