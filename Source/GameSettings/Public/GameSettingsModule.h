// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Misc/Guid.h"
#include "Modules/ModuleInterface.h"
#include "Modules/ModuleManager.h"
#include "UObject/WeakObjectPtrTemplates.h"

#define UE_API GAMESETTINGS_API

class UGameSettingsAutoContributor;
class UGameSettingsViewBindings;

/**
 * Runtime module entry point. Owns the global registry of
 * UGameSettingsAutoContributor CDOs discovered across loaded modules.
 *
 * Subsystems subscribe to OnAutoContributorDiscovered so they pick up
 * contributors that arrive after the LocalPlayer has already been
 * initialized (e.g. a GameFeaturePlugin loaded mid-session).
 */
class FGameSettingsModule : public IModuleInterface
{
public:
	static FGameSettingsModule& Get()
	{
		return FModuleManager::LoadModuleChecked<FGameSettingsModule>(TEXT("GameSettings"));
	}

	/** Like Get() but returns nullptr instead of asserting if the module is gone (shutdown ordering). */
	static FGameSettingsModule* GetPtr()
	{
		return FModuleManager::GetModulePtr<FGameSettingsModule>(TEXT("GameSettings"));
	}

	/** All auto-contributor CDOs discovered so far, filtered to live objects. */
	UE_API TArray<UGameSettingsAutoContributor*> GetAutoContributors() const;

	/** Fires when a previously-unseen UGameSettingsAutoContributor subclass is discovered. */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAutoContributorDiscovered, UGameSettingsAutoContributor* /*Contributor*/);
	FOnAutoContributorDiscovered OnAutoContributorDiscovered;

	// --- View bindings override stack --------------------------------------

	/**
	 * Push a UGameSettingsViewBindings asset onto the override stack.
	 * Higher Priority is consulted first; ties resolve in insertion order.
	 * Returns a handle the caller retains for symmetric removal.
	 */
	UE_API FGuid AddViewBindingsOverride(UGameSettingsViewBindings* Bindings, int32 Priority);

	/** Remove a previously-added override by handle. */
	UE_API void RemoveViewBindingsOverride(const FGuid& OverrideHandle);

	/** Active overrides ordered highest priority first. */
	UE_API TArray<UGameSettingsViewBindings*> GetActiveViewBindings() const;

	/** Fires after AddViewBindingsOverride / RemoveViewBindingsOverride. */
	DECLARE_MULTICAST_DELEGATE(FOnViewBindingsOverridesChanged);
	FOnViewBindingsOverridesChanged OnViewBindingsOverridesChanged;

	UE_API virtual void StartupModule() override;
	UE_API virtual void ShutdownModule() override;

private:
	UE_API void SweepAutoContributors();
	UE_API void OnModulesChanged(FName ModuleThatChanged, EModuleChangeReason ReasonForChange);
	UE_API void PruneDeadContributors();

	/**
	 * Discovered contributor CDOs. Held weakly: a module class outside a
	 * UPROPERTY is invisible to the GC, and a CDO dies when its defining
	 * module unloads (hot-reload, GFP code modules), so a strong pointer
	 * here would dangle. Dead entries are pruned on module unload and
	 * filtered out by GetAutoContributors().
	 */
	TArray<TWeakObjectPtr<UGameSettingsAutoContributor>> AutoContributors;

	/** Class set so we don't double-register. */
	TSet<TWeakObjectPtr<UClass>> KnownContributorClasses;

	FDelegateHandle ModulesChangedHandle;

	struct FViewBindingsOverride
	{
		FGuid Handle;
		TWeakObjectPtr<UGameSettingsViewBindings> Bindings;
		int32 Priority = 0;
	};

	/** Sorted highest-priority-first; latest insertion wins on ties. */
	TArray<FViewBindingsOverride> ViewBindingsOverrides;
};

#undef UE_API
