// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingEditConditionDiscreteCustomization.h"

#include "Contributions/GameSettingsContribution_Discrete.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EditCondition/Specs/GameSettingEditConditionSpec_DependsOnDiscrete.h"
#include "Engine/AssetManager.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailGroup.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GameSettings"

TSharedRef<IDetailCustomization> FGameSettingEditConditionDiscreteCustomization::MakeInstance()
{
	return MakeShared<FGameSettingEditConditionDiscreteCustomization>();
}

void FGameSettingEditConditionDiscreteCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TSharedRef<IPropertyHandle> TargetSettingHandle =
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UGameSettingEditConditionSpec_DependsOnDiscrete, TargetSetting));
	TSharedRef<IPropertyHandle> RequiredValuesHandle =
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UGameSettingEditConditionSpec_DependsOnDiscrete, RequiredValues));

	// Refresh when TargetSetting changes so the option list updates in place.
	// Registered before the unresolved-options early-out below so that
	// assigning a valid TargetSetting upgrades the fallback state live.
	TargetSettingHandle->SetOnPropertyValueChanged(
		FSimpleDelegate::CreateLambda([&DetailBuilder]() { DetailBuilder.ForceRefreshDetails(); }));

	TArray<FOptionEntry> ResolvedOptions;
	const bool bHasOptions = ResolveTargetOptions(TargetSettingHandle, ResolvedOptions);

	if (!bHasOptions)
	{
		// Leave the default TArray<FString> editor in place; the user is in a
		// state where we cannot resolve options (no target, target is
		// provider-driven, or target asset not loaded).
		return;
	}

	// Hide the default row for RequiredValues so we can replace it with a
	// purpose-built checkbox list.
	DetailBuilder.HideProperty(RequiredValuesHandle);

	BuildOptionRows(DetailBuilder, RequiredValuesHandle, ResolvedOptions);
}

bool FGameSettingEditConditionDiscreteCustomization::ResolveTargetOptions(
	TSharedPtr<IPropertyHandle> TargetSettingHandle, TArray<FOptionEntry>& OutOptions) const
{
	if (!TargetSettingHandle.IsValid())
	{
		return false;
	}

	void* RawValuePtr = nullptr;
	if (TargetSettingHandle->GetValueData(RawValuePtr) != FPropertyAccess::Success || RawValuePtr == nullptr)
	{
		return false;
	}

	const FPrimaryAssetId TargetId = *static_cast<FPrimaryAssetId*>(RawValuePtr);
	if (!TargetId.IsValid())
	{
		return false;
	}

	UAssetManager* AssetManager = UAssetManager::GetIfInitialized();
	if (!AssetManager)
	{
		return false;
	}

	UGameSettingsContribution_Discrete* TargetAsset =
		Cast<UGameSettingsContribution_Discrete>(AssetManager->GetPrimaryAssetObject(TargetId));
	if (!TargetAsset)
	{
		// Try to load on demand. GetPrimaryAssetObject returns null when the
		// asset is unloaded; a synchronous TryLoad on its path forces it in
		// for the editor.
		if (FSoftObjectPath SoftPath = AssetManager->GetPrimaryAssetPath(TargetId); SoftPath.IsValid())
		{
			TargetAsset = Cast<UGameSettingsContribution_Discrete>(SoftPath.TryLoad());
		}
		if (!TargetAsset)
		{
			return false;
		}
	}

	if (TargetAsset->OptionsProvider != nullptr || TargetAsset->Options.IsEmpty())
	{
		// Provider-driven options or no static options - can't enumerate at edit time.
		return false;
	}

	OutOptions.Reserve(TargetAsset->Options.Num());
	for (const FGameSettingsDiscreteOption& Opt : TargetAsset->Options)
	{
		OutOptions.Add({ Opt.Value, Opt.DisplayText });
	}
	return true;
}

void FGameSettingEditConditionDiscreteCustomization::BuildOptionRows(
	IDetailLayoutBuilder& DetailBuilder,
	TSharedPtr<IPropertyHandle> RequiredValuesHandle,
	const TArray<FOptionEntry>& Options)
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Dependency");

	TSharedRef<SVerticalBox> VBox = SNew(SVerticalBox);
	for (const FOptionEntry& Opt : Options)
	{
		const FString OptionValue = Opt.Value;
		const FText OptionText = Opt.DisplayText.IsEmpty() ? FText::FromString(Opt.Value) : Opt.DisplayText;

		VBox->AddSlot()
			.AutoHeight()
			.Padding(0.0f, 1.0f)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this, RequiredValuesHandle, OptionValue]()
				{
					return IsOptionChecked(RequiredValuesHandle, OptionValue)
						? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
				})
				.OnCheckStateChanged_Lambda([this, RequiredValuesHandle, OptionValue](ECheckBoxState NewState)
				{
					SetOptionChecked(RequiredValuesHandle, OptionValue, NewState == ECheckBoxState::Checked);
				})
				[
					SNew(STextBlock)
					.Text(FText::Format(LOCTEXT("OptionRowFmt", "{0}  ({1})"),
						OptionText, FText::FromString(OptionValue)))
					.ToolTipText(FText::Format(LOCTEXT("OptionRowTip",
						"Option Value: {0}\nDisplay Text: {1}"),
						FText::FromString(OptionValue), OptionText))
				]
			];
	}

	Category.AddCustomRow(LOCTEXT("RequiredValuesFilter", "Required Values"))
		.NameContent()
		[
			SNew(STextBlock)
			.Text(LOCTEXT("RequiredValues", "Required Values"))
			.ToolTipText(LOCTEXT("RequiredValuesTip",
				"Option values that satisfy the predicate. The setting is gated when the target's value is not in this list (or, when Invert Match is true, when it IS in this list)."))
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.MaxDesiredWidth(400.0f)
		[
			SNew(SScrollBox)
			+ SScrollBox::Slot()
			[
				VBox
			]
		];
}

bool FGameSettingEditConditionDiscreteCustomization::IsOptionChecked(
	TSharedPtr<IPropertyHandle> RequiredValuesHandle, const FString& OptionValue) const
{
	TSharedPtr<IPropertyHandleArray> ArrayHandle = RequiredValuesHandle->AsArray();
	if (!ArrayHandle.IsValid())
	{
		return false;
	}
	uint32 NumElements = 0;
	ArrayHandle->GetNumElements(NumElements);
	for (uint32 Index = 0; Index < NumElements; ++Index)
	{
		FString Element;
		if (ArrayHandle->GetElement(Index)->GetValue(Element) == FPropertyAccess::Success && Element == OptionValue)
		{
			return true;
		}
	}
	return false;
}

void FGameSettingEditConditionDiscreteCustomization::SetOptionChecked(
	TSharedPtr<IPropertyHandle> RequiredValuesHandle, const FString& OptionValue, bool bChecked)
{
	TSharedPtr<IPropertyHandleArray> ArrayHandle = RequiredValuesHandle->AsArray();
	if (!ArrayHandle.IsValid())
	{
		return;
	}

	if (bChecked)
	{
		if (IsOptionChecked(RequiredValuesHandle, OptionValue))
		{
			return;
		}
		uint32 NumElementsBefore = 0;
		ArrayHandle->GetNumElements(NumElementsBefore);
		ArrayHandle->AddItem();
		TSharedPtr<IPropertyHandle> NewElement = ArrayHandle->GetElement(static_cast<int32>(NumElementsBefore));
		if (NewElement.IsValid())
		{
			NewElement->SetValue(OptionValue);
		}
	}
	else
	{
		uint32 NumElements = 0;
		ArrayHandle->GetNumElements(NumElements);
		for (int32 Index = static_cast<int32>(NumElements) - 1; Index >= 0; --Index)
		{
			FString Element;
			if (ArrayHandle->GetElement(Index)->GetValue(Element) == FPropertyAccess::Success && Element == OptionValue)
			{
				ArrayHandle->DeleteItem(Index);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
