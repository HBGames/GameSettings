// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingFilterState.h"
#include "GameSettingValueScalarDynamic.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueScalar_StepSnapping,
                                 "System.GameSettings.ValueScalar.StepSnapping",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsValueScalar_StepSnapping::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>();
	UGameSettingValueScalarDynamic* Setting = MakeScalar(GetTransientPackage(), TEXT("Scalar_Snap"), Store, TRange<double>(0, 100), 10.0);

	Setting->SetValue(23.0);
	TestTrue(TEXT("23 snaps to the nearest step of 10 -> 20"), FMath::IsNearlyEqual(Setting->GetValue(), 20.0, KINDA_SMALL_NUMBER));

	Setting->SetValue(26.0);
	TestTrue(TEXT("26 snaps to 30"), FMath::IsNearlyEqual(Setting->GetValue(), 30.0, KINDA_SMALL_NUMBER));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueScalar_MinMaxClamp,
                                 "System.GameSettings.ValueScalar.MinMaxClamp",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsValueScalar_MinMaxClamp::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>();
	UGameSettingValueScalarDynamic* Setting = MakeScalar(GetTransientPackage(), TEXT("Scalar_Clamp"), Store, TRange<double>(0, 100), 1.0);
	Setting->SetMinimumLimit(10.0);
	Setting->SetMaximumLimit(90.0);

	Setting->SetValue(5.0);
	TestTrue(TEXT("Value below the minimum limit clamps up to 10"), FMath::IsNearlyEqual(Setting->GetValue(), 10.0, KINDA_SMALL_NUMBER));

	Setting->SetValue(200.0);
	TestTrue(TEXT("Value above the maximum limit clamps down to 90"), FMath::IsNearlyEqual(Setting->GetValue(), 90.0, KINDA_SMALL_NUMBER));

	Setting->SetValue(50.0);
	TestTrue(TEXT("Value within limits passes through"), FMath::IsNearlyEqual(Setting->GetValue(), 50.0, KINDA_SMALL_NUMBER));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueScalar_Normalized,
                                 "System.GameSettings.ValueScalar.Normalized",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsValueScalar_Normalized::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>();
	UGameSettingValueScalarDynamic* Setting =
		MakeScalar(GetTransientPackage(), TEXT("Scalar_Norm"), Store, TRange<double>(0, 100), 1.0);

	Setting->SetValueNormalized(0.5);
	TestTrue(TEXT("Normalized 0.5 over [0,100] maps to source 50"),
	         FMath::IsNearlyEqual(Setting->GetValue(), 50.0, KINDA_SMALL_NUMBER));
	TestTrue(TEXT("GetValueNormalized round-trips back to 0.5"),
	         FMath::IsNearlyEqual(Setting->GetValueNormalized(), 0.5, KINDA_SMALL_NUMBER));

	Setting->SetDefaultValue(25.0);
	const TOptional<double> DefaultNorm = Setting->GetDefaultValueNormalized();
	TestTrue(TEXT("Default normalized is set"), DefaultNorm.IsSet());
	TestTrue(TEXT("Default 25 over [0,100] normalizes to 0.25"),
	         DefaultNorm.IsSet() && FMath::IsNearlyEqual(DefaultNorm.GetValue(), 0.25, KINDA_SMALL_NUMBER));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueScalar_ResettableToDefault,
                                 "System.GameSettings.ValueScalar.ResettableToDefault",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsValueScalar_ResettableToDefault::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>();
	UGameSettingValueScalarDynamic* Setting =
		MakeScalar(GetTransientPackage(), TEXT("Scalar_Reset"), Store, TRange<double>(0, 100), 1.0);

	TestFalse(TEXT("No default configured -> not resettable"), Setting->IsResettableToDefault());

	Setting->SetDefaultValue(25.0);
	Setting->SetValue(60.0);
	TestTrue(TEXT("Value differs from default -> resettable"), Setting->IsResettableToDefault());

	Setting->ResetToDefault();
	TestTrue(TEXT("ResetToDefault applied the default"), FMath::IsNearlyEqual(Setting->GetValue(), 25.0, KINDA_SMALL_NUMBER));
	TestFalse(TEXT("At the default value -> no longer resettable"), Setting->IsResettableToDefault());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueScalar_FormatFunctions,
                                 "System.GameSettings.ValueScalar.FormatFunctions",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsValueScalar_FormatFunctions::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("SourceAsInteger formats 12.0 as \"12\""),
	          UGameSettingValueScalarDynamic::SourceAsInteger(12.0, 0.0).ToString(), FString(TEXT("12")));

	TestEqual(TEXT("SourceAsPercent100 formats 50 as \"50%\""),
	          UGameSettingValueScalarDynamic::SourceAsPercent100(50.0, 0.0).ToString(), FString(TEXT("50%")));

	TestEqual(TEXT("ZeroToOnePercent formats normalized 0.5 as \"50%\""),
	          UGameSettingValueScalarDynamic::ZeroToOnePercent(0.0, 0.5).ToString(), FString(TEXT("50%")));

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
