// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "GameSettingDataSource.h"
#include "PropertyPathHelpers.h"
#include "Templates/SubclassOf.h"

#define UE_API GAMESETTINGS_API

class ULocalPlayer;
class USubsystem;

/**
 * Data source that resolves a property/function chain rooted at a USubsystem
 * instance instead of the LocalPlayer itself.
 *
 * Use this when a plugin owns its own backing storage on a ULocalPlayerSubsystem,
 * UGameInstanceSubsystem, UWorldSubsystem, or UEngineSubsystem and you want to
 * bind a UGameSettingValueDiscreteDynamic / Scalar / etc. to it without making
 * the project's ULocalPlayer aware of your accessor.
 *
 * The subsystem family (LocalPlayer/GameInstance/World/Engine) is inferred at
 * resolve time from the class hierarchy of SubsystemClass. The GET_SUBSYSTEM_SETTINGS_FUNCTION_PATH
 * helper macro is the typical construction site:
 *
 *     Setting->SetDynamicGetter(GET_SUBSYSTEM_SETTINGS_FUNCTION_PATH(UMyPluginSubsystem, GetSomeValue));
 *     Setting->SetDynamicSetter(GET_SUBSYSTEM_SETTINGS_FUNCTION_PATH(UMyPluginSubsystem, SetSomeValue));
 */
class FGameSettingDataSourceFromSubsystem : public FGameSettingDataSource
{
public:
	UE_API FGameSettingDataSourceFromSubsystem(TSubclassOf<USubsystem> InSubsystemClass, const TArray<FString>& InPathFromSubsystem);

	UE_API virtual bool Resolve(ULocalPlayer* InLocalPlayer) override;
	UE_API virtual FString GetValueAsString(ULocalPlayer* InLocalPlayer) const override;
	UE_API virtual void SetValue(ULocalPlayer* InLocalPlayer, const FString& Value) override;
	UE_API virtual FString ToString() const override;

private:
	UE_API USubsystem* ResolveSubsystem(ULocalPlayer* InLocalPlayer) const;

	TSubclassOf<USubsystem> SubsystemClass;
	FCachedPropertyPath PropertyPath;
};

/**
 * Build an FGameSettingDataSourceFromSubsystem path with compile-time checking.
 * SubsystemClass must be the actual class type (not a TSubclassOf), and
 * FunctionOrPropertyName must be a UFUNCTION or reflected UPROPERTY on it.
 */
#define GET_SUBSYSTEM_SETTINGS_FUNCTION_PATH(SubsystemClass, FunctionOrPropertyName) \
	MakeShared<FGameSettingDataSourceFromSubsystem>( \
		SubsystemClass::StaticClass(), \
		TArray<FString>({ GET_FUNCTION_NAME_STRING_CHECKED(SubsystemClass, FunctionOrPropertyName) }))

#undef UE_API
