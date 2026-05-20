// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingFilterState.h"
#include "GameSettingValueBool.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueBool_GetSetRoundTrip,
                                 "System.GameSettings.ValueBool.GetSetRoundTrip",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsValueBool_GetSetRoundTrip::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>(TEXT("false"));
	UGameSettingValueBool* Setting = MakeBool(GetTransientPackage(), TEXT("Bool_RT"), Store);

	TestFalse(TEXT("Initial bool reads false"), Setting->GetBoolValue());

	Setting->SetBoolValue(true);
	TestTrue(TEXT("After SetBoolValue(true) it reads true"), Setting->GetBoolValue());

	Setting->SetBoolValue(false);
	TestFalse(TEXT("After SetBoolValue(false) it reads false"), Setting->GetBoolValue());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueBool_NoGetterIsFalse,
                                 "System.GameSettings.ValueBool.NoGetterIsFalse",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsValueBool_NoGetterIsFalse::RunTest(const FString& Parameters)
{
	UGameSettingValueBool* Setting = NewObject<UGameSettingValueBool>(GetTransientPackage());
	TestFalse(TEXT("GetBoolValue with no bound getter returns false"), Setting->GetBoolValue());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueBool_DefaultAndReset,
                                 "System.GameSettings.ValueBool.DefaultAndReset",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsValueBool_DefaultAndReset::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>(TEXT("false"));
	UGameSettingValueBool* Setting = MakeBool(GetTransientPackage(), TEXT("Bool_Default"), Store);

	TestFalse(TEXT("With no default configured, not resettable"), Setting->IsResettableToDefault());

	Setting->SetDefaultValue(true);
	TestTrue(TEXT("Current (false) differs from default (true): resettable"), Setting->IsResettableToDefault());

	Setting->ResetToDefault();
	TestTrue(TEXT("ResetToDefault applied the default value"), Setting->GetBoolValue());
	TestFalse(TEXT("Now equal to default: no longer resettable"), Setting->IsResettableToDefault());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueBool_StoreAndRestoreInitial,
                                 "System.GameSettings.ValueBool.StoreAndRestoreInitial",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsValueBool_StoreAndRestoreInitial::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>(TEXT("true"));
	UGameSettingValueBool* Setting = MakeBool(GetTransientPackage(), TEXT("Bool_Initial"), Store);

	Setting->StoreInitial();
	Setting->SetBoolValue(false);
	TestFalse(TEXT("Value changed away from initial"), Setting->GetBoolValue());

	Setting->RestoreToInitial();
	TestTrue(TEXT("RestoreToInitial put the stored initial value back"), Setting->GetBoolValue());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueBool_BroadcastsChange,
                                 "System.GameSettings.ValueBool.BroadcastsChange",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsValueBool_BroadcastsChange::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>(TEXT("false"));
	UGameSettingValueBool* Setting = MakeBool(GetTransientPackage(), TEXT("Bool_Evt"), Store);

	int32 ChangeCount = 0;
	EGameSettingChangeReason LastReason = EGameSettingChangeReason::DependencyChanged;
	Setting->OnSettingChangedEvent.AddLambda(
		[&ChangeCount, &LastReason](UGameSetting*, EGameSettingChangeReason Reason)
		{
			++ChangeCount;
			LastReason = Reason;
		});

	Setting->SetBoolValue(true);
	TestEqual(TEXT("SetBoolValue broadcasts exactly once"), ChangeCount, 1);
	TestTrue(TEXT("SetBoolValue broadcasts with reason Change"), LastReason == EGameSettingChangeReason::Change);

	Setting->RestoreToInitial();
	TestTrue(TEXT("RestoreToInitial broadcasts with reason RestoreToInitial"), LastReason == EGameSettingChangeReason::RestoreToInitial);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
