// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "DataSource/GameSettingDataSource.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "GameSettingFilterState.h"
#include "GameSettingValueBool.h"
#include "GameSettingValueDiscreteDynamic.h"
#include "GameSettingValueScalarDynamic.h"
#include "Misc/AutomationTest.h"
#include "UObject/PrimaryAssetId.h"

class UGameSettingRegistry;

namespace UE::GameSettings::Tests
{
	/**
	 * Shared flags for every GameSettings automation test. Defined once here
	 * (Unity builds merge test cpps into one translation unit, so per-file
	 * constants would collide).
	 */
	inline constexpr EAutomationTestFlags GameSettingsTestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/**
	 * Fake data source backed by one in-memory FString. Every call ignores the
	 * ULocalPlayer*, so bool/scalar value settings can be tested without a
	 * running game. The string uses the same format the real property-path
	 * data source reads and writes.
	 */
	class FInMemoryDataSource : public FGameSettingDataSource
	{
	public:
		explicit FInMemoryDataSource(const FString& InInitial = FString())
			: Stored(InInitial)
		{
		}

		virtual bool Resolve(ULocalPlayer* /*InContext*/) override { return true; }

		virtual FString GetValueAsString(ULocalPlayer* /*InContext*/) const override { return Stored; }

		virtual void SetValue(ULocalPlayer* /*InContext*/, const FString& Value) override { Stored = Value; }

		virtual FString ToString() const override { return TEXT("FInMemoryDataSource"); }

		FString Stored;
	};

	/** Build a primary-asset-id with a stable test type so ids are well-formed. */
	inline FPrimaryAssetId MakeTestId(FName Name, const TCHAR* Type = TEXT("GameSettingsTest"))
	{
		return FPrimaryAssetId(FPrimaryAssetType(Type), Name);
	}

	/** Construct an unregistered bool setting wired to a shared in-memory store. */
	inline UGameSettingValueBool* MakeBool(UObject* Outer, FName IdName,
		const TSharedRef<FInMemoryDataSource>& Store)
	{
		UGameSettingValueBool* Setting = NewObject<UGameSettingValueBool>(Outer);
		Setting->SetSettingId(MakeTestId(IdName));
		Setting->SetDisplayName(FText::FromName(IdName));
		Setting->SetDynamicGetter(Store);
		Setting->SetDynamicSetter(Store);
		return Setting;
	}

	/** Construct an unregistered scalar setting wired to a shared in-memory store. */
	inline UGameSettingValueScalarDynamic* MakeScalar(UObject* Outer, FName IdName,
		const TSharedRef<FInMemoryDataSource>& Store,
		const TRange<double>& Range, double Step)
	{
		UGameSettingValueScalarDynamic* Setting = NewObject<UGameSettingValueScalarDynamic>(Outer);
		Setting->SetSettingId(MakeTestId(IdName));
		Setting->SetDisplayName(FText::FromName(IdName));
		Setting->SetDynamicGetter(Store);
		Setting->SetDynamicSetter(Store);
		Setting->SetSourceRangeAndStep(Range, Step);
		Setting->SetDisplayFormat(UGameSettingValueScalarDynamic::Raw);
		return Setting;
	}

	/** Construct an unregistered discrete setting wired to a shared in-memory store. */
	inline UGameSettingValueDiscreteDynamic* MakeDiscrete(UObject* Outer, FName IdName,
		const TSharedRef<FInMemoryDataSource>& Store,
		std::initializer_list<const TCHAR*> Options)
	{
		UGameSettingValueDiscreteDynamic* Setting = NewObject<UGameSettingValueDiscreteDynamic>(Outer);
		Setting->SetSettingId(MakeTestId(IdName));
		Setting->SetDisplayName(FText::FromName(IdName));
		Setting->SetDynamicGetter(Store);
		Setting->SetDynamicSetter(Store);
		for (const TCHAR* Option : Options)
		{
			Setting->AddDynamicOption(Option, FText::FromString(Option));
		}
		return Setting;
	}

	/**
	 * Bare ULocalPlayer for settings that need a non-null LocalPlayer to run
	 * Initialize / RefreshEditableState. Never registered with a GameInstance,
	 * so no subsystems exist on it; our in-memory data sources ignore it.
	 * Outer is GEngine (ULocalPlayer is UCLASS(Within=Engine)).
	 */
	inline ULocalPlayer* MakeTestLocalPlayer()
	{
		return GEngine ? NewObject<ULocalPlayer>(GEngine) : nullptr;
	}

	/**
	 * Edit condition that disables a fixed list of discrete option values.
	 * Install via AddEditCondition before UGameSetting::Initialize so the
	 * editable-state cache picks it up.
	 */
	class FDisabledOptionsCondition : public FGameSettingEditCondition
	{
	public:
		explicit FDisabledOptionsCondition(TArray<FString> InDisabled)
			: DisabledOptions(MoveTemp(InDisabled))
		{
		}

		virtual void GatherEditState(const ULocalPlayer* /*InLocalPlayer*/, FGameSettingEditableState& InOutEditState) const override
		{
			for (const FString& Option : DisabledOptions)
			{
				InOutEditState.DisableOption(Option);
			}
		}

		virtual FString ToString() const override { return TEXT("FDisabledOptionsCondition"); }

	private:
		TArray<FString> DisabledOptions;
	};
}

#endif // WITH_DEV_AUTOMATION_TESTS
