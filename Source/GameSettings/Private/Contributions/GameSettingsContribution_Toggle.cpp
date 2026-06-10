// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Toggle.h"

#include "GameSettingRegistry.h"
#include "GameSettingValueBool.h"
#include "GameSettingsLog.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContribution_Toggle)

#define LOCTEXT_NAMESPACE "GameSettings"

const FPrimaryAssetType UGameSettingsContribution_Toggle::ContributionPrimaryAssetType = FPrimaryAssetType(TEXT("GameSettingsToggle"));

FPrimaryAssetType UGameSettingsContribution_Toggle::GetContributionPrimaryAssetType() const
{
	return ContributionPrimaryAssetType;
}

void UGameSettingsContribution_Toggle::Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
{
	if (!GetPrimaryAssetId().IsValid() || DisplayName.IsEmpty())
	{
		UE_LOG(LogGameSettings, Error, TEXT("Toggle contribution %s skipped: %s."),
			*GetPathName(),
			!GetPrimaryAssetId().IsValid() ? TEXT("primary asset id is invalid") : TEXT("DisplayName is empty"));
		return;
	}
	if (!Binding.IsValid())
	{
		UE_LOG(LogGameSettings, Error, TEXT("Toggle contribution %s skipped: binding does not resolve (TargetClass=%s, Getter=%s, Setter=%s). Was a bound UFUNCTION renamed?"),
			*GetPathName(),
			*Binding.TargetClass.ToString(),
			*Binding.GetterFunctionName.ToString(),
			*Binding.SetterFunctionName.ToString());
		return;
	}

	UGameSettingValueBool* Setting = NewObject<UGameSettingValueBool>(&Registry);
	ConfigureBaseSetting(*Setting);

	if (TSharedPtr<FGameSettingDataSource> Getter = Binding.CreateGetter())
	{
		Setting->SetDynamicGetter(Getter.ToSharedRef());
	}
	if (TSharedPtr<FGameSettingDataSource> Setter = Binding.CreateSetter())
	{
		Setting->SetDynamicSetter(Setter.ToSharedRef());
	}
	bool EffectiveDefault = bDefaultValue;
	if (bUseClassDefaultValue)
	{
		FString ClassDefaultStr;
		if (Binding.TryGetClassDefaultValueAsString(ClassDefaultStr))
		{
			LexFromString(EffectiveDefault, *ClassDefaultStr);
		}
	}
	Setting->SetDefaultValue(EffectiveDefault);

	const FGameSettingHandle Handle = Registry.AddSetting(Setting, ParentContainer);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
		Registry.ApplyEditConditionSpecs(Setting, EditConditions);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameSettingsContribution_Toggle::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	Result = CombineDataValidationResults(Result,
		Binding.Validate(Context, FString::Printf(TEXT("Toggle '%s'"), *GetPrimaryAssetId().ToString()),
			EGameSettingsBindingValueType::Boolean));

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
