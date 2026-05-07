// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Templates/SharedPointer.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPath.h"

#include "GameSettingsBinding.generated.h"

#define UE_API GAMESETTINGS_API

class FGameSettingDataSource;
class USubsystem;

#if WITH_EDITOR
class FDataValidationContext;
enum class EDataValidationResult : uint8;
#endif

/**
 * Designer-facing description of a property/function path to bind a typed
 * contribution to. Resolves to FGameSettingDataSourceFromSubsystem at runtime.
 *
 * The subsystem family (LocalPlayer / GameInstance / World / Engine) is
 * inferred from SubsystemClass at resolve time. GetterFunctionName and
 * SetterFunctionName must be UFUNCTIONs on that subsystem; rename them via
 * UE's reflection-aware refactor so the FName references stay live.
 */
USTRUCT(BlueprintType)
struct FGameSettingsBinding
{
	GENERATED_BODY()

	/** Subsystem class that owns the getter/setter functions. */
	UPROPERTY(EditAnywhere, Category = "Binding", meta = (MetaClass = "/Script/Engine.Subsystem", AllowAbstract = "false"))
	FSoftClassPath SubsystemClass;

	/** UFUNCTION on SubsystemClass that reads the value. */
	UPROPERTY(EditAnywhere, Category = "Binding")
	FName GetterFunctionName;

	/** UFUNCTION on SubsystemClass that writes the value. */
	UPROPERTY(EditAnywhere, Category = "Binding")
	FName SetterFunctionName;

	/** True when both the class loads and both function names resolve. */
	UE_API bool IsValid() const;

	/** Build a getter data source. Returns null if the binding is invalid. */
	UE_API TSharedPtr<FGameSettingDataSource> CreateGetter() const;

	/** Build a setter data source. Returns null if the binding is invalid. */
	UE_API TSharedPtr<FGameSettingDataSource> CreateSetter() const;

#if WITH_EDITOR
	/** Editor-time validation. Pass an asset name for prefixing error messages. */
	UE_API EDataValidationResult Validate(FDataValidationContext& Context, const FString& OwnerLabel) const;
#endif
};

#undef UE_API
