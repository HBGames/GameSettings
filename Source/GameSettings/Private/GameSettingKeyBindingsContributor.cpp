// Copyright Hitbox Games, LLC. All Rights Reserved.
// The enumeration loop follows Lyra's ULyraGameSettingRegistry::InitializeMouseAndKeyboardSettings (Copyright Epic Games, Inc.).

#include "GameSettingKeyBindingsContributor.h"

#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameSettingCollection.h"
#include "GameSettingRegistry.h"
#include "GameSettingValueKeyBinding.h"
#include "GameSettingsLog.h"
#include "Internationalization/Text.h"
#include "Misc/Crc.h"
#include "UserSettings/EnhancedInputUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GameSettingKeyBindingsContributor)

#define LOCTEXT_NAMESPACE "GameSettings"

namespace UE::GameSettings::KeyBindings
{
	static const FPrimaryAssetType CollectionType = FPrimaryAssetType(TEXT("GameSettingCollection"));
	static const FPrimaryAssetType BindingType = FPrimaryAssetType(TEXT("GameSettingKeyBinding"));

	/** Reduce a source string to an id-safe token while retaining collision resistance. */
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

	/** Prefer localization identity so category ids do not change with culture. */
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

	// The tab that holds every category of bindings. Registered last, and only
	// if it ended up with at least one row, so an empty profile leaves no tab.
	UGameSettingCollection* Root = NewObject<UGameSettingCollection>(&Registry);
	Root->SetSettingId(FPrimaryAssetId(CollectionType, FName(TEXT("KeyBindingsCollection"))));
	Root->SetDisplayName(LOCTEXT("KeyBindingsCollection_Name", "Key Bindings"));

	TMap<FString, UGameSettingCollection*> CategoryToCollection;

	auto GetOrCreateCategory = [&CategoryToCollection, &Registry, Root](FText DisplayCategory) -> UGameSettingCollection*
	{
		if (DisplayCategory.IsEmpty())
		{
			DisplayCategory = LOCTEXT("KeyBindings_DefaultCategory", "General");
		}

		const FString CategoryString = DisplayCategory.ToString();
		if (UGameSettingCollection** Existing = CategoryToCollection.Find(CategoryString))
		{
			return *Existing;
		}

		UGameSettingCollection* Category = NewObject<UGameSettingCollection>(&Registry);
		Category->SetSettingId(FPrimaryAssetId(CollectionType, MakeIdName(TEXT("KeyBindings_Cat_"), GetStableTextIdSource(DisplayCategory))));
		Category->SetDisplayName(DisplayCategory);
		Root->AddSetting(Category);
		CategoryToCollection.Add(CategoryString, Category);
		return Category;
	};

	// Dedup by mapping name so an action that appears in more than one profile
	// gets a single row. The row itself carries every slot for that action.
	TSet<FName> CreatedMappingNames;
	int32 BindingCount = 0;

	for (const TPair<FString, TObjectPtr<UEnhancedPlayerMappableKeyProfile>>& ProfilePair : UserSettings->GetAllAvailableKeyProfiles())
	{
		const UEnhancedPlayerMappableKeyProfile* Profile = ProfilePair.Value;
		if (!Profile)
		{
			continue;
		}

		for (const TPair<FName, FKeyMappingRow>& RowPair : Profile->GetPlayerMappingRows())
		{
			if (!RowPair.Value.HasAnyMappings() || CreatedMappingNames.Contains(RowPair.Key))
			{
				continue;
			}

			// Permissive options match every device, so a keyboard, mouse,
			// gamepad, or VR mapping all pass and land on the same row.
			FPlayerMappableKeyQueryOptions Options;

			const FText DesiredCategory = RowPair.Value.Mappings.begin()->GetDisplayCategory();
			UGameSettingCollection* Category = GetOrCreateCategory(DesiredCategory);

			UGameSettingValueKeyBinding* Binding = NewObject<UGameSettingValueKeyBinding>(&Registry);
			Binding->SetSettingId(FPrimaryAssetId(BindingType, MakeIdName(TEXT("KeyBind_"), RowPair.Key.ToString())));
			Binding->InitializeInputData(Profile, RowPair.Value, Options);
			if (Binding->GetDisplayName().IsEmpty())
			{
				Binding->SetDisplayName(FText::FromName(RowPair.Key));
			}

			Category->AddSetting(Binding);
			CreatedMappingNames.Add(RowPair.Key);
			++BindingCount;
		}
	}

	if (BindingCount == 0)
	{
		// Nothing to bind on this player. Leave the unregistered objects for GC.
		return;
	}

	const FGameSettingHandle Handle = Registry.AddCollection(Root, ParentContainer);
	if (Handle.IsValid())
	{
		OutHandles.Add(Handle);
		UE_LOG(LogGameSettings, Verbose, TEXT("Key bindings contributor added %d binding(s) for %s"),
			BindingCount, *LocalPlayer->GetName());
	}
}

#undef LOCTEXT_NAMESPACE
