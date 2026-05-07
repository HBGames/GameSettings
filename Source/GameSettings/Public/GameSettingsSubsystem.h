// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingHandle.h"
#include "Subsystems/LocalPlayerSubsystem.h"

#include "GameSettingsSubsystem.generated.h"

#define UE_API GAMESETTINGS_API

class UGameSettingRegistry;
class UGameSettingsAutoContributor;
class UGameSettingsContribution;

/**
 * Per-LocalPlayer owner of the UGameSettingRegistry.
 *
 * The registry exists for the lifetime of the LocalPlayer, not just while
 * a settings screen is open. Game code can query and modify settings any
 * time. Multiple screens (main menu, in-game pause overlay) share the
 * same registry.
 *
 * Four provisioning paths, in any order or combination:
 *   1. Auto-build. The first call to GetOrCreateRegistry() instantiates
 *      the class configured in UGameSettingsDeveloperSettings (defaults
 *      to plain UGameSettingRegistry).
 *   2. SetRegistry(). Hand the subsystem a registry built externally
 *      (e.g. by a project widget that constructs its own subclass).
 *   3. Auto-contributors. Every UGameSettingsAutoContributor CDO gets
 *      applied to the registry once it exists.
 *   4. UGameFeatureAction_RegisterGameSettings. GFPs push contributions
 *      via ApplyContribution() on activation.
 */
UCLASS(MinimalAPI, BlueprintType)
class UGameSettingsSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()
public:
	UE_API virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	UE_API virtual void Deinitialize() override;

	/** The registry. Builds and applies auto-contributors on first call. */
	UE_API UGameSettingRegistry* GetOrCreateRegistry();

	/** The registry, or nullptr if one has not been provided yet. */
	UGameSettingRegistry* GetRegistry() const { return Registry; }

	/** True once a registry has been provided. */
	bool HasRegistry() const { return Registry != nullptr; }

	/**
	 * Take ownership of a freshly-built registry. Reparents the registry
	 * to this subsystem so it survives whatever built it (typically a
	 * transient settings widget). Asserts if a registry is already set;
	 * call Regenerate() on the existing one instead of swapping.
	 *
	 * Runs an auto-contributor sweep right after assignment.
	 */
	UE_API void SetRegistry(UGameSettingRegistry* InRegistry);

	/**
	 * Apply a contribution to this subsystem's registry, lazily creating
	 * the registry if needed. Pushes the produced handles onto OutHandles;
	 * the caller keeps them and removes on teardown.
	 */
	UE_API void ApplyContribution(UGameSettingsContribution* Contribution, TArray<FGameSettingHandle>& OutHandles);

private:
	/** Auto-contributors applied to this subsystem and the handles they produced. */
	struct FAppliedAutoContribution
	{
		TWeakObjectPtr<UGameSettingsAutoContributor> Contributor;
		TArray<FGameSettingHandle> Handles;
	};

	UE_API void ApplyAllKnownAutoContributors();
	UE_API void ApplySingleAutoContributor(UGameSettingsAutoContributor* Contributor);
	UE_API void RemoveAllAutoContributorHandles();

	UPROPERTY(Transient)
	TObjectPtr<UGameSettingRegistry> Registry;

	TArray<FAppliedAutoContribution> AppliedAutoContributions;

	FDelegateHandle OnAutoContributorDiscoveredHandle;
};

#undef UE_API
