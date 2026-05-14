// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Tab.h"

#include "GameSettingCollection.h"
#include "GameSettingRegistry.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContribution_Tab)

#define LOCTEXT_NAMESPACE "GameSettings"

const FPrimaryAssetType UGameSettingsContribution_Tab::ContributionPrimaryAssetType = FPrimaryAssetType(TEXT("GameSettingsTab"));

FPrimaryAssetType UGameSettingsContribution_Tab::GetContributionPrimaryAssetType() const
{
	return ContributionPrimaryAssetType;
}

void UGameSettingsContribution_Tab::Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
{
	if (!GetPrimaryAssetId().IsValid() || DisplayName.IsEmpty())
	{
		return;
	}

	UGameSettingCollection* Tab = NewObject<UGameSettingCollection>(&Registry);
	ConfigureBaseSetting(*Tab);

	const FGameSettingHandle Handle = Registry.AddCollection(Tab);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameSettingsContribution_Tab::IsDataValid(FDataValidationContext& Context) const
{
	// Base class checks identity + DisplayName; nothing tab-specific to add.
	return Super::IsDataValid(Context);
}
#endif

#undef LOCTEXT_NAMESPACE
