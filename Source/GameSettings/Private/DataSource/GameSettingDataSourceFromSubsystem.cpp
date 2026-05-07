// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "DataSource/GameSettingDataSourceFromSubsystem.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameSettingsLog.h"
#include "Subsystems/EngineSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "Subsystems/Subsystem.h"
#include "Subsystems/WorldSubsystem.h"

FGameSettingDataSourceFromSubsystem::FGameSettingDataSourceFromSubsystem(
	TSubclassOf<USubsystem> InSubsystemClass,
	const TArray<FString>& InPathFromSubsystem)
	: SubsystemClass(InSubsystemClass)
{
	FString JoinedPath;
	for (int32 Index = 0; Index < InPathFromSubsystem.Num(); ++Index)
	{
		if (Index > 0)
		{
			JoinedPath += TEXT(".");
		}
		JoinedPath += InPathFromSubsystem[Index];
	}
	PropertyPath = FCachedPropertyPath(JoinedPath);
}

bool FGameSettingDataSourceFromSubsystem::Resolve(ULocalPlayer* InLocalPlayer)
{
	USubsystem* Subsystem = ResolveSubsystem(InLocalPlayer);
	if (!Subsystem)
	{
		return false;
	}
	return PropertyPath.Resolve(Subsystem);
}

FString FGameSettingDataSourceFromSubsystem::GetValueAsString(ULocalPlayer* InLocalPlayer) const
{
	USubsystem* Subsystem = ResolveSubsystem(InLocalPlayer);
	if (!Subsystem)
	{
		return FString();
	}

	FString Out;
	PropertyPathHelpers::GetPropertyValueAsString(Subsystem, PropertyPath.ToString(), Out);
	return Out;
}

void FGameSettingDataSourceFromSubsystem::SetValue(ULocalPlayer* InLocalPlayer, const FString& Value)
{
	USubsystem* Subsystem = ResolveSubsystem(InLocalPlayer);
	if (!Subsystem)
	{
		return;
	}
	PropertyPathHelpers::SetPropertyValueFromString(Subsystem, PropertyPath.ToString(), Value);
}

FString FGameSettingDataSourceFromSubsystem::ToString() const
{
	return FString::Printf(TEXT("%s.%s"),
		SubsystemClass ? *SubsystemClass->GetName() : TEXT("(null subsystem class)"),
		*PropertyPath.ToString());
}

USubsystem* FGameSettingDataSourceFromSubsystem::ResolveSubsystem(ULocalPlayer* InLocalPlayer) const
{
	UClass* SubClass = SubsystemClass.Get();
	if (!SubClass || !InLocalPlayer)
	{
		return nullptr;
	}

	if (SubClass->IsChildOf(ULocalPlayerSubsystem::StaticClass()))
	{
		return InLocalPlayer->GetSubsystemBase(SubClass);
	}
	if (SubClass->IsChildOf(UGameInstanceSubsystem::StaticClass()))
	{
		UGameInstance* GameInstance = InLocalPlayer->GetGameInstance();
		return GameInstance ? GameInstance->GetSubsystemBase(SubClass) : nullptr;
	}
	if (SubClass->IsChildOf(UWorldSubsystem::StaticClass()))
	{
		UWorld* World = InLocalPlayer->GetWorld();
		return World ? World->GetSubsystemBase(SubClass) : nullptr;
	}
	if (SubClass->IsChildOf(UEngineSubsystem::StaticClass()))
	{
		return GEngine ? GEngine->GetEngineSubsystemBase(SubClass) : nullptr;
	}

	UE_LOG(LogGameSettings, Warning,
		TEXT("FGameSettingDataSourceFromSubsystem: '%s' is not a recognized subsystem family (LocalPlayer/GameInstance/World/Engine)."),
		*SubClass->GetName());
	return nullptr;
}
