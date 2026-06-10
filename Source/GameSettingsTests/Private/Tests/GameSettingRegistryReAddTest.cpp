// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingCollection.h"
#include "GameSettingFilterState.h"
#include "GameSettingRegistry.h"
#include "GameSettingValueBool.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace UE::GameSettings::Tests
{
	static UGameSettingCollection* MakeReAddCollection(UObject* Outer, FName IdName)
	{
		UGameSettingCollection* Collection = NewObject<UGameSettingCollection>(Outer);
		Collection->SetSettingId(MakeTestId(IdName));
		Collection->SetDisplayName(FText::FromName(IdName));
		return Collection;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsRegistry_ReAddSettingIsIdempotent,
                                 "System.GameSettings.Registry.ReAddSettingIsIdempotent",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsRegistry_ReAddSettingIsIdempotent::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	// The re-add guard warns by design; mark it expected so it doesn't pollute the run.
	AddExpectedMessagePlain(TEXT("ignoring re-add"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>(TEXT("false"));
	UGameSettingValueBool* Setting = MakeBool(Registry, TEXT("ReAdd_Bool"), Store);

	const FGameSettingHandle FirstHandle = Registry->AddSetting(Setting);
	TestTrue(TEXT("First add returns a valid handle"), FirstHandle.IsValid());

	int32 StructureCount = 0;
	Registry->OnStructureChangedEvent.AddLambda([&StructureCount](UGameSettingRegistry*) { ++StructureCount; });

	const FGameSettingHandle SecondHandle = Registry->AddSetting(Setting);
	TestTrue(TEXT("Re-add returns the setting's EXISTING handle"), SecondHandle == FirstHandle);
	TestEqual(TEXT("Re-add does not broadcast a structure change (placement didn't re-run)"), StructureCount, 0);

	// Delegates must not have been double-bound: one mutation, one broadcast.
	int32 ChangeCount = 0;
	Registry->OnSettingChangedEvent.AddLambda([&ChangeCount](UGameSetting*, EGameSettingChangeReason) { ++ChangeCount; });
	Setting->SetBoolValue(true);
	TestEqual(TEXT("One change after a re-add broadcasts exactly once"), ChangeCount, 1);

	// The setting must appear exactly once in a filtered walk (no duplicate top-level placement).
	FGameSettingFilterState Filter;
	TArray<UGameSetting*> Filtered;
	Registry->GetSettingsForFilter(Filter, Filtered);
	int32 Occurrences = 0;
	for (UGameSetting* Entry : Filtered)
	{
		if (Entry == Setting)
		{
			++Occurrences;
		}
	}
	TestEqual(TEXT("Setting appears exactly once in the filtered walk"), Occurrences, 1);

	// The original handle still removes cleanly.
	TestTrue(TEXT("RemoveByHandle with the original handle still works"), Registry->RemoveByHandle(FirstHandle));
	TestNull(TEXT("Setting gone from handle lookup after removal"), Registry->FindSettingByHandle(FirstHandle));
	TestNull(TEXT("Setting gone from id lookup after removal"), Registry->FindSettingById(MakeTestId(TEXT("ReAdd_Bool"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsRegistry_ReAddCollectionKeepsHandle,
                                 "System.GameSettings.Registry.ReAddCollectionKeepsHandle",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsRegistry_ReAddCollectionKeepsHandle::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	// One warning from the direct AddCollection re-add, one from the
	// AddSetting forward (which routes collections through AddCollection).
	AddExpectedMessagePlain(TEXT("ignoring re-add"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 2);

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	UGameSettingCollection* Tab = MakeReAddCollection(Registry, TEXT("ReAdd_Tab"));

	const FGameSettingHandle FirstHandle = Registry->AddCollection(Tab);
	TestTrue(TEXT("First add returns a valid handle"), FirstHandle.IsValid());

	int32 StructureCount = 0;
	Registry->OnStructureChangedEvent.AddLambda([&StructureCount](UGameSettingRegistry*) { ++StructureCount; });

	const FGameSettingHandle ReAddHandle = Registry->AddCollection(Tab);
	TestTrue(TEXT("AddCollection re-add returns the existing handle"), ReAddHandle == FirstHandle);

	const FGameSettingHandle ViaAddSetting = Registry->AddSetting(Tab);
	TestTrue(TEXT("AddSetting on a registered collection also returns the existing handle"), ViaAddSetting == FirstHandle);

	TestEqual(TEXT("Neither re-add broadcast a structure change"), StructureCount, 0);

	TestTrue(TEXT("RemoveByHandle with the original handle still works"), Registry->RemoveByHandle(FirstHandle));
	TestNull(TEXT("Collection gone after removal"), Registry->FindSettingById(MakeTestId(TEXT("ReAdd_Tab"))));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
