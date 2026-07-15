// Copyright Hitbox Games, LLC. All Rights Reserved.
// Originally derived from Lyra's UKeyAlreadyBoundWarning (Copyright Epic Games, Inc.).

#include "Widgets/Misc/KeyAlreadyBoundWarning.h"

#include "Components/TextBlock.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(KeyAlreadyBoundWarning)

void UKeyAlreadyBoundWarning::SetWarningText(const FText& InText)
{
	if (WarningText)
	{
		WarningText->SetText(InText);
	}
}

void UKeyAlreadyBoundWarning::SetCancelText(const FText& InText)
{
	if (CancelText)
	{
		CancelText->SetText(InText);
	}
}
