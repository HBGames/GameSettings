// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;
class IPropertyHandle;
class IPropertyHandleArray;

/**
 * Detail customization for UGameSettingEditConditionSpec_DependsOnDiscrete.
 *
 * Replaces the raw TArray<FString> RequiredValues field with a multi-select
 * checkbox list sourced from the resolved target Discrete contribution's
 * Options array. Falls back to the default array editor when the target is
 * unset, unresolved, or backed by a runtime OptionsProvider.
 */
class FGameSettingEditConditionDiscreteCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

private:
	struct FOptionEntry
	{
		FString Value;
		FText DisplayText;
	};

	bool ResolveTargetOptions(TSharedPtr<IPropertyHandle> TargetSettingHandle, TArray<FOptionEntry>& OutOptions) const;

	void BuildOptionRows(IDetailLayoutBuilder& DetailBuilder,
		TSharedPtr<IPropertyHandle> RequiredValuesHandle,
		const TArray<FOptionEntry>& Options);

	bool IsOptionChecked(TSharedPtr<IPropertyHandle> RequiredValuesHandle, const FString& OptionValue) const;
	void SetOptionChecked(TSharedPtr<IPropertyHandle> RequiredValuesHandle, const FString& OptionValue, bool bChecked);
};
