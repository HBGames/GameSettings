// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "Widgets/GameSettingsListView.h"

#include "GameSettingsLog.h"
#include "GameSettingsModule.h"
#include "MVVMSubsystem.h"
#include "View/MVVMView.h"
#include "ViewModels/GameSettingViewModel.h"
#include "Widgets/GameSettingsViewBindings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingsListView)

UUserWidget& UGameSettingsListView::OnGenerateEntryWidgetInternal(UObject* Item,
	TSubclassOf<UUserWidget> DesiredEntryClass,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	UGameSettingViewModel* ItemVM = Cast<UGameSettingViewModel>(Item);
	ensureMsgf(ItemVM, TEXT("UGameSettingsListView received a non-VM item (%s)"),
		Item ? *Item->GetClass()->GetName() : TEXT("null"));

	// Resolve the entry widget class. Check runtime overrides first
	// (pushed by GFP actions, highest priority), then the local Bindings
	// asset, then fall back to DesiredEntryClass.
	TSubclassOf<UUserWidget> EntryClass = DesiredEntryClass;
	const TSubclassOf<UGameSettingViewModel> VMClass = ItemVM ? ItemVM->GetClass() : nullptr;

	bool bResolved = false;
	if (VMClass)
	{
		// Walk the runtime override stack first (highest priority wins).
		for (UGameSettingsViewBindings* Override : FGameSettingsModule::Get().GetActiveViewBindings())
		{
			if (TSubclassOf<UCommonUserWidget> Resolved = Override->FindEntryWidget(VMClass))
			{
				EntryClass = Resolved;
				bResolved = true;
				break;
			}
		}
		// Then fall back to the editor-authored Bindings on this list.
		if (!bResolved && Bindings)
		{
			if (TSubclassOf<UCommonUserWidget> Local = Bindings->FindEntryWidget(VMClass))
			{
				EntryClass = Local;
				bResolved = true;
			}
		}
	}

	if (!bResolved && VMClass)
	{
		UE_LOG(LogGameSettings, Error,
			TEXT("UGameSettingsListView '%s': no entry widget for VM class %s; falling back to DesiredEntryClass."),
			*GetName(), *VMClass->GetName());
	}

	UUserWidget& EntryWidget = GenerateTypedEntry<UUserWidget>(EntryClass, OwnerTable);

	// Wire the entry's MVVM view to point at this row's VM. The entry's BP
	// MVVM panel must declare a viewmodel slot named "Setting" expecting
	// UGameSettingViewModel (or a subclass).
	if (!IsDesignTime() && ItemVM)
	{
		if (UMVVMView* EntryView = UMVVMSubsystem::GetViewFromUserWidget(&EntryWidget))
		{
			TScriptInterface<INotifyFieldValueChanged> AsInterface(ItemVM);
			EntryView->SetViewModel(TEXT("Setting"), AsInterface);
		}
		else
		{
			UE_LOG(LogGameSettings, Warning,
				TEXT("UGameSettingsListView entry '%s' has no MVVM view; install the MVVM extension on the entry widget BP and add a 'Setting' viewmodel slot."),
				*EntryWidget.GetClass()->GetName());
		}
	}

	return EntryWidget;
}
