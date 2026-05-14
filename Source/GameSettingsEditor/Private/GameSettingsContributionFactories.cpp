// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsContributionFactories.h"

#include "Contributions/GameSettingsContribution_Action.h"
#include "Contributions/GameSettingsContribution_Discrete.h"
#include "Contributions/GameSettingsContribution_Scalar.h"
#include "Contributions/GameSettingsContribution_Section.h"
#include "Contributions/GameSettingsContribution_Tab.h"
#include "Contributions/GameSettingsContribution_Toggle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsContributionFactories)

UFactory_GameSettingsContribution_Tab::UFactory_GameSettingsContribution_Tab()
{
	SupportedClass = UGameSettingsContribution_Tab::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}
UObject* UFactory_GameSettingsContribution_Tab::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject*, FFeedbackContext*)
{
	return NewObject<UGameSettingsContribution_Tab>(InParent, InClass, InName, Flags);
}

UFactory_GameSettingsContribution_Section::UFactory_GameSettingsContribution_Section()
{
	SupportedClass = UGameSettingsContribution_Section::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}
UObject* UFactory_GameSettingsContribution_Section::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject*, FFeedbackContext*)
{
	return NewObject<UGameSettingsContribution_Section>(InParent, InClass, InName, Flags);
}

UFactory_GameSettingsContribution_Toggle::UFactory_GameSettingsContribution_Toggle()
{
	SupportedClass = UGameSettingsContribution_Toggle::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}
UObject* UFactory_GameSettingsContribution_Toggle::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject*, FFeedbackContext*)
{
	return NewObject<UGameSettingsContribution_Toggle>(InParent, InClass, InName, Flags);
}

UFactory_GameSettingsContribution_Scalar::UFactory_GameSettingsContribution_Scalar()
{
	SupportedClass = UGameSettingsContribution_Scalar::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}
UObject* UFactory_GameSettingsContribution_Scalar::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject*, FFeedbackContext*)
{
	return NewObject<UGameSettingsContribution_Scalar>(InParent, InClass, InName, Flags);
}

UFactory_GameSettingsContribution_Discrete::UFactory_GameSettingsContribution_Discrete()
{
	SupportedClass = UGameSettingsContribution_Discrete::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}
UObject* UFactory_GameSettingsContribution_Discrete::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject*, FFeedbackContext*)
{
	return NewObject<UGameSettingsContribution_Discrete>(InParent, InClass, InName, Flags);
}

UFactory_GameSettingsContribution_Action::UFactory_GameSettingsContribution_Action()
{
	SupportedClass = UGameSettingsContribution_Action::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}
UObject* UFactory_GameSettingsContribution_Action::FactoryCreateNew(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject*, FFeedbackContext*)
{
	return NewObject<UGameSettingsContribution_Action>(InParent, InClass, InName, Flags);
}
