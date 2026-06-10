// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingFilterState.h"
#include "GameSettingValueBool.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace UE::GameSettings::Tests
{
	static UGameSettingValueBool* MakeFilterSetting(UObject* Outer, FName IdName, const TCHAR* Description)
	{
		UGameSettingValueBool* Setting = NewObject<UGameSettingValueBool>(Outer);
		Setting->SetSettingId(MakeTestId(IdName));
		Setting->SetDisplayName(FText::FromName(IdName));
		Setting->SetDescriptionRichText(FString(Description));
		return Setting;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsFilter_DefaultPasses,
                                 "System.GameSettings.Filter.DefaultPasses",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsFilter_DefaultPasses::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingValueBool* Setting = MakeFilterSetting(GetTransientPackage(), TEXT("Filt_A"), TEXT("Brightness level"));

	FGameSettingFilterState Filter;
	TestTrue(TEXT("A default-state setting passes an empty filter"),
	         Filter.DoesSettingPassFilter(*Setting));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsFilter_SearchText,
                                 "System.GameSettings.Filter.SearchText",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsFilter_SearchText::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingValueBool* Setting = MakeFilterSetting(GetTransientPackage(), TEXT("Filt_Search"), TEXT("Controls the master volume"));

	FGameSettingFilterState Filter;
	Filter.SetSearchText(TEXT("volume"));
	TestTrue(TEXT("Matching search text passes"), Filter.DoesSettingPassFilter(*Setting));

	FGameSettingFilterState MissFilter;
	MissFilter.SetSearchText(TEXT("resolution"));
	TestFalse(TEXT("Non-matching search text is filtered out"), MissFilter.DoesSettingPassFilter(*Setting));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsFilter_AllowList,
                                 "System.GameSettings.Filter.AllowList",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsFilter_AllowList::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingValueBool* Allowed = MakeFilterSetting(GetTransientPackage(), TEXT("Filt_Allowed"), TEXT("A"));
	UGameSettingValueBool* Excluded = MakeFilterSetting(GetTransientPackage(), TEXT("Filt_Excluded"), TEXT("B"));
	UGameSettingValueBool* Child = MakeFilterSetting(GetTransientPackage(), TEXT("Filt_Child"), TEXT("C"));
	Child->SetSettingParent(Allowed);

	FGameSettingFilterState Filter;
	Filter.AddSettingToAllowList(Allowed);

	TestTrue(TEXT("A setting on the allow list passes"), Filter.DoesSettingPassFilter(*Allowed));
	TestTrue(TEXT("A child of an allow-listed setting passes via parent walk"), Filter.DoesSettingPassFilter(*Child));
	TestFalse(TEXT("A setting absent from a non-empty allow list is filtered out"), Filter.DoesSettingPassFilter(*Excluded));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsFilter_RootList,
                                 "System.GameSettings.Filter.RootList",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsFilter_RootList::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingValueBool* Root = MakeFilterSetting(GetTransientPackage(), TEXT("Filt_Root"), TEXT("R"));

	FGameSettingFilterState Filter;
	Filter.AddSettingToRootList(Root);

	TestTrue(TEXT("Root-listed setting reports as in the root list"), Filter.IsSettingInRootList(Root));
	TestTrue(TEXT("Root-listed setting is also on the allow list"), Filter.IsSettingInAllowList(Root));
	TestEqual(TEXT("Root list contains exactly the one setting"), Filter.GetSettingRootList().Num(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
