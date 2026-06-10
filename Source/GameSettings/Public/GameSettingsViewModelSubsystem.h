// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"

#include "GameSettingsViewModelSubsystem.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSettingsScreenViewModel;

/**
 * Per-LocalPlayer owner of the screen view model.
 *
 * Settings widgets resolve their MVVM context through
 * UGameSettingsViewModelResolver, which walks
 * Widget -> OwningLocalPlayer -> this subsystem -> screen VM.
 *
 * The screen VM is built lazily on first access and cached for the
 * LocalPlayer's lifetime, so multiple settings widgets share the same
 * VM (and therefore the same dirty/filter/focus state).
 */
UCLASS(MinimalAPI)
class UGameSettingsViewModelSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	UE_API virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	UE_API virtual void Deinitialize() override;

	/** The screen VM, built and initialized on first call. Always non-null after Initialize. */
	UFUNCTION(BlueprintPure, Category = "Game Settings")
	UE_API UGameSettingsScreenViewModel* GetScreenViewModel();

private:
	friend class UGameSettingsViewModelResolver;

	UObject* FindViewModelByClass(const UClass* ViewModelClass);

	UPROPERTY(Transient)
	TObjectPtr<UGameSettingsScreenViewModel> ScreenViewModel;
};

#undef UE_API
