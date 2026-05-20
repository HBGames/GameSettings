// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingCollection.h"
#include "GameSettingFilterState.h"
#include "GameSettingValueBool.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace UE::GameSettings::Tests
{
	static UGameSettingValueBool* MakeLeaf(UObject* Outer, FName IdName, int32 SortPriority = 0)
	{
		UGameSettingValueBool* Setting = NewObject<UGameSettingValueBool>(Outer);
		Setting->SetSettingId(MakeTestId(IdName));
		Setting->SetDisplayName(FText::FromName(IdName));
		Setting->SetDescriptionRichText(FString(TEXT("desc")));
		Setting->SetSortPriority(SortPriority);
		return Setting;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsCollection_AddRemove,
                                 "System.GameSettings.Collection.AddRemove",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsCollection_AddRemove::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingCollection* Collection = NewObject<UGameSettingCollection>(GetTransientPackage());
	UGameSettingValueBool* Leaf = MakeLeaf(Collection, TEXT("Coll_Leaf"));

	Collection->AddSetting(Leaf);
	TestTrue(TEXT("Collection contains the added setting"), Collection->GetChildSettings().Contains(Leaf));
	TestEqual(TEXT("Added setting's parent is the collection"), Leaf->GetSettingParent(), (UGameSetting*)Collection);

	TestTrue(TEXT("RemoveSetting succeeds for a contained setting"), Collection->RemoveSetting(Leaf));
	TestFalse(TEXT("Collection no longer contains the setting"), Collection->GetChildSettings().Contains(Leaf));
	TestNull(TEXT("Removed setting's parent is cleared"), Leaf->GetSettingParent());
	TestFalse(TEXT("RemoveSetting on a non-member returns false"), Collection->RemoveSetting(Leaf));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsCollection_ChildCollections,
                                 "System.GameSettings.Collection.ChildCollections",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsCollection_ChildCollections::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingCollection* Root = NewObject<UGameSettingCollection>(GetTransientPackage());

	UGameSettingCollection* Section = NewObject<UGameSettingCollection>(Root);
	Section->SetSettingId(MakeTestId(TEXT("Coll_Section")));
	Section->SetDisplayName(FText::FromString(TEXT("Section")));

	UGameSettingValueBool* Leaf = MakeLeaf(Root, TEXT("Coll_Mixed_Leaf"));

	Root->AddSetting(Section);
	Root->AddSetting(Leaf);

	const TArray<UGameSettingCollection*> ChildCollections = Root->GetChildCollections();
	TestEqual(TEXT("Only the nested collection is reported as a child collection"), ChildCollections.Num(), 1);
	TestTrue(TEXT("The nested section is the reported child collection"), ChildCollections.Contains(Section));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsCollection_FilterOrdering,
                                 "System.GameSettings.Collection.FilterOrdering",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsCollection_FilterOrdering::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingCollection* Collection = NewObject<UGameSettingCollection>(GetTransientPackage());

	// Add out of priority order; GetSettingsForFilter should emit sorted by
	// SortPriority ascending (SettingId is the stable tiebreak).
	UGameSettingValueBool* Low = MakeLeaf(Collection, TEXT("Order_Low"), /*SortPriority=*/0);
	UGameSettingValueBool* High = MakeLeaf(Collection, TEXT("Order_High"), /*SortPriority=*/10);
	UGameSettingValueBool* Mid = MakeLeaf(Collection, TEXT("Order_Mid"), /*SortPriority=*/5);
	Collection->AddSetting(High);
	Collection->AddSetting(Low);
	Collection->AddSetting(Mid);

	FGameSettingFilterState Filter;
	TArray<UGameSetting*> Filtered;
	Collection->GetSettingsForFilter(Filter, Filtered);

	TestEqual(TEXT("All three leaf settings pass the empty filter"), Filtered.Num(), 3);
	if (Filtered.Num() == 3)
	{
		TestEqual(TEXT("Lowest priority emitted first"), Filtered[0], (UGameSetting*)Low);
		TestEqual(TEXT("Middle priority emitted second"), Filtered[1], (UGameSetting*)Mid);
		TestEqual(TEXT("Highest priority emitted last"), Filtered[2], (UGameSetting*)High);
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
