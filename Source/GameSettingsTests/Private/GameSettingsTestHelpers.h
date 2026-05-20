// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "DataSource/GameSettingDataSource.h"
#include "GameSettingValueBool.h"
#include "GameSettingValueScalarDynamic.h"
#include "UObject/PrimaryAssetId.h"

class UGameSettingRegistry;

namespace UE::GameSettings::Tests
{
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
}

#endif // WITH_DEV_AUTOMATION_TESTS
