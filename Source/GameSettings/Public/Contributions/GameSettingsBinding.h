// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Contributions/GameSettingsBindingValueType.h"
#include "Templates/SharedPointer.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPath.h"

#include "GameSettingsBinding.generated.h"

#define UE_API GAMESETTINGS_API

class FGameSettingDataSource;
class UObject;

#if WITH_EDITOR
class FDataValidationContext;
enum class EDataValidationResult : uint8;
#endif

/**
 * Designer-facing description of a property/function path to bind a typed
 * contribution to. Resolves to a concrete FGameSettingDataSource subclass at
 * runtime based on the target class shape:
 *
 *  - USubsystem derivation     -> FGameSettingDataSourceFromSubsystem
 *  - UGameUserSettings         -> FGameSettingDataSourceFromGameUserSettings
 *  - ULocalPlayerSaveGame      -> FGameSettingDataSourceFromLocalPlayerSaveGame
 *
 * GetterFunctionName and SetterFunctionName must be UFUNCTIONs on the target
 * class. WARNING: the names are stored as raw FNames in saved contribution
 * assets - nothing fixes them up when a UFUNCTION is renamed. Renaming a
 * bound function breaks every saved (including shipped) asset that
 * references it: the binding stops resolving, the row logs an error from
 * Apply() and does not appear. Update all referencing assets when renaming.
 */
USTRUCT(BlueprintType)
struct FGameSettingsBinding
{
	GENERATED_BODY()

	/**
	 * Class that owns the getter/setter functions. Must derive from one of:
	 *   USubsystem (LocalPlayer / GameInstance / World / Engine)
	 *   UGameUserSettings (the engine singleton; resolves via GEngine->GetGameUserSettings())
	 *   ULocalPlayerSaveGame (resolved per-LocalPlayer; one cached instance per slot)
	 *
	 * Examples:
	 *   UEFPSettingsLocal         (engine GameUserSettings subclass for video/audio)
	 *   UEFPSettingsShared        (player-scoped save game for accessibility/mouse)
	 *   UVoiceChatGameSubsystem   (per-GameInstance subsystem owning voice prefs)
	 */
	UPROPERTY(EditAnywhere, Category = "Binding", meta = (MetaClass = "/Script/CoreUObject.Object", AllowAbstract = "false"))
	FSoftClassPath TargetClass;

	/**
	 * UFUNCTION on TargetClass that reads the value. Use the dropdown if available;
	 * only zero-argument functions whose return type matches the contribution's
	 * value shape are listed, ranked by name similarity to the SettingId.
	 *
	 * Examples: GetResolutionScaleNormalized, IsVSyncEnabled, GetOverallVolume.
	 */
	UPROPERTY(EditAnywhere, Category = "Binding")
	FName GetterFunctionName;

	/**
	 * UFUNCTION on TargetClass that writes the value. Same dropdown rules as the
	 * getter; single-input functions whose param type matches the contribution
	 * are listed.
	 *
	 * Examples: SetResolutionScaleNormalized, SetVSyncEnabled, SetOverallVolume.
	 */
	UPROPERTY(EditAnywhere, Category = "Binding")
	FName SetterFunctionName;

	/**
	 * Save-game slot name. Used only when TargetClass derives from
	 * ULocalPlayerSaveGame. Leave blank to default to TargetClass->GetName(),
	 * which is what every contribution should use unless you intentionally want
	 * multiple slots for the same save-game class.
	 *
	 * Examples: "SharedGameSettings" (typical default), "DebugSettings" (separate
	 * slot just for debug builds).
	 */
	UPROPERTY(EditAnywhere, Category = "Binding", AdvancedDisplay)
	FName SaveGameSlotName;

	/** True when the class loads and both function names resolve. */
	UE_API bool IsValid() const;

	/** Build a getter data source. Returns null if the binding is invalid. */
	UE_API TSharedPtr<FGameSettingDataSource> CreateGetter() const;

	/** Build a setter data source. Returns null if the binding is invalid. */
	UE_API TSharedPtr<FGameSettingDataSource> CreateSetter() const;

	/**
	 * Read the C++-declared default by invoking the getter on TargetClass's
	 * CDO (not the live instance, not the ini-loaded value). This is the
	 * factory default a Reset-To-Default should restore to. Returns false if
	 * the class can't load or the getter doesn't resolve.
	 *
	 * Meaningful for UGameUserSettings / ULocalPlayerSaveGame targets whose
	 * CDO carries real member initializers. For USubsystem targets the CDO is
	 * rarely configured, so the caller should treat a false return (or an
	 * unexpected value) as "fall back to the explicit default field."
	 */
	UE_API bool TryGetClassDefaultValueAsString(FString& OutValue) const;

#if WITH_EDITOR
	/**
	 * Editor-time validation. Pass an asset name for prefixing error messages.
	 *
	 * When ExpectedValueType is not Unknown, the validator also checks that the
	 * getter's return type and the setter's input type match the expected shape
	 * (e.g. a Toggle contribution passes Boolean; the validator errors if the
	 * resolved getter actually returns a float).
	 */
	UE_API EDataValidationResult Validate(
		FDataValidationContext& Context,
		const FString& OwnerLabel,
		EGameSettingsBindingValueType ExpectedValueType = EGameSettingsBindingValueType::Unknown) const;
#endif
};

#undef UE_API
