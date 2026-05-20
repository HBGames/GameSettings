// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsTypedContribution.h"

#include "GameSetting.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsTypedContribution)

#define LOCTEXT_NAMESPACE "GameSettings"

void UGameSettingsTypedContribution::ConfigureBaseSetting(UGameSetting& Setting) const
{
	const FPrimaryAssetId Id = GetPrimaryAssetId();
	if (Id.IsValid())
	{
		Setting.SetSettingId(Id);
	}
	if (!DisplayName.IsEmpty())
	{
		Setting.SetDisplayName(DisplayName);
	}
	if (!DescriptionRichText.IsEmpty())
	{
		Setting.SetDescriptionRichText(DescriptionRichText);
	}
	Setting.SetSortPriority(SortPriority);
}

#if WITH_EDITOR

EDataValidationResult UGameSettingsTypedContribution::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!GetPrimaryAssetId().IsValid())
	{
		Context.AddError(LOCTEXT("Typed_NoPrimaryAssetId",
			"Contribution has no PrimaryAssetId. Save the asset to disk so the asset manager can index it."));
		Result = EDataValidationResult::Invalid;
	}

	if (DisplayName.IsEmpty())
	{
		Context.AddError(LOCTEXT("Typed_NoDisplayName", "DisplayName is required."));
		Result = EDataValidationResult::Invalid;
	}

	// PrimaryAssetId uniqueness is enforced by the asset manager itself; no
	// duplicate-id walk is needed here.

	return Result;
}

#endif

#undef LOCTEXT_NAMESPACE
