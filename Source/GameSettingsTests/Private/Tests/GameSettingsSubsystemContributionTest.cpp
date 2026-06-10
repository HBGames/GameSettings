// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "GameSettingRegistry.h"
#include "GameSettingValueBool.h"
#include "GameSettingsSubsystem.h"
#include "GameSettingsTestHelpers.h"
#include "GameSettingsTestTypes.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"

namespace UE::GameSettings::Tests
{
	/**
	 * Subsystem constructed directly on a bare ULocalPlayer (never registered
	 * with a GameInstance, Initialize/Deinitialize never run). GetLocalPlayer()
	 * is just an outer cast, so ApplyContribution / GetOrCreateRegistry work;
	 * the module-delegate and asset-discovery paths simply stay unbound.
	 */
	static UGameSettingsSubsystem* MakeBareSubsystem()
	{
		ULocalPlayer* LocalPlayer = MakeTestLocalPlayer();
		return LocalPlayer ? NewObject<UGameSettingsSubsystem>(LocalPlayer) : nullptr;
	}

	static UGameSettingsTest_CollectionContribution* MakeTestContribution(const TCHAR* IdPrefix)
	{
		UGameSettingsTest_CollectionContribution* Contribution =
			NewObject<UGameSettingsTest_CollectionContribution>(GetTransientPackage());
		Contribution->IdPrefix = IdPrefix;
		return Contribution;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsSubsystem_DisabledContributionSkipped,
                                 "System.GameSettings.Subsystem.DisabledContributionSkipped",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsSubsystem_DisabledContributionSkipped::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingsSubsystem* Subsystem = MakeBareSubsystem();
	TestNotNull(TEXT("Bare subsystem constructed"), Subsystem);
	if (!Subsystem)
	{
		return true;
	}

	UGameSettingsTest_CollectionContribution* Contribution = MakeTestContribution(TEXT("SubDisabled"));
	Contribution->bEnabled = false;

	TArray<FGameSettingHandle> Handles;
	Subsystem->ApplyContribution(Contribution, Handles);

	TestEqual(TEXT("Disabled contribution registered nothing"), Handles.Num(), 0);
	TestEqual(TEXT("Disabled contribution's Apply never ran"), Contribution->ApplyCount, 0);
	TestFalse(TEXT("Disabled contribution did not even force a registry into existence"), Subsystem->HasRegistry());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsSubsystem_ContributionDedupWhileLive,
                                 "System.GameSettings.Subsystem.ContributionDedupWhileLive",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsSubsystem_ContributionDedupWhileLive::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingsSubsystem* Subsystem = MakeBareSubsystem();
	TestNotNull(TEXT("Bare subsystem constructed"), Subsystem);
	if (!Subsystem)
	{
		return true;
	}

	UGameSettingsTest_CollectionContribution* Contribution = MakeTestContribution(TEXT("SubDedup"));

	TArray<FGameSettingHandle> FirstHandles;
	Subsystem->ApplyContribution(Contribution, FirstHandles);
	TestEqual(TEXT("First apply registered the contribution's collections"), FirstHandles.Num(), Contribution->NumCollections);
	TestEqual(TEXT("Apply ran once"), Contribution->ApplyCount, 1);
	TestTrue(TEXT("Registry exists after first apply"), Subsystem->HasRegistry());

	UGameSettingRegistry* Registry = Subsystem->GetRegistry();
	for (const FGameSettingHandle& Handle : FirstHandles)
	{
		TestNotNull(TEXT("Each first-apply handle resolves to a live setting"), Registry->FindSettingByHandle(Handle));
	}

	// Second apply of the SAME contribution while its handles are live: no-op.
	TArray<FGameSettingHandle> SecondHandles;
	Subsystem->ApplyContribution(Contribution, SecondHandles);
	TestEqual(TEXT("Second apply produced no handles"), SecondHandles.Num(), 0);
	TestEqual(TEXT("Second apply did not re-run Apply"), Contribution->ApplyCount, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsSubsystem_ContributionReappliesAfterRemoval,
                                 "System.GameSettings.Subsystem.ContributionReappliesAfterRemoval",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsSubsystem_ContributionReappliesAfterRemoval::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingsSubsystem* Subsystem = MakeBareSubsystem();
	TestNotNull(TEXT("Bare subsystem constructed"), Subsystem);
	if (!Subsystem)
	{
		return true;
	}

	UGameSettingsTest_CollectionContribution* Contribution = MakeTestContribution(TEXT("SubReapply"));

	TArray<FGameSettingHandle> FirstHandles;
	Subsystem->ApplyContribution(Contribution, FirstHandles);
	TestEqual(TEXT("First apply ran"), Contribution->ApplyCount, 1);

	// Remove every produced setting by handle, the way a GFP deactivation does.
	UGameSettingRegistry* Registry = Subsystem->GetRegistry();
	for (const FGameSettingHandle& Handle : FirstHandles)
	{
		TestTrue(TEXT("RemoveByHandle succeeds for each applied handle"), Registry->RemoveByHandle(Handle));
	}

	// All previously-applied handles are dead: liveness-aware dedup must let
	// the same contribution object apply again.
	TArray<FGameSettingHandle> SecondHandles;
	Subsystem->ApplyContribution(Contribution, SecondHandles);
	TestEqual(TEXT("Re-apply after removal ran Apply again"), Contribution->ApplyCount, 2);
	TestEqual(TEXT("Re-apply produced fresh handles"), SecondHandles.Num(), Contribution->NumCollections);
	for (const FGameSettingHandle& Handle : SecondHandles)
	{
		TestNotNull(TEXT("Each re-applied handle resolves to a live setting"), Registry->FindSettingByHandle(Handle));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsRegistry_RegenerateBroadcastsOnRegenerated,
                                 "System.GameSettings.Registry.RegenerateBroadcastsOnRegenerated",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsRegistry_RegenerateBroadcastsOnRegenerated::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());

	UGameSettingValueBool* Setting = NewObject<UGameSettingValueBool>(Registry);
	Setting->SetSettingId(MakeTestId(TEXT("Regen_Evt_A")));
	Setting->SetDisplayName(FText::FromString(TEXT("Regen_Evt_A")));
	Registry->AddSetting(Setting);

	int32 RegeneratedCount = 0;
	bool bRegistryEmptyAtRegenerated = false;
	Registry->OnRegeneratedEvent.AddLambda([&](UGameSettingRegistry* InRegistry)
		{
			++RegeneratedCount;
			bRegistryEmptyAtRegenerated = (InRegistry->FindSettingById(MakeTestId(TEXT("Regen_Evt_A"))) == nullptr);
		});

	bool bRegeneratedFiredBeforeStructureChanged = false;
	Registry->OnStructureChangedEvent.AddLambda([&](UGameSettingRegistry*)
		{
			bRegeneratedFiredBeforeStructureChanged = (RegeneratedCount > 0);
		});

	Registry->Regenerate();

	TestEqual(TEXT("OnRegeneratedEvent fired exactly once"), RegeneratedCount, 1);
	TestTrue(TEXT("Old tree was torn down before OnRegenerated (owner can repopulate)"), bRegistryEmptyAtRegenerated);
	TestTrue(TEXT("OnRegenerated fires before the final structure-changed broadcast"), bRegeneratedFiredBeforeStructureChanged);
	TestNull(TEXT("Registry is empty after Regenerate"), Registry->FindSettingById(MakeTestId(TEXT("Regen_Evt_A"))));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsSubsystem_RegenerateClearsContributionTracking,
                                 "System.GameSettings.Subsystem.RegenerateClearsContributionTracking",
                                 UE::GameSettings::Tests::GameSettingsTestFlags)

bool FGameSettingsSubsystem_RegenerateClearsContributionTracking::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingsSubsystem* Subsystem = MakeBareSubsystem();
	TestNotNull(TEXT("Bare subsystem constructed"), Subsystem);
	if (!Subsystem)
	{
		return true;
	}

	UGameSettingsTest_CollectionContribution* Contribution = MakeTestContribution(TEXT("SubRegen"));

	TArray<FGameSettingHandle> FirstHandles;
	Subsystem->ApplyContribution(Contribution, FirstHandles);
	TestEqual(TEXT("First apply ran"), Contribution->ApplyCount, 1);

	// Regenerate destroys the whole tree. The subsystem's OnRegenerated handler
	// drops its stale dedup records (and re-applies module/asset-discovered
	// contributions - none in this test).
	UGameSettingRegistry* Registry = Subsystem->GetRegistry();
	Registry->Regenerate();

	for (const FGameSettingHandle& Handle : FirstHandles)
	{
		TestNull(TEXT("Old handles are dead after Regenerate"), Registry->FindSettingByHandle(Handle));
	}

	// Directly-applied contributions are NOT auto re-applied by the regenerate
	// sweep (only auto-contributors and discovered assets are), but the stale
	// dedup entry must be gone so the owner can push the contribution again.
	TArray<FGameSettingHandle> SecondHandles;
	Subsystem->ApplyContribution(Contribution, SecondHandles);
	TestEqual(TEXT("Re-apply after Regenerate is not blocked by stale dedup state"), Contribution->ApplyCount, 2);
	TestEqual(TEXT("Re-apply produced fresh handles"), SecondHandles.Num(), Contribution->NumCollections);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
