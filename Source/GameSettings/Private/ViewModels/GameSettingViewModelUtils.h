// Copyright Hitbox Games, LLC. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Internationalization/Text.h"

inline bool AreFTextArraysEqual(const TArray<FText>& A, const TArray<FText>& B)
{
	if (A.Num() != B.Num())
	{
		return false;
	}
	for (int32 Index = 0; Index < A.Num(); ++Index)
	{
		if (!A[Index].EqualTo(B[Index]))
		{
			return false;
		}
	}
	return true;
}
