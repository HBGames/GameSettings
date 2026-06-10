// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's GameSettings plugin (Copyright Epic Games, Inc.).

#include "SGameResponsivePanel.h"

#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformApplicationMisc.h"
#include "Layout/ArrangedChildren.h"
#include "Widgets/SViewport.h"

#define LOCTEXT_NAMESPACE "GameSetting"

SGameResponsivePanel::SGameResponsivePanel()
	: InnerGrid(SNew(SGridPanel))
{
	SetCanTick(false);
	bCanSupportFocus = false;
	bHasCustomPrepass = true;
	bHasRelativeLayoutScale = true;
	bCanWrapVertically = true;
}

void SGameResponsivePanel::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		InnerGrid
	];
}

SGridPanel::FSlot& SGameResponsivePanel::AddSlot()
{
	SGridPanel::FSlot* Slot;
	InnerGrid->AddSlot(InnerGrid->GetChildren()->Num(), 0)
		.Expose(Slot);
	InnerSlots.Add(Slot);

	RefreshLayout();

	return *Slot;
}

int32 SGameResponsivePanel::RemoveSlot(const TSharedRef<SWidget>& SlotWidget)
{
	for (int32 SlotIdx = 0; SlotIdx < InnerSlots.Num(); ++SlotIdx)
	{
		if (SlotWidget == InnerSlots[SlotIdx]->GetWidget())
		{
			InnerSlots.RemoveAt(SlotIdx);
			break;
		}
	}

	const int32 Result = InnerGrid->RemoveSlot(SlotWidget);

	// Re-pack surviving slots: without this they keep their old column/row
	// indices (a gap where the removed slot was) and the empty column keeps
	// its fill weight until the next add or wrap flip.
	RefreshLayout();

	return Result;
}

void SGameResponsivePanel::ClearChildren()
{
	// Drop the slot cache with the slots; the grid owns the FSlot memory, so
	// stale entries here would dangle into the next AddSlot/RefreshLayout.
	InnerSlots.Reset();
	InnerGrid->ClearChildren();
}

void SGameResponsivePanel::EnableVerticalStacking(const bool bCanVerticallyWrap)
{
	bCanWrapVertically = bCanVerticallyWrap;
}

bool SGameResponsivePanel::CustomPrepass(float LayoutScaleMultiplier)
{
	RefreshResponsiveness();
	return true;
}

void SGameResponsivePanel::OnArrangeChildren(const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren) const
{
	ArrangedChildren.AddWidget(AllottedGeometry.MakeChild(
		ChildSlot.GetWidget(),
		FVector2D(0, 0),
		AllottedGeometry.GetLocalSize() / Scale,
		Scale
	));
}

FVector2D SGameResponsivePanel::ComputeDesiredSize(float InLayoutScale) const
{
	return SCompoundWidget::ComputeDesiredSize(InLayoutScale) * Scale;
}

float SGameResponsivePanel::GetRelativeLayoutScale(int32 ChildIndex, float LayoutScaleMultiplier) const
{
	return Scale;
}

bool SGameResponsivePanel::ShouldWrap() const
{
	if (PhysicalScreenSize.IsZero() || !bCanWrapVertically)
	{
		return false;
	}

	return (PhysicalScreenSize.X < 7);
}

void SGameResponsivePanel::RefreshResponsiveness()
{
	PhysicalScreenSize = FVector2D(0, 0);

	TSharedPtr<SViewport> GameViewport = FSlateApplication::Get().GetGameViewport();
	if (GameViewport.IsValid())
	{
		TSharedPtr<ISlateViewport> ViewportInterface = GameViewport->GetViewportInterface().Pin();
		if (ViewportInterface.IsValid())
		{
			const FIntPoint ViewportSize = ViewportInterface->GetSize();

			int32 ScreenDensity = 0;
			FPlatformApplicationMisc::GetPhysicalScreenDensity(ScreenDensity);

			if (ScreenDensity != 0)
			{
				PhysicalScreenSize = ViewportSize / (float)ScreenDensity;
			}
		}
	}

	const bool bShouldWrap = ShouldWrap();
	const float NewScale = bShouldWrap ? 1.5f : 1.0f;
	if (!FMath::IsNearlyEqual(NewScale, Scale))
	{
		Scale = NewScale;
		RefreshLayout();
		Invalidate(EInvalidateWidgetReason::Prepass);
	}
}

void SGameResponsivePanel::RefreshLayout()
{
	const bool bShouldWrap = ShouldWrap();

	InnerGrid->ClearFill();

	for (int32 SlotIdx = 0; SlotIdx < InnerSlots.Num(); ++SlotIdx)
	{
		InnerSlots[SlotIdx]->SetColumn(bShouldWrap ? 0 : SlotIdx);
		InnerSlots[SlotIdx]->SetRow(bShouldWrap ? SlotIdx : 0);

		if (!bShouldWrap)
		{
			InnerGrid->SetColumnFill(SlotIdx, 1.0f);
		}
	}

	if (bShouldWrap)
	{
		InnerGrid->SetColumnFill(0, 1.0f);
	}
}

#undef LOCTEXT_NAMESPACE
