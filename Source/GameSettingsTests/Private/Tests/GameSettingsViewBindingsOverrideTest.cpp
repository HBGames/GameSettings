// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingsModule.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/GameSettingsViewBindings.h"

namespace UE::GameSettings::Tests
{
	/**
	 * The override stack is global module state and other systems (active GFPs)
	 * may have entries on it, so tests assert only on the relative order /
	 * presence of their own bindings and always remove what they added.
	 */
	static TArray<UGameSettingsViewBindings*> FilterToOurs(
		const TArray<UGameSettingsViewBindings*>& Active,
		const TArray<UGameSettingsViewBindings*>& Ours)
	{
		return Active.FilterByPredicate([&Ours](UGameSettingsViewBindings* Entry) { return Ours.Contains(Entry); });
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsViewBindings_PriorityOrdering,
                                 "System.GameSettings.ViewBindings.PriorityOrdering",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsViewBindings_PriorityOrdering::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	FGameSettingsModule& Module = FGameSettingsModule::Get();

	UGameSettingsViewBindings* LowFirst = NewObject<UGameSettingsViewBindings>(GetTransientPackage());
	UGameSettingsViewBindings* High = NewObject<UGameSettingsViewBindings>(GetTransientPackage());
	UGameSettingsViewBindings* LowSecond = NewObject<UGameSettingsViewBindings>(GetTransientPackage());
	const TArray<UGameSettingsViewBindings*> Ours = {LowFirst, High, LowSecond};

	const FGuid LowFirstHandle = Module.AddViewBindingsOverride(LowFirst, 10);
	const FGuid HighHandle = Module.AddViewBindingsOverride(High, 20);
	const FGuid LowSecondHandle = Module.AddViewBindingsOverride(LowSecond, 10);
	TestTrue(TEXT("Add returns valid handles"), LowFirstHandle.IsValid() && HighHandle.IsValid() && LowSecondHandle.IsValid());

	const TArray<UGameSettingsViewBindings*> Ordered = FilterToOurs(Module.GetActiveViewBindings(), Ours);
	TestEqual(TEXT("All three overrides are active"), Ordered.Num(), 3);
	if (Ordered.Num() == 3)
	{
		TestEqual(TEXT("Higher priority is consulted first"), Ordered[0], High);
		TestEqual(TEXT("Tie-break: the OLDER equal-priority entry wins"), Ordered[1], LowFirst);
		TestEqual(TEXT("Tie-break: the newer equal-priority entry sorts after"), Ordered[2], LowSecond);
	}

	Module.RemoveViewBindingsOverride(LowFirstHandle);
	Module.RemoveViewBindingsOverride(HighHandle);
	Module.RemoveViewBindingsOverride(LowSecondHandle);
	TestEqual(TEXT("Cleanup removed all of our overrides"), FilterToOurs(Module.GetActiveViewBindings(), Ours).Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsViewBindings_RemoveByHandle,
                                 "System.GameSettings.ViewBindings.RemoveByHandle",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsViewBindings_RemoveByHandle::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	FGameSettingsModule& Module = FGameSettingsModule::Get();

	UGameSettingsViewBindings* First = NewObject<UGameSettingsViewBindings>(GetTransientPackage());
	UGameSettingsViewBindings* Second = NewObject<UGameSettingsViewBindings>(GetTransientPackage());
	const TArray<UGameSettingsViewBindings*> Ours = {First, Second};

	TestFalse(TEXT("Adding a null bindings object returns an invalid handle"),
	          Module.AddViewBindingsOverride(nullptr, 0).IsValid());

	const FGuid FirstHandle = Module.AddViewBindingsOverride(First, 0);
	const FGuid SecondHandle = Module.AddViewBindingsOverride(Second, 0);
	TestEqual(TEXT("Both overrides are active"), FilterToOurs(Module.GetActiveViewBindings(), Ours).Num(), 2);

	Module.RemoveViewBindingsOverride(FirstHandle);
	TArray<UGameSettingsViewBindings*> Remaining = FilterToOurs(Module.GetActiveViewBindings(), Ours);
	TestEqual(TEXT("One override remains after removal"), Remaining.Num(), 1);
	TestTrue(TEXT("The remaining override is the un-removed one"), Remaining.Contains(Second));

	// Double-remove and bogus handles are harmless no-ops.
	Module.RemoveViewBindingsOverride(FirstHandle);
	Module.RemoveViewBindingsOverride(FGuid());
	TestEqual(TEXT("Repeat / invalid removals change nothing"), FilterToOurs(Module.GetActiveViewBindings(), Ours).Num(), 1);

	Module.RemoveViewBindingsOverride(SecondHandle);
	TestEqual(TEXT("Cleanup removed the last override"), FilterToOurs(Module.GetActiveViewBindings(), Ours).Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsViewBindings_ChangeBroadcasts,
                                 "System.GameSettings.ViewBindings.ChangeBroadcasts",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsViewBindings_ChangeBroadcasts::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	FGameSettingsModule& Module = FGameSettingsModule::Get();

	int32 BroadcastCount = 0;
	const FDelegateHandle SubscriptionHandle =
		Module.OnViewBindingsOverridesChanged.AddLambda([&BroadcastCount]() { ++BroadcastCount; });

	UGameSettingsViewBindings* Bindings = NewObject<UGameSettingsViewBindings>(GetTransientPackage());

	const FGuid Handle = Module.AddViewBindingsOverride(Bindings, 0);
	TestEqual(TEXT("Add broadcasts once"), BroadcastCount, 1);

	Module.AddViewBindingsOverride(nullptr, 0);
	TestEqual(TEXT("A rejected null add does not broadcast"), BroadcastCount, 1);

	Module.RemoveViewBindingsOverride(Handle);
	TestEqual(TEXT("Remove broadcasts once"), BroadcastCount, 2);

	Module.RemoveViewBindingsOverride(Handle);
	TestEqual(TEXT("Removing an already-removed handle does not broadcast"), BroadcastCount, 2);

	Module.RemoveViewBindingsOverride(FGuid());
	TestEqual(TEXT("Removing an invalid handle does not broadcast"), BroadcastCount, 2);

	Module.OnViewBindingsOverridesChanged.Remove(SubscriptionHandle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsViewBindings_DeadEntriesFiltered,
                                 "System.GameSettings.ViewBindings.DeadEntriesFiltered",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsViewBindings_DeadEntriesFiltered::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	FGameSettingsModule& Module = FGameSettingsModule::Get();

	// Keeper survives GC via the strong pointer; Doomed is unreferenced (the
	// override stack only holds weak refs) and gets collected.
	TStrongObjectPtr<UGameSettingsViewBindings> Keeper(NewObject<UGameSettingsViewBindings>(GetTransientPackage()));
	UGameSettingsViewBindings* Doomed = NewObject<UGameSettingsViewBindings>(GetTransientPackage());
	TWeakObjectPtr<UGameSettingsViewBindings> DoomedWeak = Doomed;

	const FGuid KeeperHandle = Module.AddViewBindingsOverride(Keeper.Get(), 0);
	const FGuid DoomedHandle = Module.AddViewBindingsOverride(Doomed, 0);

	{
		const TArray<UGameSettingsViewBindings*> Active = Module.GetActiveViewBindings();
		TestTrue(TEXT("Pre-GC: keeper is active"), Active.Contains(Keeper.Get()));
		TestTrue(TEXT("Pre-GC: doomed is active"), Active.Contains(Doomed));
	}

	Doomed = nullptr;
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
	TestFalse(TEXT("GC actually collected the unreferenced bindings asset"), DoomedWeak.IsValid());

	const TArray<UGameSettingsViewBindings*> Active = Module.GetActiveViewBindings();
	TestTrue(TEXT("Post-GC: keeper is still active"), Active.Contains(Keeper.Get()));
	for (UGameSettingsViewBindings* Entry : Active)
	{
		TestNotNull(TEXT("GetActiveViewBindings never returns a dead entry"), Entry);
	}

	// Removing the stale entry by handle still works (cleans internal state).
	Module.RemoveViewBindingsOverride(DoomedHandle);
	Module.RemoveViewBindingsOverride(KeeperHandle);
	TestFalse(TEXT("Cleanup: keeper removed from the stack"), Module.GetActiveViewBindings().Contains(Keeper.Get()));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
