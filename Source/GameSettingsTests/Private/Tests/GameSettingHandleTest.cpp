// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingHandle.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsHandle_DefaultIsInvalid,
                                 "System.GameSettings.Handle.DefaultIsInvalid",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsHandle_DefaultIsInvalid::RunTest(const FString& Parameters)
{
	FGameSettingHandle Handle;
	TestFalse(TEXT("Default-constructed handle is invalid"), Handle.IsValid());

	const FGameSettingHandle Generated = FGameSettingHandle::Generate();
	TestTrue(TEXT("Generated handle is valid"), Generated.IsValid());

	FGameSettingHandle Reset = FGameSettingHandle::Generate();
	Reset.Reset();
	TestFalse(TEXT("Reset handle is invalid again"), Reset.IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsHandle_GenerateIsUnique,
                                 "System.GameSettings.Handle.GenerateIsUnique",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsHandle_GenerateIsUnique::RunTest(const FString& Parameters)
{
	TSet<uint32> Hashes;
	FGameSettingHandle Previous;
	for (int32 i = 0; i < 100; ++i)
	{
		const FGameSettingHandle Handle = FGameSettingHandle::Generate();
		TestTrue(TEXT("Each generated handle is valid"), Handle.IsValid());
		if (i > 0)
		{
			TestTrue(TEXT("Successive handles are not equal"), Handle != Previous);
		}
		Hashes.Add(GetTypeHash(Handle));
		Previous = Handle;
	}

	TestEqual(TEXT("All 100 handle hashes are distinct"), Hashes.Num(), 100);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsHandle_EqualityAndCopy,
                                 "System.GameSettings.Handle.EqualityAndCopy",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsHandle_EqualityAndCopy::RunTest(const FString& Parameters)
{
	const FGameSettingHandle A = FGameSettingHandle::Generate();
	const FGameSettingHandle Copy = A;

	TestTrue(TEXT("A copy compares equal to its source"), A == Copy);
	TestEqual(TEXT("A copy hashes the same as its source"), GetTypeHash(Copy), GetTypeHash(A));

	const FGameSettingHandle B = FGameSettingHandle::Generate();
	TestTrue(TEXT("Distinct handles compare not-equal"), A != B);
	TestFalse(TEXT("ToString on a valid handle is non-empty"), A.ToString().IsEmpty());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
