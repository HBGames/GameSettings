// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Tab.h"

#include "GameSettingCollection.h"
#include "GameSettingRegistry.h"
#include "GameSettingsLog.h"

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
		UE_LOG(LogGameSettings, Error, TEXT("Tab contribution %s skipped: %s."),
			*GetPathName(),
			!GetPrimaryAssetId().IsValid() ? TEXT("primary asset id is invalid") : TEXT("DisplayName is empty"));
		return;
	}

	UGameSettingCollection* Tab = NewObject<UGameSettingCollection>(&Registry);
	ConfigureBaseSetting(*Tab);

	const FGameSettingHandle Handle = Registry.AddCollection(Tab);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
		Registry.ApplyEditConditionSpecs(Tab, EditConditions);
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
