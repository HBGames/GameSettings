// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "EditCondition/Specs/GameSettingEditConditionSpec_DependsOnToggle.h"

#include "EditCondition/WhenCondition.h"
#include "GameSetting.h"
#include "GameSettingFilterState.h"
#include "GameSettingRegistry.h"
#include "GameSettingValueBool.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingEditConditionSpec_DependsOnToggle)

#define LOCTEXT_NAMESPACE "GameSettings"

void UGameSettingEditConditionSpec_DependsOnToggle::GetSettingDependencies(TArray<FPrimaryAssetId>& OutIds) const
{
	if (TargetSetting.IsValid())
	{
		OutIds.Add(TargetSetting);
	}
}

TSharedPtr<FGameSettingEditCondition> UGameSettingEditConditionSpec_DependsOnToggle::BuildCondition(
	UGameSettingRegistry& Registry, UGameSetting& Owner) const
{
	TWeakObjectPtr<const ThisClass> WeakSpec(this);
	TWeakObjectPtr<UGameSettingValueBool> WeakTarget(Cast<UGameSettingValueBool>(Registry.FindSettingById(TargetSetting)));

	return MakeShared<FWhenCondition>(
		[WeakSpec, WeakTarget](const ULocalPlayer*, FGameSettingEditableState& InOutState)
		{
			const ThisClass* Spec = WeakSpec.Get();
			UGameSettingValueBool* Target = WeakTarget.Get();
			if (!Spec || !Target)
			{
				return;
			}
			if (Target->GetBoolValue() != Spec->bRequiredValue)
			{
				Spec->ApplyActionToState(InOutState);
			}
		});
}

FString UGameSettingEditConditionSpec_DependsOnToggle::DebugDescribe() const
{
	return FString::Printf(TEXT("DependsOnToggle(%s == %s)"),
		*TargetSetting.ToString(),
		bRequiredValue ? TEXT("true") : TEXT("false"));
}

#if WITH_EDITOR
EDataValidationResult UGameSettingEditConditionSpec_DependsOnToggle::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!TargetSetting.IsValid())
	{
		Context.AddError(LOCTEXT("DependsOnToggle_NoTarget",
			"DependsOnToggle: TargetSetting is required."));
		return EDataValidationResult::Invalid;
	}

	if (UPrimaryDataAsset* OwningAsset = Cast<UPrimaryDataAsset>(GetOuter()))
	{
		if (OwningAsset->GetPrimaryAssetId() == TargetSetting)
		{
			Context.AddError(LOCTEXT("DependsOnToggle_SelfRef",
				"DependsOnToggle: TargetSetting points at the contribution that owns this spec."));
			Result = EDataValidationResult::Invalid;
		}
	}

	if (TargetSetting.PrimaryAssetType.ToString() != TEXT("GameSettingsToggle"))
	{
		Context.AddError(FText::Format(
			LOCTEXT("DependsOnToggle_BadType", "DependsOnToggle: TargetSetting '{0}' must be a GameSettingsToggle (got type '{1}')."),
			FText::FromString(TargetSetting.ToString()),
			FText::FromString(TargetSetting.PrimaryAssetType.ToString())));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
