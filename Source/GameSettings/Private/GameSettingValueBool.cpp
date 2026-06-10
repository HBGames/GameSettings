// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingValueBool.h"

#include "DataSource/GameSettingDataSource.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingValueBool)

UGameSettingValueBool::UGameSettingValueBool()
{
}

void UGameSettingValueBool::SetDynamicGetter(const TSharedRef<FGameSettingDataSource>& InGetter)
{
	Getter = InGetter;
}

void UGameSettingValueBool::SetDynamicSetter(const TSharedRef<FGameSettingDataSource>& InSetter)
{
	Setter = InSetter;
}

void UGameSettingValueBool::SetDefaultValue(bool InDefault)
{
	DefaultValue = InDefault;
}

bool UGameSettingValueBool::GetBoolValue() const
{
	if (!Getter)
	{
		return false;
	}

	// Data sources are string-typed under the hood; LexFromString handles bool round-trip
	// for the common values UE produces ("true" / "false" / "True" / "False" / "1" / "0").
	const FString Raw = Getter->GetValueAsString(LocalPlayer);
	bool bResult = false;
	LexFromString(bResult, *Raw);
	return bResult;
}

void UGameSettingValueBool::SetBoolValue(bool InValue)
{
	SetBoolValueWithReason(InValue, EGameSettingChangeReason::Change);
}

void UGameSettingValueBool::SetBoolValueWithReason(bool InValue, EGameSettingChangeReason Reason)
{
	check(Setter);
	Setter->SetValue(LocalPlayer, LexToString(InValue));
	NotifySettingChanged(Reason);
}

bool UGameSettingValueBool::IsResettableToDefault() const
{
	if (!DefaultValue.IsSet())
	{
		return false;
	}
	return GetBoolValue() != DefaultValue.GetValue();
}

void UGameSettingValueBool::OnInitialized()
{
	if (!ensureAlways(Getter) || !ensureAlways(Setter))
	{
		return;
	}

#if !UE_BUILD_SHIPPING
	ensureAlwaysMsgf(Getter->Resolve(LocalPlayer),
		TEXT("%s: getter %s did not resolve. Confirm the UFUNCTION exists, takes no parameters, and returns bool."),
		*GetSettingId().ToString(), *Getter->ToString());
	ensureAlwaysMsgf(Setter->Resolve(LocalPlayer),
		TEXT("%s: setter %s did not resolve. Confirm the UFUNCTION exists and takes exactly one bool parameter."),
		*GetSettingId().ToString(), *Setter->ToString());
#endif

	Super::OnInitialized();
}

void UGameSettingValueBool::Startup()
{
	if (!ensureAlways(Getter))
	{
		// Misconfigured (no Getter bound). Complete startup anyway so the
		// registry's IsFinishedInitializing doesn't wait forever on us.
		StartupComplete();
		return;
	}
	Getter->Startup(LocalPlayer, FSimpleDelegate::CreateUObject(this, &ThisClass::OnDataSourcesReady));
}

void UGameSettingValueBool::OnDataSourcesReady()
{
	StartupComplete();
}

void UGameSettingValueBool::StoreInitial()
{
	InitialValue = GetBoolValue();
}

void UGameSettingValueBool::ResetToDefault()
{
	if (DefaultValue.IsSet())
	{
		SetBoolValueWithReason(DefaultValue.GetValue(), EGameSettingChangeReason::ResetToDefault);
	}
}

void UGameSettingValueBool::RestoreToInitial()
{
	SetBoolValueWithReason(InitialValue, EGameSettingChangeReason::RestoreToInitial);
}
