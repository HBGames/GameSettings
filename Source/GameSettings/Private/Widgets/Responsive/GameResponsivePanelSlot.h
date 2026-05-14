// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's GameSettings plugin (Copyright Epic Games, Inc.).

#pragma once

#include "Components/PanelSlot.h"
#include "SGameResponsivePanel.h"

#include "GameResponsivePanelSlot.generated.h"

class UObject;

/**
 * Slot type used by UGameResponsivePanel for each of its children. Currently
 * carries no slot-specific properties; the panel itself drives row/column
 * placement based on wrap state. Exists so the panel's GetSlotClass override
 * returns a typed slot rather than the generic UPanelSlot.
 */
UCLASS()
class UGameResponsivePanelSlot : public UPanelSlot
{
	GENERATED_UCLASS_BODY()

public:

	void BuildSlot(TSharedRef<SGameResponsivePanel> GameResponsivePanel);

	// UPanelSlot interface
	virtual void SynchronizeProperties() override;
	// End of UPanelSlot interface

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	SGameResponsivePanel::FSlot* Slot;
};
