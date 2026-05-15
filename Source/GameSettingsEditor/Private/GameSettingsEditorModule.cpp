// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsBinding.h"
#include "EditCondition/Specs/GameSettingEditConditionSpec_DependsOnDiscrete.h"
#include "GameSettingEditConditionDiscreteCustomization.h"
#include "GameSettingsBindingCustomization.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

class FGameSettingsEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

		PropertyModule.RegisterCustomPropertyTypeLayout(
			FGameSettingsBinding::StaticStruct()->GetFName(),
			FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGameSettingsBindingCustomization::MakeInstance));

		PropertyModule.RegisterCustomClassLayout(
			UGameSettingEditConditionSpec_DependsOnDiscrete::StaticClass()->GetFName(),
			FOnGetDetailCustomizationInstance::CreateStatic(&FGameSettingEditConditionDiscreteCustomization::MakeInstance));

		PropertyModule.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (FPropertyEditorModule* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
		{
			PropertyModule->UnregisterCustomPropertyTypeLayout(FGameSettingsBinding::StaticStruct()->GetFName());
			PropertyModule->UnregisterCustomClassLayout(UGameSettingEditConditionSpec_DependsOnDiscrete::StaticClass()->GetFName());
		}
	}
};

IMPLEMENT_MODULE(FGameSettingsEditorModule, GameSettingsEditor)
