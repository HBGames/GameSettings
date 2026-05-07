// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Modules/ModuleManager.h"

class FGameSettingsGameFeaturesModule : public IModuleInterface
{
public:
	virtual void StartupModule() override {}
	virtual void ShutdownModule() override {}
};

IMPLEMENT_MODULE(FGameSettingsGameFeaturesModule, GameSettingsGameFeatures);
