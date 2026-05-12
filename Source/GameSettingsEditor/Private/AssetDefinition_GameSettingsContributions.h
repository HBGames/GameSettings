// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "AssetDefinitionDefault.h"
#include "Contributions/GameSettingsContribution_Action.h"
#include "Contributions/GameSettingsContribution_Discrete.h"
#include "Contributions/GameSettingsContribution_Scalar.h"
#include "Contributions/GameSettingsContribution_Tab.h"
#include "Contributions/GameSettingsContribution_Toggle.h"

#include "AssetDefinition_GameSettingsContributions.generated.h"

#define LOCTEXT_NAMESPACE "GameSettingsEditor"

namespace GameSettingsAssetCategories
{
	inline TConstArrayView<FAssetCategoryPath> GetCategories()
	{
		static const FAssetCategoryPath Categories[] =
		{
			FAssetCategoryPath(LOCTEXT("UICategory", "UI"), LOCTEXT("SettingsSubCategory", "Settings"))
		};
		return Categories;
	}
}

UCLASS()
class UAssetDefinition_GameSettingsContribution_Tab : public UAssetDefinitionDefault
{
	GENERATED_BODY()
public:
	virtual FText GetAssetDisplayName() const override { return LOCTEXT("Tab_Name", "Settings Tab"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(0.20f, 0.45f, 0.80f); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UGameSettingsContribution_Tab::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override { return GameSettingsAssetCategories::GetCategories(); }
};

UCLASS()
class UAssetDefinition_GameSettingsContribution_Toggle : public UAssetDefinitionDefault
{
	GENERATED_BODY()
public:
	virtual FText GetAssetDisplayName() const override { return LOCTEXT("Toggle_Name", "Settings Toggle"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(0.30f, 0.75f, 0.40f); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UGameSettingsContribution_Toggle::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override { return GameSettingsAssetCategories::GetCategories(); }
};

UCLASS()
class UAssetDefinition_GameSettingsContribution_Scalar : public UAssetDefinitionDefault
{
	GENERATED_BODY()
public:
	virtual FText GetAssetDisplayName() const override { return LOCTEXT("Scalar_Name", "Settings Scalar"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(0.90f, 0.55f, 0.20f); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UGameSettingsContribution_Scalar::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override { return GameSettingsAssetCategories::GetCategories(); }
};

UCLASS()
class UAssetDefinition_GameSettingsContribution_Discrete : public UAssetDefinitionDefault
{
	GENERATED_BODY()
public:
	virtual FText GetAssetDisplayName() const override { return LOCTEXT("Discrete_Name", "Settings Discrete"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(0.65f, 0.30f, 0.80f); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UGameSettingsContribution_Discrete::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override { return GameSettingsAssetCategories::GetCategories(); }
};

UCLASS()
class UAssetDefinition_GameSettingsContribution_Action : public UAssetDefinitionDefault
{
	GENERATED_BODY()
public:
	virtual FText GetAssetDisplayName() const override { return LOCTEXT("Action_Name", "Settings Action"); }
	virtual FLinearColor GetAssetColor() const override { return FLinearColor(0.85f, 0.30f, 0.30f); }
	virtual TSoftClassPtr<UObject> GetAssetClass() const override { return UGameSettingsContribution_Action::StaticClass(); }
	virtual TConstArrayView<FAssetCategoryPath> GetAssetCategories() const override { return GameSettingsAssetCategories::GetCategories(); }
};

#undef LOCTEXT_NAMESPACE
