// Copyright Hollywood Camera Work

#pragma once

#include "CoreMinimal.h"

class SHOTDESIGNER_API FHelperUtil
{
    public: template<typename T>
    static bool AreArraysIdentical(const TArray<T>& Array1, const TArray<T>& Array2)
    {
        if (Array1.Num() != Array2.Num()) {
            return false;
        }
        for (int32 Index = 0; Index < Array1.Num(); ++Index) {
            if (!(Array1[Index] == Array2[Index]))
            {
                return false;
            }
        }
        return true;
    }
};
