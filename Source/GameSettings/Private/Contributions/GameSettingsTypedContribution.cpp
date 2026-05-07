// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsTypedContribution.h"

#include "GameSetting.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsTypedContribution)

void UGameSettingsTypedContribution::ConfigureBaseSetting(UGameSetting& Setting) const
{
	if (SettingId.IsValid())
	{
		Setting.SetSettingId(SettingId);
	}
	if (!DisplayName.IsEmpty())
	{
		Setting.SetDisplayName(DisplayName);
	}
	if (!DescriptionRichText.IsEmpty())
	{
		Setting.SetDescriptionRichText(DescriptionRichText);
	}
}
