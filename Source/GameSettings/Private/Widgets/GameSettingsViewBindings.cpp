// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Widgets/GameSettingsViewBindings.h"

#include "CommonUserWidget.h"
#include "ViewModels/GameSettingViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsViewBindings)

TSubclassOf<UCommonUserWidget> UGameSettingsViewBindings::FindEntryWidget(TSubclassOf<UGameSettingViewModel> ViewModelClass) const
{
	// Walk the class chain inclusive of UGameSettingViewModel itself so a
	// catch-all entry registered against the base type still matches.
	for (UClass* Walk = ViewModelClass.Get();
		 Walk && Walk->IsChildOf(UGameSettingViewModel::StaticClass());
		 Walk = Walk->GetSuperClass())
	{
		if (const TSoftClassPtr<UCommonUserWidget>* Soft = EntryWidgetForViewModel.Find(Walk))
		{
			if (TSubclassOf<UCommonUserWidget> Loaded = Soft->LoadSynchronous())
			{
				return Loaded;
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
