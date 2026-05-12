// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace UE::GameSettingsEditor::FuzzyMatch
{
	/**
	 * Score a candidate function name against a target setting name. Higher
	 * is better. Designed for ranking UFUNCTIONs in the binding combo box.
	 *
	 * Hybrid algorithm:
	 *   - Token-set match after stripping Get/Set/Is/Has/Can prefixes
	 *     (strong signal: shared meaningful tokens after camelCase split).
	 *   - fzf-style subsequence with camelCase boundary bonus (weak signal:
	 *     typos and partial names) used as tiebreaker.
	 *
	 * If the target is empty, returns 0 for every candidate.
	 */
	int32 Score(FStringView CandidateName, FStringView TargetName);

	/**
	 * Strip a leading Get/Set/Is/Has/Can/Try prefix from a function name.
	 * Returns the remainder; if no prefix matches, returns the input unchanged.
	 */
	FStringView StripPrefix(FStringView FunctionName);

	/**
	 * Split a CamelCase / PascalCase string into its component tokens.
	 * "GetResolutionScale" -> { "Get", "Resolution", "Scale" }.
	 * Underscores act as separators too: "set_voice_volume" -> { "set", "voice", "volume" }.
	 */
	void TokenizeCamelCase(FStringView Input, TArray<FString>& OutTokens);

	/**
	 * Pull a "core" setting name from a tag path or DisplayName.
	 * "Settings.Video.ResolutionScale" -> "ResolutionScale".
	 * "Resolution Scale" -> "ResolutionScale".
	 */
	FString NormalizeTargetName(const FString& Input);
}
