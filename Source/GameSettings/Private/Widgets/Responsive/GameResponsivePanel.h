// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's GameSettings plugin (Copyright Epic Games, Inc.).

#pragma once

#include "Components/PanelWidget.h"
#include "GameResponsivePanel.generated.h"

class UGameResponsivePanelSlot;

/**
 * UMG panel that lays children out horizontally on wide displays and stacks
 * them vertically when the physical screen is small enough to warrant it.
 * Useful for settings rows that want to render as [label | control | value]
 * on desktop and collapse to a vertical stack on narrow viewports
 * (split-screen, mobile, portrait windows).
 *
 * Wrapping can be disabled per-instance via bCanStackVertically.
 */
UCLASS()
class UGameResponsivePanel : public UPanelWidget
{
	GENERATED_UCLASS_BODY()

	/**  */
	UFUNCTION(BlueprintCallable, Category="Widget")
	UGameResponsivePanelSlot* AddChildToGameResponsivePanel(UWidget* Content);

#if WITH_EDITOR
	// UWidget interface
	virtual const FText GetPaletteCategory() override;
	// End UWidget interface
#endif

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Behavior")
	bool bCanStackVertically = true;

protected:

	// UPanelWidget
	virtual UClass* GetSlotClass() const override;
	virtual void OnSlotAdded(UPanelSlot* Slot) override;
	virtual void OnSlotRemoved(UPanelSlot* Slot) override;
	// End UPanelWidget

protected:

	TSharedPtr<class SGameResponsivePanel> MyGameResponsivePanel;

protected:
	// UWidget interface
	virtual TSharedRef<SWidget> RebuildWidget() override;
	// End of UWidget interface
};
