// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "DataSource/GameSettingDataSourceEnhancedInput.h"
#include "GameSettingValueKeyBinding.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"

using namespace UE::GameSettings::Tests;

// Every key binding row persists through the Enhanced Input user settings, so
// they must all report the same, non-empty persist key. That is what lets the
// registry collapse a screen full of rebinds into one save on Apply.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettings_KeyBinding_PersistDataSourceShared,
	"GameSettings.KeyBinding.PersistDataSourceShared", GameSettingsTestFlags)
bool FGameSettings_KeyBinding_PersistDataSourceShared::RunTest(const FString& Parameters)
{
	ULocalPlayer* LocalPlayer = MakeTestLocalPlayer();
	if (!LocalPlayer)
	{
		AddError(TEXT("No GEngine to root a test ULocalPlayer."));
		return false;
	}

	TestFalse(TEXT("Persist key is not empty"), FGameSettingDataSourceEnhancedInput::PersistKey.IsEmpty());

	auto MakeBinding = [LocalPlayer](FName IdName) -> UGameSettingValueKeyBinding*
	{
		UGameSettingValueKeyBinding* Binding = NewObject<UGameSettingValueKeyBinding>(GEngine);
		Binding->SetSettingId(MakeTestId(IdName));
		Binding->SetDisplayName(FText::FromName(IdName));
		Binding->Initialize(LocalPlayer);
		return Binding;
	};

	UGameSettingValueKeyBinding* First = MakeBinding(TEXT("KeyBind_Jump"));
	UGameSettingValueKeyBinding* Second = MakeBinding(TEXT("KeyBind_Fire"));

	const TSharedPtr<FGameSettingDataSource> FirstSource = First->GetPersistableDataSource();
	const TSharedPtr<FGameSettingDataSource> SecondSource = Second->GetPersistableDataSource();

	TestTrue(TEXT("First binding has a persistable data source"), FirstSource.IsValid());
	TestTrue(TEXT("Second binding has a persistable data source"), SecondSource.IsValid());

	if (FirstSource.IsValid() && SecondSource.IsValid())
	{
		TestEqual(TEXT("First persist key matches the shared constant"),
			FirstSource->GetPersistKey(), FGameSettingDataSourceEnhancedInput::PersistKey);
		TestEqual(TEXT("Both bindings share one persist key so the save de-dups"),
			FirstSource->GetPersistKey(), SecondSource->GetPersistKey());
	}

	return true;
}

// With no Enhanced Input subsystem reachable (a bare local player), a binding
// must stay inert: nothing to reset, a rebind reports failure, and the key
// queries return the invalid-key text instead of crashing.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettings_KeyBinding_InertWhenUnbound,
	"GameSettings.KeyBinding.InertWhenUnbound", GameSettingsTestFlags)
bool FGameSettings_KeyBinding_InertWhenUnbound::RunTest(const FString& Parameters)
{
	ULocalPlayer* LocalPlayer = MakeTestLocalPlayer();
	if (!LocalPlayer)
	{
		AddError(TEXT("No GEngine to root a test ULocalPlayer."));
		return false;
	}

	UGameSettingValueKeyBinding* Binding = NewObject<UGameSettingValueKeyBinding>(GEngine);
	Binding->SetSettingId(MakeTestId(TEXT("KeyBind_Inert")));
	Binding->SetDisplayName(FText::FromString(TEXT("Inert")));
	Binding->Initialize(LocalPlayer);

	TestFalse(TEXT("An unbound row is not resettable"), Binding->IsResettableToDefault());
	TestFalse(TEXT("Rebinding fails with no user settings"), Binding->ChangeBinding(0, EKeys::J));

	// Must not crash without a profile.
	Binding->ResetToDefault();
	TestEqual(TEXT("Slot text falls back to the invalid key"),
		Binding->GetKeyTextForSlot(EPlayerMappableKeySlot::First).ToString(),
		EKeys::Invalid.GetDisplayName().ToString());

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
