// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsBindingCustomization.h"

#include "Contributions/GameSettingsBinding.h"
#include "Contributions/GameSettingsBindingValueType.h"
#include "Contributions/GameSettingsContribution_Discrete.h"
#include "Contributions/GameSettingsContribution_Scalar.h"
#include "Contributions/GameSettingsContribution_Toggle.h"
#include "Contributions/GameSettingsTypedContribution.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "GameSettingsEditorFuzzyMatch.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"
#include "UObject/UnrealType.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "GameSettingsEditor"

TSharedRef<IPropertyTypeCustomization> FGameSettingsBindingCustomization::MakeInstance()
{
	return MakeShared<FGameSettingsBindingCustomization>();
}

void FGameSettingsBindingCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> StructHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& Utils)
{
	StructHandlePtr = StructHandle;

	HeaderRow
		.NameContent()
		[
			StructHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(STextBlock)
				.Text(LOCTEXT("BindingValueSummary", "Binding"))
				.Font(IDetailLayoutBuilder::GetDetailFont())
		];
}

void FGameSettingsBindingCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> StructHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& Utils)
{
	TargetClassHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGameSettingsBinding, TargetClass));
	GetterHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGameSettingsBinding, GetterFunctionName));
	SetterHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGameSettingsBinding, SetterFunctionName));
	SaveGameSlotHandle = StructHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGameSettingsBinding, SaveGameSlotName));

	if (TargetClassHandle.IsValid())
	{
		TargetClassHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FGameSettingsBindingCustomization::OnTargetClassChanged));

		ChildBuilder.AddProperty(TargetClassHandle.ToSharedRef());
	}

	BuildFunctionOptions(NAME_None);

	if (GetterHandle.IsValid())
	{
		ChildBuilder.AddCustomRow(LOCTEXT("GetterRow", "Getter"))
			.NameContent()
			[
				GetterHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			.MinDesiredWidth(220.f)
			[
				BuildFunctionPicker(GetterHandle.ToSharedRef(), TEXT("Getter"))
			];
	}

	if (SetterHandle.IsValid())
	{
		ChildBuilder.AddCustomRow(LOCTEXT("SetterRow", "Setter"))
			.NameContent()
			[
				SetterHandle->CreatePropertyNameWidget()
			]
			.ValueContent()
			.MinDesiredWidth(220.f)
			[
				BuildFunctionPicker(SetterHandle.ToSharedRef(), TEXT("Setter"))
			];
	}

	if (SaveGameSlotHandle.IsValid())
	{
		ChildBuilder.AddProperty(SaveGameSlotHandle.ToSharedRef());
	}
}

void FGameSettingsBindingCustomization::OnTargetClassChanged()
{
	BuildFunctionOptions(NAME_None);

	if (GetterComboBox.IsValid())
	{
		GetterComboBox->RefreshOptions();
	}
	if (SetterComboBox.IsValid())
	{
		SetterComboBox->RefreshOptions();
	}
}

UClass* FGameSettingsBindingCustomization::ResolveTargetClass() const
{
	if (!TargetClassHandle.IsValid())
	{
		return nullptr;
	}

	void* RawData = nullptr;
	if (TargetClassHandle->GetValueData(RawData) != FPropertyAccess::Success || RawData == nullptr)
	{
		return nullptr;
	}

	const FSoftClassPath* SoftPath = static_cast<const FSoftClassPath*>(RawData);
	if (!SoftPath || !SoftPath->IsValid())
	{
		return nullptr;
	}
	return SoftPath->TryLoadClass<UObject>();
}

FString FGameSettingsBindingCustomization::ResolveTargetSettingName() const
{
	if (!StructHandlePtr.IsValid())
	{
		return FString();
	}

	// Walk up to the parent contribution and use the asset name (the
	// PrimaryAssetId's Name half) as the fuzzy-match source. Fall back to
	// DisplayName if the asset is unnamed.
	TArray<UObject*> Outers;
	StructHandlePtr->GetOuterObjects(Outers);
	for (UObject* Outer : Outers)
	{
		if (UGameSettingsTypedContribution* Contribution = Cast<UGameSettingsTypedContribution>(Outer))
		{
			const FPrimaryAssetId AssetId = Contribution->GetPrimaryAssetId();
			if (AssetId.IsValid())
			{
				return UE::GameSettingsEditor::FuzzyMatch::NormalizeTargetName(AssetId.PrimaryAssetName.ToString());
			}
			if (!Contribution->DisplayName.IsEmpty())
			{
				return UE::GameSettingsEditor::FuzzyMatch::NormalizeTargetName(Contribution->DisplayName.ToString());
			}
		}
	}
	return FString();
}

bool FGameSettingsBindingCustomization::IsCandidateGetter(const UFunction* Function) const
{
	if (!Function)
	{
		return false;
	}
	if (!Function->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure | FUNC_Native | FUNC_Public))
	{
		// UFUNCTION reflection still applies if not pure/callable; we just need it reflected.
	}
	// Must have a return parameter and no non-return parameters.
	const FProperty* ReturnProp = Function->GetReturnProperty();
	if (!ReturnProp)
	{
		return false;
	}
	int32 NonReturnParams = 0;
	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			continue;
		}
		NonReturnParams++;
	}
	if (NonReturnParams != 0)
	{
		return false;
	}

	return UE::GameSettings::IsPropertyOfValueType(ReturnProp, ResolveExpectedValueType());
}

bool FGameSettingsBindingCustomization::IsCandidateSetter(const UFunction* Function) const
{
	if (!Function)
	{
		return false;
	}
	// One input param, no return value.
	if (Function->GetReturnProperty() != nullptr)
	{
		return false;
	}
	int32 InputParams = 0;
	const FProperty* TheInput = nullptr;
	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		if (It->HasAnyPropertyFlags(CPF_ReturnParm | CPF_OutParm))
		{
			continue;
		}
		InputParams++;
		TheInput = *It;
	}
	if (InputParams != 1)
	{
		return false;
	}

	return UE::GameSettings::IsPropertyOfValueType(TheInput, ResolveExpectedValueType());
}

EGameSettingsBindingValueType FGameSettingsBindingCustomization::ResolveExpectedValueType() const
{
	if (!StructHandlePtr.IsValid())
	{
		return EGameSettingsBindingValueType::Unknown;
	}

	TArray<UObject*> Outers;
	StructHandlePtr->GetOuterObjects(Outers);
	for (const UObject* Outer : Outers)
	{
		if (Outer->IsA<UGameSettingsContribution_Toggle>())
		{
			return EGameSettingsBindingValueType::Boolean;
		}
		if (Outer->IsA<UGameSettingsContribution_Scalar>())
		{
			return EGameSettingsBindingValueType::Numeric;
		}
		if (Outer->IsA<UGameSettingsContribution_Discrete>())
		{
			return EGameSettingsBindingValueType::Discrete;
		}
	}
	return EGameSettingsBindingValueType::Unknown;
}

void FGameSettingsBindingCustomization::BuildFunctionOptions(const FName ContextPropertyName)
{
	GetterOptions.Reset();
	SetterOptions.Reset();

	UClass* Target = ResolveTargetClass();
	if (!Target)
	{
		return;
	}

	const FString SettingName = ResolveTargetSettingName();

	for (TFieldIterator<UFunction> It(Target, EFieldIteratorFlags::IncludeSuper); It; ++It)
	{
		UFunction* Function = *It;
		if (!Function)
		{
			continue;
		}
		// Skip generated UPROPERTY accessors and other engine-internal helpers we don't want to surface.
		const FString FunctionString = Function->GetName();
		if (FunctionString.StartsWith(TEXT("ExecGetterFunction"))
			|| FunctionString.StartsWith(TEXT("ExecSetterFunction"))
			|| FunctionString.Contains(TEXT("__DelegateSignature")))
		{
			continue;
		}

		const int32 Score = UE::GameSettingsEditor::FuzzyMatch::Score(FunctionString, SettingName);

		if (IsCandidateGetter(Function))
		{
			TSharedPtr<FFunctionOption> Option = MakeShared<FFunctionOption>();
			Option->FunctionName = Function->GetFName();
			Option->DisplayLabel = FunctionString;
			Option->Score = Score;
			GetterOptions.Add(Option);
		}
		if (IsCandidateSetter(Function))
		{
			TSharedPtr<FFunctionOption> Option = MakeShared<FFunctionOption>();
			Option->FunctionName = Function->GetFName();
			Option->DisplayLabel = FunctionString;
			Option->Score = Score;
			SetterOptions.Add(Option);
		}
	}

	auto SortByScoreThenName = [](const TSharedPtr<FFunctionOption>& A, const TSharedPtr<FFunctionOption>& B)
		{
			if (A->Score != B->Score)
			{
				return A->Score > B->Score;
			}
			return A->DisplayLabel < B->DisplayLabel;
		};
	GetterOptions.Sort(SortByScoreThenName);
	SetterOptions.Sort(SortByScoreThenName);
}

TSharedRef<SWidget> FGameSettingsBindingCustomization::BuildFunctionPicker(TSharedRef<IPropertyHandle> FunctionNameHandle, const FName ContextPropertyName)
{
	const bool bIsSetter = (ContextPropertyName == TEXT("Setter"));
	TArray<TSharedPtr<FFunctionOption>>* OptionsPtr = bIsSetter ? &SetterOptions : &GetterOptions;

	TSharedRef<SComboBox<TSharedPtr<FFunctionOption>>> Combo =
		SNew(SComboBox<TSharedPtr<FFunctionOption>>)
			.OptionsSource(OptionsPtr)
			.OnGenerateWidget_Lambda([](TSharedPtr<FFunctionOption> Option)
				{
					return SNew(STextBlock)
						.Text(Option.IsValid() ? FText::FromString(Option->DisplayLabel) : LOCTEXT("EmptyOption", "(none)"));
				})
			.OnSelectionChanged_Lambda([FunctionNameHandle](TSharedPtr<FFunctionOption> NewSelection, ESelectInfo::Type SelectInfo)
				{
					if (NewSelection.IsValid() && FunctionNameHandle->IsValidHandle())
					{
						FunctionNameHandle->SetValue(NewSelection->FunctionName);
					}
				})
			[
				SNew(STextBlock)
					.Text_Lambda([FunctionNameHandle]()
						{
							FName CurrentValue;
							if (FunctionNameHandle->IsValidHandle() && FunctionNameHandle->GetValue(CurrentValue) == FPropertyAccess::Success)
							{
								if (CurrentValue.IsNone())
								{
									return LOCTEXT("PickFunction", "(pick a function)");
								}
								return FText::FromName(CurrentValue);
							}
							return LOCTEXT("PickFunction", "(pick a function)");
						})
					.Font(IDetailLayoutBuilder::GetDetailFont())
			];

	if (bIsSetter)
	{
		SetterComboBox = Combo;
	}
	else
	{
		GetterComboBox = Combo;
	}

	// Inline signature preview ("() -> bool", "(float) -> void", etc.) that
	// refreshes whenever the function name changes, so designers can catch
	// type mismatches without saving the asset.
	TSharedRef<SWidget> SignaturePreview = SNew(STextBlock)
		.Text_Lambda([WeakThis = AsShared(), FunctionNameHandle]()
			{
				TSharedPtr<FGameSettingsBindingCustomization> Pinned = StaticCastSharedPtr<FGameSettingsBindingCustomization>(WeakThis.ToSharedPtr());
				return Pinned.IsValid() ? Pinned->FormatSelectedFunctionSignature(FunctionNameHandle) : FText::GetEmpty();
			})
		.ColorAndOpacity(FSlateColor::UseSubduedForeground())
		.Font(IDetailLayoutBuilder::GetDetailFontItalic());

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			Combo
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(0.f, 2.f, 0.f, 0.f)
		[
			SignaturePreview
		];
}

FText FGameSettingsBindingCustomization::FormatSelectedFunctionSignature(TSharedRef<IPropertyHandle> FunctionNameHandle) const
{
	FName CurrentValue;
	if (!FunctionNameHandle->IsValidHandle() || FunctionNameHandle->GetValue(CurrentValue) != FPropertyAccess::Success)
	{
		return FText::GetEmpty();
	}
	if (CurrentValue.IsNone())
	{
		return FText::GetEmpty();
	}

	UClass* Target = ResolveTargetClass();
	if (!Target)
	{
		return FText::GetEmpty();
	}

	UFunction* Function = Target->FindFunctionByName(CurrentValue);
	if (!Function)
	{
		return LOCTEXT("FunctionMissing", "(function not found on TargetClass)");
	}

	TArray<FString> ParamStrings;
	FString ReturnString = TEXT("void");
	for (TFieldIterator<FProperty> It(Function); It; ++It)
	{
		const FProperty* Prop = *It;
		if (Prop->HasAnyPropertyFlags(CPF_ReturnParm))
		{
			ReturnString = Prop->GetCPPType();
		}
		else if (Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ConstParm))
		{
			ParamStrings.Add(FString::Printf(TEXT("out %s"), *Prop->GetCPPType()));
		}
		else
		{
			ParamStrings.Add(Prop->GetCPPType());
		}
	}

	const FString ParamList = FString::Join(ParamStrings, TEXT(", "));
	return FText::FromString(FString::Printf(TEXT("(%s) -> %s"), *ParamList, *ReturnString));
}

#undef LOCTEXT_NAMESPACE
