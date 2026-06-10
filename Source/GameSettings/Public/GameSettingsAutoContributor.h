// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingsContribution.h"

#include "GameSettingsAutoContributor.generated.h"

/**
 * A contribution that auto-registers with every UGameSettingsSubsystem
 * (every LocalPlayer) on plugin load. Use it for settings that should
 * always be present, like an accessibility plugin's "high contrast UI"
 * toggle.
 *
 * Subclass and implement Apply() from UGameSettingsContribution. The
 * GameSettings module finds your subclass via GetDerivedClasses at startup
 * and again when a new module loads, applies it to every existing
 * LocalPlayer's subsystem, and applies it to any LocalPlayer that joins
 * later.
 *
 * Override ShouldAutoContribute() to gate registration at runtime (a
 * platform check, a beta flag, whatever).
 */
UCLASS(MinimalAPI, BlueprintType, Abstract)
class UGameSettingsAutoContributor : public UGameSettingsContribution
{
	GENERATED_BODY()
public:
	/** Override to gate auto-registration. Default is to register always. */
	virtual bool ShouldAutoContribute() const { return true; }
};
