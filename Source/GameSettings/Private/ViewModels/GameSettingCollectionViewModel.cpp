// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "ViewModels/GameSettingCollectionViewModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingCollectionViewModel)

void UGameSettingCollectionViewModel::SetChildViewModels(TArray<UGameSettingViewModel*> InChildren)
{
	ChildViewModels = MoveTemp(InChildren);
	UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GetChildViewModels);
}
