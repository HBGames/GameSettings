// Copyright Hitbox Games, LLC. All Rights Reserved.

#include "GameSettingsEditorFuzzyMatch.h"

namespace UE::GameSettingsEditor::FuzzyMatch
{
	namespace
	{
		const TCHAR* const Prefixes[] = { TEXT("Get"), TEXT("Set"), TEXT("Is"), TEXT("Has"), TEXT("Can"), TEXT("Try") };

		bool IsBoundary(TCHAR Prev, TCHAR Curr)
		{
			if (Prev == TEXT('\0'))
			{
				return true;
			}
			if (!FChar::IsAlnum(Prev) || Prev == TEXT('_'))
			{
				return true;
			}
			// camelCase transition: lower -> upper.
			return FChar::IsLower(Prev) && FChar::IsUpper(Curr);
		}

		int32 SubsequenceBonus(FStringView Candidate, FStringView Target)
		{
			if (Target.IsEmpty() || Candidate.IsEmpty())
			{
				return 0;
			}

			int32 Score = 0;
			int32 ConsecutiveRun = 0;
			int32 TargetIdx = 0;
			TCHAR PrevChar = TEXT('\0');

			for (int32 CandIdx = 0; CandIdx < Candidate.Len() && TargetIdx < Target.Len(); ++CandIdx)
			{
				const TCHAR C = Candidate[CandIdx];
				const TCHAR T = Target[TargetIdx];

				if (FChar::ToLower(C) == FChar::ToLower(T))
				{
					Score += 10;
					ConsecutiveRun++;
					Score += ConsecutiveRun * 2;
					if (IsBoundary(PrevChar, C))
					{
						Score += 20;
					}
					TargetIdx++;
				}
				else
				{
					ConsecutiveRun = 0;
				}

				PrevChar = C;
			}

			// Penalize when the target wasn't fully matched as a subsequence.
			if (TargetIdx < Target.Len())
			{
				Score -= (Target.Len() - TargetIdx) * 30;
			}

			return Score;
		}
	}

	FStringView StripPrefix(FStringView FunctionName)
	{
		for (const TCHAR* Prefix : Prefixes)
		{
			const int32 PrefixLen = FCString::Strlen(Prefix);
			if (FunctionName.Len() > PrefixLen
				&& FunctionName.Left(PrefixLen).Compare(FStringView(Prefix, PrefixLen)) == 0
				&& FChar::IsUpper(FunctionName[PrefixLen]))
			{
				return FunctionName.RightChop(PrefixLen);
			}
		}
		return FunctionName;
	}

	void TokenizeCamelCase(FStringView Input, TArray<FString>& OutTokens)
	{
		OutTokens.Reset();
		if (Input.IsEmpty())
		{
			return;
		}

		FString Current;
		TCHAR Prev = TEXT('\0');

		auto Flush = [&Current, &OutTokens]()
		{
			if (!Current.IsEmpty())
			{
				OutTokens.Add(MoveTemp(Current));
				Current.Reset();
			}
		};

		for (int32 Idx = 0; Idx < Input.Len(); ++Idx)
		{
			const TCHAR C = Input[Idx];
			if (C == TEXT('_') || C == TEXT(' ') || C == TEXT('-'))
			{
				Flush();
				Prev = C;
				continue;
			}
			if (Idx > 0 && FChar::IsLower(Prev) && FChar::IsUpper(C))
			{
				Flush();
			}
			Current.AppendChar(C);
			Prev = C;
		}
		Flush();
	}

	FString NormalizeTargetName(const FString& Input)
	{
		if (Input.IsEmpty())
		{
			return FString();
		}

		// Take the last segment of a dotted tag path.
		FString LastSegment;
		if (!Input.Split(TEXT("."), nullptr, &LastSegment, ESearchCase::IgnoreCase, ESearchDir::FromEnd))
		{
			LastSegment = Input;
		}

		// Strip spaces from human-readable names.
		LastSegment.RemoveSpacesInline();

		return LastSegment;
	}

	int32 Score(FStringView CandidateName, FStringView TargetName)
	{
		if (TargetName.IsEmpty() || CandidateName.IsEmpty())
		{
			return 0;
		}

		const FStringView CandidateCore = StripPrefix(CandidateName);

		// Exact match on the normalized core is overwhelmingly the best signal.
		if (CandidateCore.Len() == TargetName.Len()
			&& CandidateCore.Compare(TargetName, ESearchCase::IgnoreCase) == 0)
		{
			return 1000;
		}

		// Token-set score.
		TArray<FString> CandidateTokens;
		TokenizeCamelCase(CandidateCore, CandidateTokens);

		TArray<FString> TargetTokens;
		TokenizeCamelCase(TargetName, TargetTokens);

		int32 TokenScore = 0;
		int32 UnmatchedTargetTokens = 0;
		for (const FString& TargetToken : TargetTokens)
		{
			bool bExact = false;
			bool bSubstring = false;
			for (const FString& CandidateToken : CandidateTokens)
			{
				if (CandidateToken.Equals(TargetToken, ESearchCase::IgnoreCase))
				{
					bExact = true;
					break;
				}
				if (CandidateToken.Contains(TargetToken, ESearchCase::IgnoreCase))
				{
					bSubstring = true;
				}
			}
			if (bExact)
			{
				TokenScore += 50;
			}
			else if (bSubstring)
			{
				TokenScore += 20;
			}
			else
			{
				UnmatchedTargetTokens++;
			}
		}

		TokenScore -= UnmatchedTargetTokens * 15;

		// fzf-style subsequence over the unstripped name catches partial typos.
		const int32 FuzzyScore = SubsequenceBonus(CandidateName, TargetName);

		return TokenScore + FuzzyScore;
	}
}
