// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsBinding.h"
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

		PropertyModule.NotifyCustomizationModuleChanged();
	}

	virtual void ShutdownModule() override
	{
		if (FPropertyEditorModule* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
		{
			PropertyModule->UnregisterCustomPropertyTypeLayout(FGameSettingsBinding::StaticStruct()->GetFName());
		}
	}
};

IMPLEMENT_MODULE(FGameSettingsEditorModule, GameSettingsEditor)
