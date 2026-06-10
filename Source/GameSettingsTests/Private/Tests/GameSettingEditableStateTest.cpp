// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingFilterState.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsEditableState_Defaults,
                                 "System.GameSettings.EditableState.Defaults",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsEditableState_Defaults::RunTest(const FString& Parameters)
{
	FGameSettingEditableState State;
	TestTrue(TEXT("Default state is visible"), State.IsVisible());
	TestTrue(TEXT("Default state is enabled"), State.IsEnabled());
	TestTrue(TEXT("Default state is resetable"), State.IsResetable());
	TestFalse(TEXT("Default state is not hidden from analytics"), State.IsHiddenFromAnalytics());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsEditableState_HideDisableReset,
                                 "System.GameSettings.EditableState.HideDisableReset",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsEditableState_HideDisableReset::RunTest(const FString& Parameters)
{
	FGameSettingEditableState State;

	State.Disable(NSLOCTEXT("GameSettingsTests", "DisabledReason", "Disabled for test"));
	TestFalse(TEXT("Disable clears the enabled flag"), State.IsEnabled());
	TestEqual(TEXT("Disable records one reason"), State.GetDisabledReasons().Num(), 1);
	TestTrue(TEXT("Disable leaves visibility untouched"), State.IsVisible());

	State.Hide(TEXT("Hidden for test"));
	TestFalse(TEXT("Hide clears the visible flag"), State.IsVisible());

	State.UnableToReset();
	TestFalse(TEXT("UnableToReset clears the resetable flag"), State.IsResetable());

	State.DisableOption(TEXT("OptionX"));
	TestTrue(TEXT("DisableOption records the option"), State.GetDisabledOptions().Contains(TEXT("OptionX")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsEditableState_Kill,
                                 "System.GameSettings.EditableState.Kill",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsEditableState_Kill::RunTest(const FString& Parameters)
{
	FGameSettingEditableState State;
	State.Kill(TEXT("Killed for test"));

	TestFalse(TEXT("Kill hides the setting"), State.IsVisible());
	TestTrue(TEXT("Kill hides from analytics"), State.IsHiddenFromAnalytics());
	TestFalse(TEXT("Kill marks it unable to reset"), State.IsResetable());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
