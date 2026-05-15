// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "EditCondition/Specs/GameSettingEditConditionSpec_DisableDiscreteOption.h"

#include "Contributions/GameSettingsContribution_Discrete.h"
#include "EditCondition/WhenCondition.h"
#include "GameSetting.h"
#include "GameSettingFilterState.h"
#include "GameSettingValueDiscreteDynamic.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingEditConditionSpec_DisableDiscreteOption)

#define LOCTEXT_NAMESPACE "GameSettings"

void UGameSettingEditConditionSpec_DisableDiscreteOption::GetSettingDependencies(TArray<FPrimaryAssetId>& OutIds) const
{
	if (Predicate)
	{
		Predicate->GetSettingDependencies(OutIds);
	}
}

TSharedPtr<FGameSettingEditCondition> UGameSettingEditConditionSpec_DisableDiscreteOption::BuildCondition(
	UGameSettingRegistry& Registry, UGameSetting& Owner) const
{
	if (!Predicate || OptionValuesToDisable.IsEmpty())
	{
		return nullptr;
	}

	// Build the inner condition once and cache it inside the lambda capture
	// (kept alive by the SharedPtr inside the FWhenCondition itself).
	TSharedPtr<FGameSettingEditCondition> InnerCondition = Predicate->BuildCondition(Registry, Owner);
	if (!InnerCondition.IsValid())
	{
		return nullptr;
	}

	TWeakObjectPtr<const ThisClass> WeakSpec(this);

	return MakeShared<FWhenCondition>(
		[WeakSpec, InnerCondition](const ULocalPlayer* LocalPlayer, FGameSettingEditableState& InOutState)
		{
			const ThisClass* Spec = WeakSpec.Get();
			if (!Spec || !InnerCondition.IsValid())
			{
				return;
			}

			// Probe the inner condition on a throwaway state to detect "would
			// have disabled / hidden / killed". We deliberately don't mirror
			// the inner action onto the outer state - this spec is purely
			// about per-option disabling.
			FGameSettingEditableState Probe;
			InnerCondition->GatherEditState(LocalPlayer, Probe);

			const bool bInnerFired = !Probe.IsEnabled() || !Probe.IsVisible();
			if (!bInnerFired)
			{
				return;
			}
			for (const FString& Value : Spec->OptionValuesToDisable)
			{
				InOutState.DisableOption(Value);
			}
		});
}

FString UGameSettingEditConditionSpec_DisableDiscreteOption::DebugDescribe() const
{
	return FString::Printf(TEXT("DisableDiscreteOption([%s] when %s)"),
		*FString::Join(OptionValuesToDisable, TEXT(", ")),
		Predicate ? *Predicate->DebugDescribe() : TEXT("<null>"));
}

#if WITH_EDITOR
EDataValidationResult UGameSettingEditConditionSpec_DisableDiscreteOption::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (OptionValuesToDisable.IsEmpty())
	{
		Context.AddError(LOCTEXT("DisableOption_NoValues",
			"DisableDiscreteOption: OptionValuesToDisable is empty. Add at least one option value."));
		Result = EDataValidationResult::Invalid;
	}

	if (!Predicate)
	{
		Context.AddError(LOCTEXT("DisableOption_NoPredicate",
			"DisableDiscreteOption: Predicate is required."));
		Result = EDataValidationResult::Invalid;
	}
	else if (Predicate->IsA(StaticClass()))
	{
		Context.AddError(LOCTEXT("DisableOption_NestedSelf",
			"DisableDiscreteOption: Predicate cannot itself be a DisableDiscreteOption (no nesting)."));
		Result = EDataValidationResult::Invalid;
	}
	else
	{
		Result = CombineDataValidationResults(Result, Predicate->IsDataValid(Context));
	}

	// The owning contribution must be a Discrete.
	if (UPrimaryDataAsset* OwningAsset = Cast<UPrimaryDataAsset>(GetOuter()))
	{
		if (!OwningAsset->IsA<UGameSettingsContribution_Discrete>())
		{
			Context.AddError(LOCTEXT("DisableOption_NotDiscrete",
				"DisableDiscreteOption: this spec is only valid on a Discrete contribution."));
			Result = EDataValidationResult::Invalid;
		}
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
