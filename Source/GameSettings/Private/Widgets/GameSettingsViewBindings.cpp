// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Widgets/GameSettingsViewBindings.h"

#include "CommonUserWidget.h"
#include "ViewModels/GameSettingViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsViewBindings)

void UGameSettingsViewBindings::ResolveEntryWidgets() const
{
	if (bEntryWidgetsResolved)
	{
		return;
	}
	bEntryWidgetsResolved = true;

	ResolvedEntryWidgets.Reserve(EntryWidgetForViewModel.Num());
	for (const TPair<TSubclassOf<UGameSettingViewModel>, TSoftClassPtr<UCommonUserWidget>>& Pair : EntryWidgetForViewModel)
	{
		if (UClass* VMClass = Pair.Key.Get())
		{
			ResolvedEntryWidgets.Add(VMClass, Pair.Value.LoadSynchronous());
		}
	}
}

#if WITH_EDITOR
void UGameSettingsViewBindings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Authored bindings changed; rebuild the resolved cache on next consult.
	ResolvedEntryWidgets.Reset();
	bEntryWidgetsResolved = false;
}
#endif

TSubclassOf<UCommonUserWidget> UGameSettingsViewBindings::FindEntryWidget(TSubclassOf<UGameSettingViewModel> ViewModelClass) const
{
	ResolveEntryWidgets();

	// Walk the class chain inclusive of UGameSettingViewModel itself so a
	// catch-all entry registered against the base type still matches.
	for (UClass* Walk = ViewModelClass.Get();
		 Walk && Walk->IsChildOf(UGameSettingViewModel::StaticClass());
		 Walk = Walk->GetSuperClass())
	{
		if (const TSubclassOf<UCommonUserWidget>* Resolved = ResolvedEntryWidgets.Find(Walk))
		{
			// A null value means the authored soft class failed to load; fall
			// through to a base-class match, same as the lazy-load behavior.
			if (*Resolved)
			{
				return *Resolved;
			}
		}
	}
	return nullptr;
}

TArray<TSoftClassPtr<UCommonUserWidget>> UGameSettingsViewBindings::GatherDetailExtensions(TSubclassOf<UGameSettingViewModel> ViewModelClass) const
{
	TArray<TSoftClassPtr<UCommonUserWidget>> Out;

	for (UClass* Walk = ViewModelClass.Get();
		 Walk && Walk->IsChildOf(UGameSettingViewModel::StaticClass());
		 Walk = Walk->GetSuperClass())
	{
		if (const FGameSettingsDetailExtensionList* List = DetailExtensions.Find(Walk))
		{
			Out.Append(List->Extensions);
		}
	}
	return Out;
}
