// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingValueDiscreteDynamic.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace UE::GameSettings::Tests
{
	/**
	 * Discrete setting with the canonical Low/Medium/High/Epic options,
	 * optionally initialized against a bare LocalPlayer with an edit condition
	 * disabling some options (the editable-state cache only computes during
	 * Initialize / RefreshEditableState, which require a LocalPlayer).
	 */
	static UGameSettingValueDiscreteDynamic* MakeQualityDiscrete(FName IdName,
		const TSharedRef<FInMemoryDataSource>& Store)
	{
		return MakeDiscrete(GetTransientPackage(), IdName, Store,
			{TEXT("Low"), TEXT("Medium"), TEXT("High"), TEXT("Epic")});
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueDiscrete_UnfilteredIdentity,
                                 "System.GameSettings.ValueDiscrete.UnfilteredIdentity",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsValueDiscrete_UnfilteredIdentity::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>(TEXT("Low"));
	UGameSettingValueDiscreteDynamic* Setting = MakeQualityDiscrete(TEXT("Disc_Identity"), Store);
	Setting->SetDefaultValueFromString(TEXT("Medium"));

	// With nothing disabled, the filtered index space IS the storage index space.
	TestEqual(TEXT("All options exposed"), Setting->GetDiscreteOptions().Num(), 4);
	TestEqual(TEXT("Current value index is the storage index"), Setting->GetDiscreteOptionIndex(), 0);
	TestEqual(TEXT("Default index is the storage index"), Setting->GetDiscreteOptionDefaultIndex(), 1);

	Setting->SetDiscreteOptionByIndex(2);
	TestEqual(TEXT("SetDiscreteOptionByIndex(2) selected the third option"), Setting->GetValueAsString(), FString(TEXT("High")));
	TestEqual(TEXT("Index round-trips"), Setting->GetDiscreteOptionIndex(), 2);

	// An unknown current value falls back to the default index.
	Store->Stored = TEXT("Bogus");
	TestEqual(TEXT("Unknown value falls back to the default index"), Setting->GetDiscreteOptionIndex(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueDiscrete_FilteredIndexContract,
                                 "System.GameSettings.ValueDiscrete.FilteredIndexContract",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsValueDiscrete_FilteredIndexContract::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	ULocalPlayer* LocalPlayer = MakeTestLocalPlayer();
	TestNotNull(TEXT("Test LocalPlayer constructed"), LocalPlayer);
	if (!LocalPlayer)
	{
		return true;
	}

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>(TEXT("Low"));
	UGameSettingValueDiscreteDynamic* Setting = MakeQualityDiscrete(TEXT("Disc_Filtered"), Store);
	Setting->SetDefaultValueFromString(TEXT("High"));

	// Disable "Medium" (storage index 1) before Initialize so the
	// editable-state cache picks the condition up.
	Setting->AddEditCondition(MakeShared<FDisabledOptionsCondition>(TArray<FString>{TEXT("Medium")}));
	Setting->Initialize(LocalPlayer);

	const TArray<FText> Options = Setting->GetDiscreteOptions();
	TestEqual(TEXT("Disabled option is filtered out"), Options.Num(), 3);
	if (Options.Num() == 3)
	{
		TestEqual(TEXT("Filtered option 0 is Low"), Options[0].ToString(), FString(TEXT("Low")));
		TestEqual(TEXT("Filtered option 1 is High"), Options[1].ToString(), FString(TEXT("High")));
		TestEqual(TEXT("Filtered option 2 is Epic"), Options[2].ToString(), FString(TEXT("Epic")));
	}

	// All index APIs speak FILTERED indices.
	TestEqual(TEXT("Current value Low maps to filtered index 0"), Setting->GetDiscreteOptionIndex(), 0);
	TestEqual(TEXT("Default High maps to filtered index 1"), Setting->GetDiscreteOptionDefaultIndex(), 1);

	Setting->SetDiscreteOptionByIndex(1);
	TestEqual(TEXT("Filtered index 1 selects High (storage index 2)"), Setting->GetValueAsString(), FString(TEXT("High")));
	TestEqual(TEXT("Index round-trips in filtered space"), Setting->GetDiscreteOptionIndex(), 1);

	Setting->SetDiscreteOptionByIndex(2);
	TestEqual(TEXT("Filtered index 2 selects Epic (storage index 3)"), Setting->GetValueAsString(), FString(TEXT("Epic")));
	TestEqual(TEXT("Index round-trips in filtered space"), Setting->GetDiscreteOptionIndex(), 2);

	// A current value that is itself disabled falls back to the default index.
	Setting->SetValueFromString(TEXT("Medium"));
	TestEqual(TEXT("Disabled current value falls back to the default's filtered index"), Setting->GetDiscreteOptionIndex(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsValueDiscrete_DisabledDefault,
                                 "System.GameSettings.ValueDiscrete.DisabledDefault",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsValueDiscrete_DisabledDefault::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	ULocalPlayer* LocalPlayer = MakeTestLocalPlayer();
	TestNotNull(TEXT("Test LocalPlayer constructed"), LocalPlayer);
	if (!LocalPlayer)
	{
		return true;
	}

	TSharedRef<FInMemoryDataSource> Store = MakeShared<FInMemoryDataSource>(TEXT("Low"));
	UGameSettingValueDiscreteDynamic* Setting = MakeQualityDiscrete(TEXT("Disc_DisabledDefault"), Store);

	// The default itself is disabled: there is no valid fallback index.
	Setting->SetDefaultValueFromString(TEXT("Medium"));
	Setting->AddEditCondition(MakeShared<FDisabledOptionsCondition>(TArray<FString>{TEXT("Medium")}));
	Setting->Initialize(LocalPlayer);

	TestEqual(TEXT("A disabled default has no filtered index"), Setting->GetDiscreteOptionDefaultIndex(), (int32)INDEX_NONE);

	Setting->SetValueFromString(TEXT("Medium"));
	TestEqual(TEXT("Disabled current value with a disabled default yields INDEX_NONE"), Setting->GetDiscreteOptionIndex(), (int32)INDEX_NONE);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
