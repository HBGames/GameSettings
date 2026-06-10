// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingRegistry.h"
#include "GameSettingRegistryChangeTracker.h"
#include "GameSettingValueBool.h"
#include "GameSettingsTestHelpers.h"
#include "GameSettingsTestTypes.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace UE::GameSettings::Tests
{
	/** Bool setting registered with the given registry and wired to its own store. */
	static UGameSettingValueBool* MakeTrackedBool(UGameSettingRegistry* Registry, FName IdName)
	{
		UGameSettingValueBool* Setting = MakeBool(Registry, IdName, MakeShared<FInMemoryDataSource>(TEXT("false")));
		Registry->AddSetting(Setting);
		return Setting;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsChangeTracker_DirtyApplyAndClear,
                                 "System.GameSettings.ChangeTracker.DirtyApplyAndClear",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsChangeTracker_DirtyApplyAndClear::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	UGameSettingValueBool* Dirty = MakeTrackedBool(Registry, TEXT("Trk_Dirty"));
	UGameSettingValueBool* Clean = MakeTrackedBool(Registry, TEXT("Trk_Clean"));

	int32 DirtyAppliedCount = 0;
	int32 CleanAppliedCount = 0;
	Dirty->OnSettingAppliedEvent.AddLambda([&DirtyAppliedCount](UGameSetting*) { ++DirtyAppliedCount; });
	Clean->OnSettingAppliedEvent.AddLambda([&CleanAppliedCount](UGameSetting*) { ++CleanAppliedCount; });

	FGameSettingRegistryChangeTracker Tracker;
	Tracker.WatchRegistry(Registry);
	TestFalse(TEXT("Nothing changed yet"), Tracker.HaveSettingsBeenChanged());

	Dirty->StoreInitial();
	Dirty->SetBoolValue(true);
	TestTrue(TEXT("A change marks the tracker dirty"), Tracker.HaveSettingsBeenChanged());

	Tracker.ApplyChanges();
	TestEqual(TEXT("The dirty setting was applied exactly once"), DirtyAppliedCount, 1);
	TestEqual(TEXT("The untouched setting was not applied"), CleanAppliedCount, 0);
	TestFalse(TEXT("ApplyChanges clears the dirty state"), Tracker.HaveSettingsBeenChanged());

	// ApplyChanges also re-stores the initial value: a subsequent change +
	// RestoreToInitial lands on the APPLIED value, not the original one.
	Dirty->SetBoolValue(false);
	TestTrue(TEXT("Post-apply change re-dirties the tracker"), Tracker.HaveSettingsBeenChanged());
	Tracker.RestoreToInitial();
	TestTrue(TEXT("RestoreToInitial returns to the value stored at ApplyChanges"), Dirty->GetBoolValue());

	Tracker.StopWatchingRegistry();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsChangeTracker_RestoreToInitial,
                                 "System.GameSettings.ChangeTracker.RestoreToInitial",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsChangeTracker_RestoreToInitial::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	UGameSettingValueBool* Setting = MakeTrackedBool(Registry, TEXT("Trk_Restore"));
	Setting->StoreInitial();

	FGameSettingRegistryChangeTracker Tracker;
	Tracker.WatchRegistry(Registry);

	Setting->SetBoolValue(true);
	TestTrue(TEXT("Change marks the tracker dirty"), Tracker.HaveSettingsBeenChanged());

	Tracker.RestoreToInitial();
	TestFalse(TEXT("Value was restored to its initial"), Setting->GetBoolValue());
	TestFalse(TEXT("Restore clears the dirty state"), Tracker.HaveSettingsBeenChanged());
	// The restore-driven change events must not have re-dirtied the tracker
	// (bRestoringSettings guard) - covered by the assertion above.

	Tracker.StopWatchingRegistry();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsChangeTracker_ReentrantApplyGuard,
                                 "System.GameSettings.ChangeTracker.ReentrantApplyGuard",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsChangeTracker_ReentrantApplyGuard::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());

	// Bystander is watched but NOT dirty; CrossApply's Apply() flips it
	// mid-transaction, which would grow DirtySettings while ApplyChanges
	// iterates it if the bApplyingSettings guard were missing.
	UGameSettingValueBool* Bystander = MakeTrackedBool(Registry, TEXT("Trk_Bystander"));

	UGameSettingsTest_CrossApplyBool* CrossApply = NewObject<UGameSettingsTest_CrossApplyBool>(Registry);
	CrossApply->SetSettingId(MakeTestId(TEXT("Trk_CrossApply")));
	CrossApply->SetDisplayName(FText::FromString(TEXT("Trk_CrossApply")));
	TSharedRef<FInMemoryDataSource> CrossStore = MakeShared<FInMemoryDataSource>(TEXT("false"));
	CrossApply->SetDynamicGetter(CrossStore);
	CrossApply->SetDynamicSetter(CrossStore);
	CrossApply->SettingToMutateOnApply = Bystander;
	Registry->AddSetting(CrossApply);

	UGameSettingValueBool* Plain = MakeTrackedBool(Registry, TEXT("Trk_Plain"));

	int32 BystanderAppliedCount = 0;
	int32 PlainAppliedCount = 0;
	Bystander->OnSettingAppliedEvent.AddLambda([&BystanderAppliedCount](UGameSetting*) { ++BystanderAppliedCount; });
	Plain->OnSettingAppliedEvent.AddLambda([&PlainAppliedCount](UGameSetting*) { ++PlainAppliedCount; });

	FGameSettingRegistryChangeTracker Tracker;
	Tracker.WatchRegistry(Registry);

	CrossApply->SetBoolValue(true);
	Plain->SetBoolValue(true);
	TestTrue(TEXT("Two settings are dirty"), Tracker.HaveSettingsBeenChanged());
	TestFalse(TEXT("Bystander starts false"), Bystander->GetBoolValue());

	// Must not corrupt the dirty-set iteration even though CrossApply's
	// Apply() fires a change on a setting that wasn't dirty before.
	Tracker.ApplyChanges();

	TestTrue(TEXT("CrossApply's Apply mutated the bystander"), Bystander->GetBoolValue());
	TestEqual(TEXT("Bystander was NOT swept into the apply pass"), BystanderAppliedCount, 0);
	TestEqual(TEXT("The other dirty setting still applied exactly once"), PlainAppliedCount, 1);
	TestFalse(TEXT("Tracker is clean after the transaction (mid-apply mutation did not re-dirty it)"), Tracker.HaveSettingsBeenChanged());

	Tracker.StopWatchingRegistry();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsChangeTracker_RemovedSettingSkipped,
                                 "System.GameSettings.ChangeTracker.RemovedSettingSkipped",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsChangeTracker_RemovedSettingSkipped::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	UGameSettingValueBool* Doomed = MakeTrackedBool(Registry, TEXT("Trk_Doomed"));
	UGameSettingValueBool* Survivor = MakeTrackedBool(Registry, TEXT("Trk_Survivor"));

	int32 DoomedAppliedCount = 0;
	int32 SurvivorAppliedCount = 0;
	Doomed->OnSettingAppliedEvent.AddLambda([&DoomedAppliedCount](UGameSetting*) { ++DoomedAppliedCount; });
	Survivor->OnSettingAppliedEvent.AddLambda([&SurvivorAppliedCount](UGameSetting*) { ++SurvivorAppliedCount; });

	FGameSettingRegistryChangeTracker Tracker;
	Tracker.WatchRegistry(Registry);

	Doomed->SetBoolValue(true);
	Survivor->SetBoolValue(true);
	TestTrue(TEXT("Both settings are dirty"), Tracker.HaveSettingsBeenChanged());

	// Remove one mid-transaction (a GFP deactivation can do this). The dirty
	// map holds it weakly, so ApplyChanges must skip it without crashing.
	TestTrue(TEXT("Doomed setting removed from the registry"), Registry->RemoveByHandle(Doomed->GetHandle()));

	Tracker.ApplyChanges();
	TestEqual(TEXT("The removed setting was skipped"), DoomedAppliedCount, 0);
	TestEqual(TEXT("The surviving setting applied exactly once"), SurvivorAppliedCount, 1);
	TestFalse(TEXT("Tracker is clean after the transaction"), Tracker.HaveSettingsBeenChanged());

	Tracker.StopWatchingRegistry();
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
