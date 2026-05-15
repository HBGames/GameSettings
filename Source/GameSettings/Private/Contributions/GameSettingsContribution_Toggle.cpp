// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Toggle.h"

#include "GameSettingRegistry.h"
#include "GameSettingValueBool.h"

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
	if (!GetPrimaryAssetId().IsValid() || DisplayName.IsEmpty() || !Binding.IsValid())
	{
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
	Setting->SetDefaultValue(bDefaultValue);

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
