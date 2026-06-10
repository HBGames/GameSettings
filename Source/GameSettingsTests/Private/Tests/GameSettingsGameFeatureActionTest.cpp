// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameFeatureAction_AddViewBindings.h"
#include "GameFeatureAction_RegisterGameSettings.h"
#include "GameFeaturesSubsystem.h"
#include "GameSettingsModule.h"
#include "GameSettingsTestHelpers.h"
#include "GameSettingsTestTypes.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/GameSettingsViewBindings.h"

namespace UE::GameSettings::Tests
{
	/**
	 * Activation/deactivation contexts scoped to a world-context handle that
	 * matches no real world context, so the actions exercise their per-context
	 * bookkeeping without touching live game instances or local players.
	 */
	static FGameFeatureActivatingContext MakeActivatingContext(FName WorldContextHandle)
	{
		FGameFeatureActivatingContext Context;
		Context.SetRequiredWorldContextHandle(WorldContextHandle);
		return Context;
	}

	static FGameFeatureDeactivatingContext MakeDeactivatingContext(FName WorldContextHandle)
	{
		FGameFeatureDeactivatingContext Context(TEXTVIEW("GameSettingsTests"), [](FStringView) {});
		Context.SetRequiredWorldContextHandle(WorldContextHandle);
		return Context;
	}

	/** How many times Bindings currently appears on the module's override stack. */
	static int32 CountActiveOccurrences(const UGameSettingsViewBindings* Bindings)
	{
		int32 Count = 0;
		for (UGameSettingsViewBindings* Entry : FGameSettingsModule::Get().GetActiveViewBindings())
		{
			if (Entry == Bindings)
			{
				++Count;
			}
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsGameFeatures_AddViewBindingsPerContext,
                                 "System.GameSettings.GameFeatures.AddViewBindingsPerContext",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsGameFeatures_AddViewBindingsPerContext::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	TStrongObjectPtr<UGameSettingsViewBindings> Bindings(NewObject<UGameSettingsViewBindings>(GetTransientPackage()));
	TStrongObjectPtr<UGameFeatureAction_AddViewBindings> Action(NewObject<UGameFeatureAction_AddViewBindings>(GetTransientPackage()));
	Action->Bindings = Bindings.Get();
	Action->Priority = 0;

	TestEqual(TEXT("Stack starts without our bindings"), CountActiveOccurrences(Bindings.Get()), 0);

	// Two independent activation contexts each push their own override.
	FGameFeatureActivatingContext ContextA = MakeActivatingContext(TEXT("GameSettingsTests_CtxA"));
	FGameFeatureActivatingContext ContextB = MakeActivatingContext(TEXT("GameSettingsTests_CtxB"));

	Action->OnGameFeatureActivating(ContextA);
	TestEqual(TEXT("First context pushed one override"), CountActiveOccurrences(Bindings.Get()), 1);

	Action->OnGameFeatureActivating(ContextB);
	TestEqual(TEXT("Second context pushed its own override"), CountActiveOccurrences(Bindings.Get()), 2);

	// Deactivating one context pops only that context's override.
	FGameFeatureDeactivatingContext DeactivatingA = MakeDeactivatingContext(TEXT("GameSettingsTests_CtxA"));
	Action->OnGameFeatureDeactivating(DeactivatingA);
	TestEqual(TEXT("Deactivating context A leaves context B's override"), CountActiveOccurrences(Bindings.Get()), 1);

	FGameFeatureDeactivatingContext DeactivatingB = MakeDeactivatingContext(TEXT("GameSettingsTests_CtxB"));
	Action->OnGameFeatureDeactivating(DeactivatingB);
	TestEqual(TEXT("Deactivating context B empties the stack of our bindings"), CountActiveOccurrences(Bindings.Get()), 0);

	// Per-context state was fully cleaned up: the same context can activate again.
	FGameFeatureActivatingContext ContextA2 = MakeActivatingContext(TEXT("GameSettingsTests_CtxA"));
	Action->OnGameFeatureActivating(ContextA2);
	TestEqual(TEXT("Re-activation after a clean deactivate works"), CountActiveOccurrences(Bindings.Get()), 1);

	FGameFeatureDeactivatingContext DeactivatingA2 = MakeDeactivatingContext(TEXT("GameSettingsTests_CtxA"));
	Action->OnGameFeatureDeactivating(DeactivatingA2);
	TestEqual(TEXT("Final cleanup leaves the stack without our bindings"), CountActiveOccurrences(Bindings.Get()), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsGameFeatures_RegisterGameSettingsContextScoping,
                                 "System.GameSettings.GameFeatures.RegisterGameSettingsContextScoping",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsGameFeatures_RegisterGameSettingsContextScoping::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	// The full apply path needs a real LocalPlayer with an initialized
	// subsystem collection, which an automation test doesn't have. What IS
	// verifiable here: per-context activate/deactivate bookkeeping holds up
	// across two independent contexts (any double-activate / stale-state /
	// missing-context bug trips the action's ensures, failing this test), and
	// a context whose world-context filter matches nothing never applies its
	// contributions.
	TStrongObjectPtr<UGameSettingsTest_CollectionContribution> Contribution(
		NewObject<UGameSettingsTest_CollectionContribution>(GetTransientPackage()));
	Contribution->IdPrefix = TEXT("GfaScoped");

	TStrongObjectPtr<UGameFeatureAction_RegisterGameSettings> Action(
		NewObject<UGameFeatureAction_RegisterGameSettings>(GetTransientPackage()));
	Action->Contributions.Add(Contribution.Get());

	FGameFeatureActivatingContext ContextA = MakeActivatingContext(TEXT("GameSettingsTests_GfaCtxA"));
	FGameFeatureActivatingContext ContextB = MakeActivatingContext(TEXT("GameSettingsTests_GfaCtxB"));

	Action->OnGameFeatureActivating(ContextA);
	Action->OnGameFeatureActivating(ContextB);
	TestEqual(TEXT("No world context matched, so the contribution never applied"), Contribution->ApplyCount, 0);

	FGameFeatureDeactivatingContext DeactivatingA = MakeDeactivatingContext(TEXT("GameSettingsTests_GfaCtxA"));
	Action->OnGameFeatureDeactivating(DeactivatingA);

	FGameFeatureDeactivatingContext DeactivatingB = MakeDeactivatingContext(TEXT("GameSettingsTests_GfaCtxB"));
	Action->OnGameFeatureDeactivating(DeactivatingB);

	// Re-activation of a cleanly-deactivated context must not trip the
	// double-activation ensures.
	FGameFeatureActivatingContext ContextA2 = MakeActivatingContext(TEXT("GameSettingsTests_GfaCtxA"));
	Action->OnGameFeatureActivating(ContextA2);
	FGameFeatureDeactivatingContext DeactivatingA2 = MakeDeactivatingContext(TEXT("GameSettingsTests_GfaCtxA"));
	Action->OnGameFeatureDeactivating(DeactivatingA2);

	TestEqual(TEXT("Contribution still never applied"), Contribution->ApplyCount, 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
