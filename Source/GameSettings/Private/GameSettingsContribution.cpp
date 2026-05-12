// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsContribution.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContribution)

const FPrimaryAssetType UGameSettingsContribution::PrimaryAssetType = FPrimaryAssetType(TEXT("GameSettingsContribution"));

FPrimaryAssetId UGameSettingsContribution::GetPrimaryAssetId() const
{
	if (HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		return FPrimaryAssetId();
	}
	return FPrimaryAssetId(GetContributionPrimaryAssetType(), GetFName());
}

FPrimaryAssetType UGameSettingsContribution::GetContributionPrimaryAssetType() const
{
	return PrimaryAssetType;
}
