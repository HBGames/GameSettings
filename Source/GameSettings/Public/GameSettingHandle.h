// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#define UE_API GAMESETTINGS_API

/**
 * Opaque handle to a setting registered with UGameSettingRegistry.
 *
 * AddSetting and AddCollection return one. Pass it back to RemoveByHandle
 * on teardown. The handle is a process-wide 64-bit counter, cheap to copy
 * and hash. It's stable for the run, not across runs.
 *
 * Value 0 is the invalid / default-constructed sentinel.
 */
struct FGameSettingHandle
{
public:
	FGameSettingHandle() = default;

	/** Allocate a new unique handle. Thread-safe. */
	static UE_API FGameSettingHandle Generate();

	bool IsValid() const { return Value != 0; }
	void Reset() { Value = 0; }

	bool operator==(const FGameSettingHandle& Other) const { return Value == Other.Value; }
	bool operator!=(const FGameSettingHandle& Other) const { return Value != Other.Value; }

	friend uint32 GetTypeHash(const FGameSettingHandle& In) { return ::GetTypeHash(In.Value); }

	UE_API FString ToString() const;

private:
	explicit FGameSettingHandle(uint64 InValue) : Value(InValue) {}

	uint64 Value = 0;
};

#undef UE_API
