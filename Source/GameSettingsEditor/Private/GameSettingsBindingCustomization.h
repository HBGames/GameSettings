// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsBindingValueType.h"
#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "Widgets/Input/SComboBox.h"

class FDetailWidgetRow;
class FProperty;
class IDetailChildrenBuilder;
class IPropertyHandle;
class UClass;
class UFunction;

/**
 * Detail customization for FGameSettingsBinding.
 *
 * Replaces the bare FName fields for the getter and setter with combo
 * boxes that introspect the bound TargetClass and rank UFUNCTIONs by
 * name-similarity to the parent contribution's SettingId / DisplayName.
 */
class FGameSettingsBindingCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	//~IPropertyTypeCustomization
	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& Utils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& Utils) override;
	//~End IPropertyTypeCustomization

private:
	struct FFunctionOption
	{
		FName FunctionName;
		FString DisplayLabel;
		int32 Score = 0;
	};

	void BuildFunctionOptions();

	TSharedRef<class SWidget> BuildFunctionPicker(TSharedRef<IPropertyHandle> FunctionNameHandle, const FName ContextPropertyName);

	void OnTargetClassChanged();

	UClass* ResolveTargetClass() const;
	FString ResolveTargetSettingName() const;

	bool IsCandidateGetter(const UFunction* Function) const;
	bool IsCandidateSetter(const UFunction* Function) const;

	/** Walk Outers to find which typed contribution wraps this binding, returning its expected value shape. */
	EGameSettingsBindingValueType ResolveExpectedValueType() const;

	/** "(ParamTypes) -> ReturnType" string for the currently-selected function on the resolved TargetClass. */
	FText FormatSelectedFunctionSignature(TSharedRef<IPropertyHandle> FunctionNameHandle) const;

	TSharedPtr<IPropertyHandle> StructHandlePtr;
	TSharedPtr<IPropertyHandle> TargetClassHandle;
	TSharedPtr<IPropertyHandle> GetterHandle;
	TSharedPtr<IPropertyHandle> SetterHandle;
	TSharedPtr<IPropertyHandle> SaveGameSlotHandle;

	/**
	 * Ranked function lists, recomputed on TargetClass changes. Heap-shared
	 * because SComboBox holds a raw OptionsSource pointer: each combo widget
	 * co-owns its array (via a delegate capture in BuildFunctionPicker), so
	 * the arrays stay valid even if the widgets outlive this customization.
	 */
	TSharedRef<TArray<TSharedPtr<FFunctionOption>>> GetterOptions = MakeShared<TArray<TSharedPtr<FFunctionOption>>>();
	TSharedRef<TArray<TSharedPtr<FFunctionOption>>> SetterOptions = MakeShared<TArray<TSharedPtr<FFunctionOption>>>();

	/** Combo widgets retained so OnTargetClassChanged can refresh them in place. */
	TSharedPtr<SComboBox<TSharedPtr<FFunctionOption>>> GetterComboBox;
	TSharedPtr<SComboBox<TSharedPtr<FFunctionOption>>> SetterComboBox;
};
