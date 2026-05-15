// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "EditCondition/Specs/GameSettingEditConditionSpec_DependsOnDiscrete.h"

#include "Contributions/GameSettingsContribution_Discrete.h"
#include "Contributions/GameSettingsDiscreteOptionsProvider.h"
#include "EditCondition/WhenCondition.h"
#include "GameSetting.h"
#include "GameSettingFilterState.h"
#include "GameSettingRegistry.h"
#include "GameSettingValueDiscreteDynamic.h"

#if WITH_EDITOR
#include "Engine/AssetManager.h"
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingEditConditionSpec_DependsOnDiscrete)

#define LOCTEXT_NAMESPACE "GameSettings"

void UGameSettingEditConditionSpec_DependsOnDiscrete::GetSettingDependencies(TArray<FPrimaryAssetId>& OutIds) const
{
	if (TargetSetting.IsValid())
	{
		OutIds.Add(TargetSetting);
	}
}

TSharedPtr<FGameSettingEditCondition> UGameSettingEditConditionSpec_DependsOnDiscrete::BuildCondition(
	UGameSettingRegistry& Registry, UGameSetting& Owner) const
{
	TWeakObjectPtr<const ThisClass> WeakSpec(this);
	TWeakObjectPtr<UGameSettingValueDiscreteDynamic> WeakTarget(Cast<UGameSettingValueDiscreteDynamic>(Registry.FindSettingById(TargetSetting)));

	return MakeShared<FWhenCondition>(
		[WeakSpec, WeakTarget](const ULocalPlayer*, FGameSettingEditableState& InOutState)
		{
			const ThisClass* Spec = WeakSpec.Get();
			UGameSettingValueDiscreteDynamic* Target = WeakTarget.Get();
			if (!Spec || !Target)
			{
				return;
			}

			const FString CurrentValue = Target->GetValueAsString();
			const bool bIsInList = Spec->RequiredValues.Contains(CurrentValue);
			const bool bPredicateFails = Spec->bInvertMatch ? bIsInList : !bIsInList;
			if (bPredicateFails)
			{
				Spec->ApplyActionToState(InOutState);
			}
		});
}

FString UGameSettingEditConditionSpec_DependsOnDiscrete::DebugDescribe() const
{
	return FString::Printf(TEXT("DependsOnDiscrete(%s %s [%s])"),
		*TargetSetting.ToString(),
		bInvertMatch ? TEXT("NOT IN") : TEXT("IN"),
		*FString::Join(RequiredValues, TEXT(", ")));
}

#if WITH_EDITOR
EDataValidationResult UGameSettingEditConditionSpec_DependsOnDiscrete::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!TargetSetting.IsValid())
	{
		Context.AddError(LOCTEXT("DependsOnDiscrete_NoTarget",
			"DependsOnDiscrete: TargetSetting is required."));
		return EDataValidationResult::Invalid;
	}

	if (RequiredValues.IsEmpty())
	{
		Context.AddError(LOCTEXT("DependsOnDiscrete_NoValues",
			"DependsOnDiscrete: RequiredValues is empty. Add at least one option value to match against."));
		Result = EDataValidationResult::Invalid;
	}

	if (UPrimaryDataAsset* OwningAsset = Cast<UPrimaryDataAsset>(GetOuter()))
	{
		if (OwningAsset->GetPrimaryAssetId() == TargetSetting)
		{
			Context.AddError(LOCTEXT("DependsOnDiscrete_SelfRef",
				"DependsOnDiscrete: TargetSetting points at the contribution that owns this spec."));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (TargetSetting.PrimaryAssetType.ToString() != TEXT("GameSettingsDiscrete"))
	{
		Context.AddError(FText::Format(
			LOCTEXT("DependsOnDiscrete_BadType", "DependsOnDiscrete: TargetSetting '{0}' must be a GameSettingsDiscrete (got type '{1}')."),
			FText::FromString(TargetSetting.ToString()),
			FText::FromString(TargetSetting.PrimaryAssetType.ToString())));
		Result = EDataValidationResult::Invalid;
	}
	else if (UAssetManager* AssetManager = UAssetManager::GetIfInitialized())
	{
		// Verify each RequiredValues entry exists in the target's static Options
		// array. Skip when the target uses an OptionsProvider (runtime-only).
		if (UGameSettingsContribution_Discrete* TargetAsset =
			Cast<UGameSettingsContribution_Discrete>(AssetManager->GetPrimaryAssetObject(TargetSetting)))
		{
			if (TargetAsset->OptionsProvider == nullptr && !TargetAsset->Options.IsEmpty())
			{
				for (const FString& Required : RequiredValues)
				{
					const bool bMatched = TargetAsset->Options.ContainsByPredicate(
						[&Required](const FGameSettingsDiscreteOption& Opt) { return Opt.Value == Required; });
					if (!bMatched)
					{
						Context.AddError(FText::Format(
							LOCTEXT("DependsOnDiscrete_BadValue",
								"DependsOnDiscrete: RequiredValue '{0}' is not present in target '{1}'s Options."),
							FText::FromString(Required),
							FText::FromString(TargetSetting.ToString())));
						Result = EDataValidationResult::Invalid;
					}
				}
			}
		}
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
