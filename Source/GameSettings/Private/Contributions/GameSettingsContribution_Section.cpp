// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsContribution_Section.h"

#include "GameSettingCollection.h"
#include "GameSettingRegistry.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContribution_Section)

#define LOCTEXT_NAMESPACE "GameSettings"

const FPrimaryAssetType UGameSettingsContribution_Section::ContributionPrimaryAssetType = FPrimaryAssetType(TEXT("GameSettingsSection"));

FPrimaryAssetType UGameSettingsContribution_Section::GetContributionPrimaryAssetType() const
{
	return ContributionPrimaryAssetType;
}

void UGameSettingsContribution_Section::Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
{
	if (!GetPrimaryAssetId().IsValid() || DisplayName.IsEmpty())
	{
		return;
	}

	UGameSettingCollection* Section = NewObject<UGameSettingCollection>(&Registry);
	ConfigureBaseSetting(*Section);

	// Route through AddCollection so the registry knows this is a container
	// (and so deferred children waiting for this section's id get unblocked).
	const FGameSettingHandle Handle = Registry.AddCollection(Section, ParentContainer);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
	}
}

#if WITH_EDITOR
EDataValidationResult UGameSettingsContribution_Section::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!ParentContainer.IsValid())
	{
		Context.AddError(LOCTEXT("Section_NoParent",
			"Section contribution: ParentContainer is required - set it to a Tab (top-level section) or another Section (nested)."));
		Result = EDataValidationResult::Invalid;
	}
	else if (ParentContainer == GetPrimaryAssetId())
	{
		Context.AddError(LOCTEXT("Section_SelfParent",
			"Section contribution: ParentContainer cannot reference this section itself."));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
