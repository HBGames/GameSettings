// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "EditCondition/Specs/GameSettingEditConditionSpec_DependsOnScalar.h"

#include "EditCondition/WhenCondition.h"
#include "GameSetting.h"
#include "GameSettingFilterState.h"
#include "GameSettingRegistry.h"
#include "GameSettingValueScalar.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingEditConditionSpec_DependsOnScalar)

#define LOCTEXT_NAMESPACE "GameSettings"

namespace
{
	bool EvaluateCompare(double Lhs, EGameSettingScalarCompare Op, double Rhs, double Epsilon)
	{
		switch (Op)
		{
		case EGameSettingScalarCompare::LessThan:       return Lhs <  Rhs;
		case EGameSettingScalarCompare::LessOrEqual:    return Lhs <= Rhs;
		case EGameSettingScalarCompare::Equal:          return FMath::Abs(Lhs - Rhs) <= Epsilon;
		case EGameSettingScalarCompare::GreaterOrEqual: return Lhs >= Rhs;
		case EGameSettingScalarCompare::GreaterThan:    return Lhs >  Rhs;
		case EGameSettingScalarCompare::NotEqual:       return FMath::Abs(Lhs - Rhs) >  Epsilon;
		default:                                        return true;
		}
	}
}

void UGameSettingEditConditionSpec_DependsOnScalar::GetSettingDependencies(TArray<FPrimaryAssetId>& OutIds) const
{
	if (TargetSetting.IsValid())
	{
		OutIds.Add(TargetSetting);
	}
}

TSharedPtr<FGameSettingEditCondition> UGameSettingEditConditionSpec_DependsOnScalar::BuildCondition(
	UGameSettingRegistry& Registry, UGameSetting& Owner) const
{
	TWeakObjectPtr<const ThisClass> WeakSpec(this);
	TWeakObjectPtr<UGameSettingValueScalar> WeakTarget(Cast<UGameSettingValueScalar>(Registry.FindSettingById(TargetSetting)));

	return MakeShared<FWhenCondition>(
		[WeakSpec, WeakTarget](const ULocalPlayer*, FGameSettingEditableState& InOutState)
		{
			const ThisClass* Spec = WeakSpec.Get();
			UGameSettingValueScalar* Target = WeakTarget.Get();
			if (!Spec || !Target)
			{
				return;
			}
			const double CurrentValue = Target->GetValue();
			const bool bSatisfied = EvaluateCompare(CurrentValue, Spec->Op, Spec->Threshold, Spec->Epsilon);
			if (!bSatisfied)
			{
				Spec->ApplyActionToState(InOutState);
			}
		});
}

FString UGameSettingEditConditionSpec_DependsOnScalar::DebugDescribe() const
{
	const TCHAR* OpStr = TEXT("?");
	switch (Op)
	{
	case EGameSettingScalarCompare::LessThan:       OpStr = TEXT("<");  break;
	case EGameSettingScalarCompare::LessOrEqual:    OpStr = TEXT("<="); break;
	case EGameSettingScalarCompare::Equal:          OpStr = TEXT("==~"); break;
	case EGameSettingScalarCompare::GreaterOrEqual: OpStr = TEXT(">="); break;
	case EGameSettingScalarCompare::GreaterThan:    OpStr = TEXT(">");  break;
	case EGameSettingScalarCompare::NotEqual:       OpStr = TEXT("!=~"); break;
	}
	return FString::Printf(TEXT("DependsOnScalar(%s %s %.4f)"),
		*TargetSetting.ToString(), OpStr, Threshold);
}

#if WITH_EDITOR
EDataValidationResult UGameSettingEditConditionSpec_DependsOnScalar::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!TargetSetting.IsValid())
	{
		Context.AddError(LOCTEXT("DependsOnScalar_NoTarget",
			"DependsOnScalar: TargetSetting is required."));
		Result = EDataValidationResult::Invalid;
	}
	else if (TargetSetting.PrimaryAssetType.ToString() != TEXT("GameSettingsScalar"))
	{
		Context.AddError(FText::Format(
			LOCTEXT("DependsOnScalar_BadType", "DependsOnScalar: TargetSetting '{0}' must be a GameSettingsScalar (got type '{1}')."),
			FText::FromString(TargetSetting.ToString()),
			FText::FromString(TargetSetting.PrimaryAssetType.ToString())));
		Result = EDataValidationResult::Invalid;
	}

	if (UPrimaryDataAsset* OwningAsset = Cast<UPrimaryDataAsset>(GetOuter()))
	{
		if (OwningAsset->GetPrimaryAssetId() == TargetSetting)
		{
			Context.AddError(LOCTEXT("DependsOnScalar_SelfRef",
				"DependsOnScalar: TargetSetting points at the contribution that owns this spec."));
			Result = EDataValidationResult::Invalid;
		}
	}

	if ((Op == EGameSettingScalarCompare::Equal || Op == EGameSettingScalarCompare::NotEqual) && Epsilon < 0.0)
	{
		Context.AddError(LOCTEXT("DependsOnScalar_BadEpsilon",
			"DependsOnScalar: Epsilon must be non-negative for Equal / NotEqual comparisons."));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
