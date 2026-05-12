// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Contributions/GameSettingsBindingValueType.h"

#include "UObject/UnrealType.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsBindingValueType)

namespace UE::GameSettings
{
	bool IsPropertyOfValueType(const FProperty* Prop, EGameSettingsBindingValueType Expected)
	{
		if (Expected == EGameSettingsBindingValueType::Unknown)
		{
			return true;
		}
		if (!Prop)
		{
			return false;
		}

		switch (Expected)
		{
		case EGameSettingsBindingValueType::Boolean:
			return Prop->IsA<FBoolProperty>();

		case EGameSettingsBindingValueType::Numeric:
			return Prop->IsA<FFloatProperty>()
				|| Prop->IsA<FDoubleProperty>()
				|| Prop->IsA<FIntProperty>()
				|| Prop->IsA<FInt64Property>()
				|| Prop->IsA<FInt16Property>()
				|| Prop->IsA<FInt8Property>()
				|| Prop->IsA<FUInt32Property>()
				|| Prop->IsA<FUInt64Property>()
				|| Prop->IsA<FUInt16Property>()
				|| Prop->IsA<FByteProperty>();

		case EGameSettingsBindingValueType::Discrete:
			return Prop->IsA<FStrProperty>()
				|| Prop->IsA<FNameProperty>()
				|| Prop->IsA<FEnumProperty>()
				|| Prop->IsA<FByteProperty>()
				|| Prop->IsA<FIntProperty>();

		default:
			return true;
		}
	}
}
