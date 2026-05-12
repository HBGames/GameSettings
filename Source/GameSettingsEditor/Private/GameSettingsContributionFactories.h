// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Factories/Factory.h"

#include "GameSettingsContributionFactories.generated.h"

/**
 * Factories that surface each typed contribution as a first-class entry in
 * the Content Browser's right-click "Add New" menu (under UI > Settings,
 * per the matching UAssetDefinition's category). Without these the only
 * way to create a contribution is the generic Miscellaneous > Data Asset
 * class picker.
 */

UCLASS()
class UFactory_GameSettingsContribution_Tab : public UFactory
{
	GENERATED_BODY()
public:
	UFactory_GameSettingsContribution_Tab();
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class UFactory_GameSettingsContribution_Toggle : public UFactory
{
	GENERATED_BODY()
public:
	UFactory_GameSettingsContribution_Toggle();
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class UFactory_GameSettingsContribution_Scalar : public UFactory
{
	GENERATED_BODY()
public:
	UFactory_GameSettingsContribution_Scalar();
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class UFactory_GameSettingsContribution_Discrete : public UFactory
{
	GENERATED_BODY()
public:
	UFactory_GameSettingsContribution_Discrete();
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

UCLASS()
class UFactory_GameSettingsContribution_Action : public UFactory
{
	GENERATED_BODY()
public:
	UFactory_GameSettingsContribution_Action();
	virtual UObject* FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};
