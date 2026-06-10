// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingCollection.h"
#include "GameSettingRegistry.h"
#include "GameSettingValueBool.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace UE::GameSettings::Tests
{
	static UGameSettingValueBool* MakePlain(UObject* Outer, FName IdName)
	{
		UGameSettingValueBool* Setting = NewObject<UGameSettingValueBool>(Outer);
		Setting->SetSettingId(MakeTestId(IdName));
		Setting->SetDisplayName(FText::FromName(IdName));
		return Setting;
	}

	static UGameSettingCollection* MakeCollection(UObject* Outer, FName IdName)
	{
		UGameSettingCollection* Collection = NewObject<UGameSettingCollection>(Outer);
		Collection->SetSettingId(MakeTestId(IdName));
		Collection->SetDisplayName(FText::FromName(IdName));
		return Collection;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsRegistry_AddFindRemove,
                                 "System.GameSettings.Registry.AddFindRemove",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsRegistry_AddFindRemove::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	UGameSettingValueBool* Setting = MakePlain(Registry, TEXT("Lookup_A"));

	const FGameSettingHandle Handle = Registry->AddSetting(Setting);
	TestTrue(TEXT("AddSetting returns a valid handle"), Handle.IsValid());
	TestEqual(TEXT("FindSettingById resolves the setting"), Registry->FindSettingById(Setting->GetSettingId()), (UGameSetting*)Setting);
	TestEqual(TEXT("FindSettingByHandle resolves the setting"), Registry->FindSettingByHandle(Handle), (UGameSetting*)Setting);

	TestTrue(TEXT("RemoveById succeeds for a registered setting"), Registry->RemoveById(Setting->GetSettingId()));
	TestNull(TEXT("Setting is gone from id lookup after removal"), Registry->FindSettingById(MakeTestId(TEXT("Lookup_A"))));
	TestNull(TEXT("Setting is gone from handle lookup after removal"), Registry->FindSettingByHandle(Handle));

	TestFalse(TEXT("RemoveById on an unknown id returns false"), Registry->RemoveById(MakeTestId(TEXT("NeverAdded"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsRegistry_StructureChangedBroadcast,
                                 "System.GameSettings.Registry.StructureChangedBroadcast",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsRegistry_StructureChangedBroadcast::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());

	int32 BroadcastCount = 0;
	Registry->OnStructureChangedEvent.AddLambda([&BroadcastCount](UGameSettingRegistry*) { ++BroadcastCount; });

	Registry->AddSetting(MakePlain(Registry, TEXT("Evt_A")));
	TestEqual(TEXT("AddSetting fired one structure-changed event"), BroadcastCount, 1);

	const FGameSettingHandle Handle = Registry->AddSetting(MakePlain(Registry, TEXT("Evt_B")));
	TestEqual(TEXT("Second AddSetting fired another event"), BroadcastCount, 2);

	Registry->RemoveByHandle(Handle);
	TestEqual(TEXT("RemoveByHandle fired another event"), BroadcastCount, 3);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsRegistry_CollectionNesting,
                                 "System.GameSettings.Registry.CollectionNesting",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsRegistry_CollectionNesting::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());

	UGameSettingCollection* Tab = MakeCollection(Registry, TEXT("Tab_Video"));
	Registry->AddCollection(Tab);

	UGameSettingValueBool* Child = MakePlain(Registry, TEXT("Child_Vsync"));
	Registry->AddSetting(Child, Tab->GetSettingId());

	TestEqual(TEXT("Child's parent is the tab collection"), Child->GetSettingParent(), (UGameSetting*)Tab);
	TestTrue(TEXT("Tab reports the child among its settings"), Tab->GetChildSettings().Contains(Child));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsRegistry_DeferredPlacementResolves,
                                 "System.GameSettings.Registry.DeferredPlacementResolves",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsRegistry_DeferredPlacementResolves::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());

	// Child lands before its parent collection exists.
	const FPrimaryAssetId ParentId = MakeTestId(TEXT("Tab_LateAudio"));
	UGameSettingValueBool* Child = MakePlain(Registry, TEXT("Child_Subtitles"));
	Registry->AddSetting(Child, ParentId);

	TestEqual(TEXT("Deferred child is still id-findable while waiting"),
	          Registry->FindSettingById(Child->GetSettingId()), (UGameSetting*)Child);
	TestNull(TEXT("Deferred child has no parent yet"), Child->GetSettingParent());

	// Parent arrives - the deferred queue should flush and re-parent.
	UGameSettingCollection* Tab = MakeCollection(Registry, TEXT("Tab_LateAudio"));
	Registry->AddCollection(Tab);

	TestEqual(TEXT("After parent arrives the child is re-parented"), Child->GetSettingParent(), (UGameSetting*)Tab);
	TestTrue(TEXT("Tab now contains the previously-deferred child"), Tab->GetChildSettings().Contains(Child));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsRegistry_RemoveSubtree,
                                 "System.GameSettings.Registry.RemoveSubtree",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsRegistry_RemoveSubtree::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());

	UGameSettingCollection* Tab = MakeCollection(Registry, TEXT("Tab_Tree"));
	Registry->AddCollection(Tab);

	UGameSettingValueBool* ChildA = MakePlain(Registry, TEXT("Tree_ChildA"));
	UGameSettingValueBool* ChildB = MakePlain(Registry, TEXT("Tree_ChildB"));
	Registry->AddSetting(ChildA, Tab->GetSettingId());
	Registry->AddSetting(ChildB, Tab->GetSettingId());

	const FGameSettingHandle ChildAHandle = ChildA->GetHandle();

	// Removing the parent tab should drop the whole subtree.
	TestTrue(TEXT("RemoveById removes the tab"), Registry->RemoveById(Tab->GetSettingId()));
	TestNull(TEXT("Tab is no longer findable"), Registry->FindSettingById(MakeTestId(TEXT("Tab_Tree"))));
	TestNull(TEXT("ChildA was removed with the subtree"), Registry->FindSettingByHandle(ChildAHandle));
	TestNull(TEXT("ChildB was removed with the subtree"), Registry->FindSettingById(MakeTestId(TEXT("Tree_ChildB"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsRegistry_RegenerateClears,
                                 "System.GameSettings.Registry.RegenerateClears",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsRegistry_RegenerateClears::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	Registry->AddSetting(MakePlain(Registry, TEXT("Regen_A")));
	Registry->AddSetting(MakePlain(Registry, TEXT("Regen_B")));

	bool bStructureChanged = false;
	Registry->OnStructureChangedEvent.AddLambda([&bStructureChanged](UGameSettingRegistry*) { bStructureChanged = true; });

	Registry->Regenerate();

	TestTrue(TEXT("Regenerate broadcasts a structure-changed event"), bStructureChanged);
	TestNull(TEXT("Settings are cleared after Regenerate"), Registry->FindSettingById(MakeTestId(TEXT("Regen_A"))));
	TestNull(TEXT("All settings are cleared after Regenerate"), Registry->FindSettingById(MakeTestId(TEXT("Regen_B"))));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
