// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

/**
 * Module for GameSettings automation tests. Carries no runtime behavior —
 * tests live as IMPLEMENT_SIMPLE_AUTOMATION_TEST definitions in Private/,
 * gated on WITH_DEV_AUTOMATION_TESTS, and surfaced through the Session
 * Frontend.
 */
class FGameSettingsTestsModule : public IModuleInterface
{
};
