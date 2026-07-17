// Copyright Hitbox Games, LLC. All Rights Reserved.
// The enumeration loop follows Lyra's ULyraGameSettingRegistry::InitializeMouseAndKeyboardSettings (Copyright Epic Games, Inc.).

#include "GameSettingKeyBindingsContributor.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameSettingCollection.h"
#include "GameSettingKeyBindingSettings.h"
#include "GameSettingRegistry.h"
#include "GameSettingValueKeyBinding.h"
#include "GameSettingsLog.h"
#include "InputAction.h"
#include "Internationalization/Text.h"
#include "Misc/Crc.h"
#include "PlayerMappableKeySettings.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingKeyBindingsContributor)

#define LOCTEXT_NAMESPACE "GameSettings"

namespace UE::GameSettings::KeyBindings
{
	static const FPrimaryAssetType CollectionType = FPrimaryAssetType(TEXT("GameSettingCollection"));
	static const FPrimaryAssetType BindingType = FPrimaryAssetType(TEXT("GameSettingKeyBinding"));

	/** A built row plus its sort keys. */
	struct FPendingRow
	{
		UGameSettingValueKeyBinding* Binding = nullptr;
		FText DisplayName;
		int32 SortOrder = 0;
	};

	/** Rows in one display category. */
	struct FPendingCategory
	{
		FText DisplayText;
		TArray<FPendingRow> Rows;
	};

	/** Stable id token for a source string. */
	static FName MakeIdName(const TCHAR* Prefix, const FString& Source)
	{
		FString Clean;
		Clean.Reserve(Source.Len());
		for (const TCHAR Ch : Source)
		{
			Clean.AppendChar(FChar::IsAlnum(Ch) ? Ch : TEXT('_'));
		}
		return FName(*FString::Printf(TEXT("%s%s_%08X"), Prefix, *Clean, FCrc::StrCrc32(*Source)));
	}

	/** Loc key so category ids stay stable across cultures. */
	static FString GetStableTextIdSource(const FText& Text)
	{
		const TOptional<FString> Namespace = FTextInspector::GetNamespace(Text);
		const TOptional<FString> Key = FTextInspector::GetKey(Text);
		if (Key.IsSet())
		{
			return Namespace.Get(FString()) + TEXT(":") + Key.GetValue();
		}

		return Text.ToString();
	}

	/** SortOrder from the action's mappable key settings, or 0. */
	static int32 ReadSortOrder(const FKeyMappingRow& Row)
	{
		if (Row.Mappings.IsEmpty())
		{
			return 0;
		}
		if (const UInputAction* Action = Row.Mappings.begin()->GetAssociatedInputAction())
		{
			if (const UGameSettingKeyBindingSettings* Settings = Cast<UGameSettingKeyBindingSettings>(Action->GetPlayerMappableKeySettings()))
			{
				return Settings->SortOrder;
			}
		}
		return 0;
	}
}

void UGameSettingKeyBindingsContributor::Apply(UGameSettingRegistry& Registry, TArray<FGameSettingHandle>& OutHandles)
{
	using namespace UE::GameSettings::KeyBindings;

	ULocalPlayer* LocalPlayer = Registry.GetOwningLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	const UEnhancedInputLocalPlayerSubsystem* EISubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	const UEnhancedInputUserSettings* UserSettings = EISubsystem ? EISubsystem->GetUserSettings() : nullptr;
	if (!UserSettings)
	{
		return;
	}

	// Build a row per action, grouped by category. Ordering waits for the full set.
	TMap<FString, FPendingCategory> CategoriesByKey;
	TSet<FName> CreatedMappingNames;
	int32 RowCount = 0;

	for (const TPair<FString, TObjectPtr<UEnhancedPlayerMappableKeyProfile>>& ProfilePair : UserSettings->GetAllAvailableKeyProfiles())
	{
		const UEnhancedPlayerMappableKeyProfile* Profile = ProfilePair.Value;
		if (!Profile)
		{
			continue;
		}

		for (const TPair<FName, FKeyMappingRow>& RowPair : Profile->GetPlayerMappingRows())
		{
			// One row per mapping name, even across profiles.
			if (!RowPair.Value.HasAnyMappings() || CreatedMappingNames.Contains(RowPair.Key))
			{
				continue;
			}

			// Default options match every device.
			FPlayerMappableKeyQueryOptions Options;

			UGameSettingValueKeyBinding* Binding = NewObject<UGameSettingValueKeyBinding>(&Registry);
			Binding->SetSettingId(FPrimaryAssetId(BindingType, MakeIdName(TEXT("KeyBind_"), RowPair.Key.ToString())));
			Binding->InitializeInputData(Profile, RowPair.Value, Options);
			if (Binding->GetDisplayName().IsEmpty())
			{
				Binding->SetDisplayName(FText::FromName(RowPair.Key));
			}

			FText CategoryText = RowPair.Value.Mappings.begin()->GetDisplayCategory();
			if (CategoryText.IsEmpty())
			{
				CategoryText = LOCTEXT("KeyBindings_DefaultCategory", "General");
			}

			FPendingCategory& Category = CategoriesByKey.FindOrAdd(CategoryText.ToString());
			Category.DisplayText = CategoryText;
			Category.Rows.Add({ Binding, Binding->GetDisplayName(), ReadSortOrder(RowPair.Value) });

			CreatedMappingNames.Add(RowPair.Key);
			++RowCount;
		}
	}

	if (CategoriesByKey.IsEmpty())
	{
		// Nothing to bind. The unregistered objects fall to GC.
		return;
	}

	// Order categories and rows, then bake it into SortPriority so the registry
	// reproduces it. Order is fixed to the current culture and refreshes on Regenerate.
	TArray<FPendingCategory*> SortedCategories;
	SortedCategories.Reserve(CategoriesByKey.Num());
	for (TPair<FString, FPendingCategory>& Pair : CategoriesByKey)
	{
		SortedCategories.Add(&Pair.Value);
	}
	// Sort dereferences the pointers before calling the predicate.
	SortedCategories.Sort([](const FPendingCategory& A, const FPendingCategory& B)
	{
		return A.DisplayText.CompareTo(B.DisplayText) < 0;
	});

	TArray<UGameSettingCollection*> Categories;
	Categories.Reserve(SortedCategories.Num());
	int32 CategoryPriority = 0;
	for (FPendingCategory* Pending : SortedCategories)
	{
		UGameSettingCollection* Category = NewObject<UGameSettingCollection>(&Registry);
		Category->SetSettingId(FPrimaryAssetId(CollectionType, MakeIdName(TEXT("KeyBindings_Cat_"), GetStableTextIdSource(Pending->DisplayText))));
		Category->SetDisplayName(Pending->DisplayText);
		Category->SetSortPriority(CategoryPriority);
		CategoryPriority += 10;

		Pending->Rows.StableSort([](const FPendingRow& A, const FPendingRow& B)
		{
			if (A.SortOrder != B.SortOrder)
			{
				return A.SortOrder < B.SortOrder;
			}
			return A.DisplayName.CompareTo(B.DisplayName) < 0;
		});

		int32 RowPriority = 0;
		for (const FPendingRow& Row : Pending->Rows)
		{
			Row.Binding->SetSortPriority(RowPriority);
			RowPriority += 10;
			Category->AddSetting(Row.Binding);
		}

		Categories.Add(Category);
	}

	// With a parent, nest the categories under it. Otherwise build the fallback tab.
	if (ParentContainer.IsValid())
	{
		for (UGameSettingCollection* Category : Categories)
		{
			const FGameSettingHandle Handle = Registry.AddCollection(Category, ParentContainer);
			if (Handle.IsValid())
			{
				OutHandles.Add(Handle);
			}
		}
	}
	else
	{
		UGameSettingCollection* Root = NewObject<UGameSettingCollection>(&Registry);
		Root->SetSettingId(FPrimaryAssetId(CollectionType, FName(TEXT("KeyBindingsCollection"))));
		Root->SetDisplayName(LOCTEXT("KeyBindingsCollection_Name", "Key Bindings"));
		Root->SetSortPriority(FallbackTabSortPriority);
		for (UGameSettingCollection* Category : Categories)
		{
			Root->AddSetting(Category);
		}

		const FGameSettingHandle Handle = Registry.AddCollection(Root, FPrimaryAssetId());
		if (Handle.IsValid())
		{
			OutHandles.Add(Handle);
		}
	}

	UE_LOG(LogGameSettings, Verbose, TEXT("Key bindings contributor added %d row(s) in %d categories for %s"),
		RowCount, Categories.Num(), *LocalPlayer->GetName());
}

#undef LOCTEXT_NAMESPACE
