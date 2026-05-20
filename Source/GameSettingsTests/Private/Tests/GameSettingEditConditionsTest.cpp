// Copyright Hitbox Games, LLC. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "EditCondition/GameSettingEditConditionSpec.h"
#include "EditCondition/Specs/GameSettingEditConditionSpec_DependsOnToggle.h"
#include "EditCondition/Specs/GameSettingEditConditionSpec_PrimaryPlayerOnly.h"
#include "GameSettingFilterState.h"
#include "GameSettingRegistry.h"
#include "GameSettingValueBool.h"
#include "GameSettingsTestHelpers.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "UObject/TextProperty.h"

namespace UE::GameSettings::Tests
{
	static UGameSettingValueBool* CreateToggle(UGameSettingRegistry* Registry, FName IdName)
	{
		UGameSettingValueBool* Setting = NewObject<UGameSettingValueBool>(Registry);
		Setting->SetSettingId(MakeTestId(IdName, TEXT("GameSettingsToggle")));
		Setting->SetDisplayName(FText::FromName(IdName));
		return Setting;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsEditConditions_EagerInstall,
                                 "System.GameSettings.EditConditions.EagerInstall",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsEditConditions_EagerInstall::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	UGameSettingValueBool* Target = CreateToggle(Registry, TEXT("TargetToggle_Eager"));
	UGameSettingValueBool* Owner = CreateToggle(Registry, TEXT("OwnerToggle_Eager"));

	// Target lands first - the dependency is satisfied eagerly.
	Registry->AddSetting(Target);
	Registry->AddSetting(Owner);

	UGameSettingEditConditionSpec_DependsOnToggle* Spec = NewObject<UGameSettingEditConditionSpec_DependsOnToggle>(Registry);
	Spec->TargetSetting = Target->GetSettingId();
	Spec->bRequiredValue = true;

	TArray<TObjectPtr<UGameSettingEditConditionSpec>> Specs = {Spec};
	Registry->ApplyEditConditionSpecs(Owner, Specs);

	TestEqual(TEXT("Eagerly-installed: owner has one edit condition"), Owner->GetEditConditions().Num(), 1);
	TestEqual(TEXT("Eagerly-installed: deferred queue is empty"), Registry->GetNumDeferredEditConditions(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsEditConditions_DeferredResolve,
                                 "System.GameSettings.EditConditions.DeferredResolve",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsEditConditions_DeferredResolve::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	UGameSettingValueBool* Owner = CreateToggle(Registry, TEXT("OwnerToggle_Deferred"));
	UGameSettingValueBool* Target = CreateToggle(Registry, TEXT("TargetToggle_Deferred"));

	// Owner lands first; spec references target that doesn't exist yet.
	Registry->AddSetting(Owner);

	UGameSettingEditConditionSpec_DependsOnToggle* Spec = NewObject<UGameSettingEditConditionSpec_DependsOnToggle>(Registry);
	Spec->TargetSetting = Target->GetSettingId();
	Spec->bRequiredValue = true;

	TArray<TObjectPtr<UGameSettingEditConditionSpec>> Specs = {Spec};
	Registry->ApplyEditConditionSpecs(Owner, Specs);

	TestEqual(TEXT("Pre-arrival: owner has no installed conditions"), Owner->GetEditConditions().Num(), 0);
	TestEqual(TEXT("Pre-arrival: one spec is queued"), Registry->GetNumDeferredEditConditions(), 1);

	// Target arrival should flush the deferred queue.
	Registry->AddSetting(Target);

	TestEqual(TEXT("Post-arrival: owner has one installed condition"), Owner->GetEditConditions().Num(), 1);
	TestEqual(TEXT("Post-arrival: deferred queue is empty"), Registry->GetNumDeferredEditConditions(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsEditConditions_IdempotentReapply,
                                 "System.GameSettings.EditConditions.IdempotentReapply",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsEditConditions_IdempotentReapply::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	UGameSettingValueBool* Target = CreateToggle(Registry, TEXT("TargetToggle_Idem"));
	UGameSettingValueBool* Owner = CreateToggle(Registry, TEXT("OwnerToggle_Idem"));
	Registry->AddSetting(Target);
	Registry->AddSetting(Owner);

	UGameSettingEditConditionSpec_DependsOnToggle* Spec = NewObject<UGameSettingEditConditionSpec_DependsOnToggle>(Registry);
	Spec->TargetSetting = Target->GetSettingId();
	Spec->bRequiredValue = true;

	TArray<TObjectPtr<UGameSettingEditConditionSpec>> Specs = {Spec};
	Registry->ApplyEditConditionSpecs(Owner, Specs);
	Registry->ApplyEditConditionSpecs(Owner, Specs);
	Registry->ApplyEditConditionSpecs(Owner, Specs);

	TestEqual(TEXT("Idempotent: only one condition installed despite three apply calls"), Owner->GetEditConditions().Num(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsEditConditions_TargetRemovalCleanup,
                                 "System.GameSettings.EditConditions.TargetRemovalCleanup",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsEditConditions_TargetRemovalCleanup::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	UGameSettingValueBool* Target = CreateToggle(Registry, TEXT("TargetToggle_Remove"));
	UGameSettingValueBool* Owner = CreateToggle(Registry, TEXT("OwnerToggle_Remove"));
	Registry->AddSetting(Target);
	Registry->AddSetting(Owner);

	UGameSettingEditConditionSpec_DependsOnToggle* Spec = NewObject<UGameSettingEditConditionSpec_DependsOnToggle>(Registry);
	Spec->TargetSetting = Target->GetSettingId();
	Spec->bRequiredValue = true;

	TArray<TObjectPtr<UGameSettingEditConditionSpec>> Specs = {Spec};
	Registry->ApplyEditConditionSpecs(Owner, Specs);
	TestEqual(TEXT("Pre-remove: owner has one installed condition"), Owner->GetEditConditions().Num(), 1);

	Registry->RemoveById(Target->GetSettingId());

	TestEqual(TEXT("Post-remove: owner's condition was cleaned up"), Owner->GetEditConditions().Num(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameSettingsEditConditions_SpecActionMapping,
                                 "System.GameSettings.EditConditions.SpecActionMapping",
                                 EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FGameSettingsEditConditions_SpecActionMapping::RunTest(const FString& Parameters)
{
	using namespace UE::GameSettings::Tests;

	UGameSettingRegistry* Registry = NewObject<UGameSettingRegistry>(GetTransientPackage());
	UGameSettingValueBool* Target = CreateToggle(Registry, TEXT("TargetToggle_Action"));
	UGameSettingValueBool* Owner = CreateToggle(Registry, TEXT("OwnerToggle_Action"));
	Registry->AddSetting(Target);
	Registry->AddSetting(Owner);

	// Default action is Disable. Target value defaults to false; predicate
	// requires true, so the predicate fails and the lambda calls Disable.
	UGameSettingEditConditionSpec_DependsOnToggle* Spec = NewObject<UGameSettingEditConditionSpec_DependsOnToggle>(Registry);
	Spec->TargetSetting = Target->GetSettingId();
	Spec->bRequiredValue = true;

	// DisableReason is a protected EditAnywhere property normally authored on
	// the contribution asset; set it via reflection so Disable() gets the
	// player-facing reason it (correctly) requires, instead of tripping the
	// empty-reason ensure.
	FTextProperty* ReasonProp = FindFProperty<FTextProperty>(UGameSettingEditConditionSpec::StaticClass(), TEXT("DisableReason"));
	TestNotNull(TEXT("DisableReason property is reflectable"), ReasonProp);
	if (ReasonProp)
	{
		ReasonProp->SetPropertyValue_InContainer(Spec, NSLOCTEXT("GameSettingsTests", "TestDisableReason", "Disabled by test"));
	}

	TSharedPtr<FGameSettingEditCondition> Condition = Spec->BuildCondition(*Registry, *Owner);
	TestNotNull(TEXT("BuildCondition returns a non-null shared ref"), Condition.Get());

	FGameSettingEditableState State;
	Condition->GatherEditState(nullptr, State);

	TestFalse(TEXT("Predicate fails - state is disabled"), State.IsEnabled());
	TestEqual(TEXT("A disable reason was recorded"), State.GetDisabledReasons().Num(), 1);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
