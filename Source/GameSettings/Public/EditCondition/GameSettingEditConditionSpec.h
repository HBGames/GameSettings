// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "EditCondition/GameSettingEditAction.h"
#include "Templates/SharedPointer.h"
#include "UObject/Object.h"

#include "GameSettingEditConditionSpec.generated.h"

#define UE_API GAMESETTINGS_API

class FGameSettingEditCondition;
class FGameSettingEditableState;
class UGameSetting;
class UGameSettingRegistry;
struct FPrimaryAssetId;

#if WITH_EDITOR
class FDataValidationContext;
enum class EDataValidationResult : uint8;
#endif

/**
 * Designer-facing description of an edit condition to attach to a setting at
 * Apply() time. Subclasses produce the runtime FGameSettingEditCondition via
 * BuildCondition. The registry handles dependency wiring (AddEditDependency)
 * and arrival-order safety automatically based on GetSettingDependencies.
 *
 * Lifetime contract: BuildCondition lambdas MUST capture target settings as
 * TWeakObjectPtr<UGameSetting> rather than raw pointers, so a GameFeature
 * unload that removes the target can be cleaned up safely by the registry's
 * CleanupEditConditionsForRemovedTarget pass.
 */
UCLASS(MinimalAPI, Abstract, EditInlineNew, DefaultToInstanced, Blueprintable, CollapseCategories)
class UGameSettingEditConditionSpec : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Produce the runtime condition to attach to the owner. Return null if
	 * the spec needs to defer (the registry tolerates this and will retry on
	 * the next flush).
	 *
	 * Owner is non-null; Registry is non-null. Use Registry.FindSettingById
	 * to resolve target settings declared via GetSettingDependencies.
	 */
	virtual TSharedPtr<FGameSettingEditCondition> BuildCondition(UGameSettingRegistry& Registry, UGameSetting& Owner) const
	PURE_VIRTUAL(UGameSettingEditConditionSpec::BuildCondition, return nullptr;);

	/**
	 * Setting primary-asset-ids this spec reads from. The registry uses this
	 * to know what to wire AddEditDependency against and what to wait for
	 * during deferred resolution. Default: empty (most non-cross-setting
	 * specs like PlatformTrait and PrimaryPlayerOnly).
	 */
	virtual void GetSettingDependencies(TArray<FPrimaryAssetId>& OutIds) const
	{
	}

	/** Dev-only debug string for "why is this disabled" overlays. */
	virtual FString DebugDescribe() const { return GetClass()->GetName(); }

#if WITH_EDITOR
	UE_API virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif

protected:
	/**
	 * Shared helper invoked by every concrete spec's BuildCondition lambda
	 * when the predicate fails. Reads Action / DisableReason / DevReason and
	 * routes to the right FGameSettingEditableState mutator.
	 */
	UE_API void ApplyActionToState(FGameSettingEditableState& State) const;

	/** Hide / Disable / Kill - what to do when the predicate fails. */
	UPROPERTY(EditAnywhere, Category = "Action")
	EGameSettingEditAction Action = EGameSettingEditAction::Disable;

	/** User-facing reason. Required when Action == Disable. */
	UPROPERTY(EditAnywhere, Category = "Action", meta = (EditCondition = "Action == EGameSettingEditAction::Disable", EditConditionHides))
	FText DisableReason;

	/** Dev-only reason. Required when Action == Hide or Kill (not localized). */
	UPROPERTY(EditAnywhere, Category = "Action", meta = (EditCondition = "Action != EGameSettingEditAction::Disable", EditConditionHides))
	FString DevReason;
};

#undef UE_API
