// Copyright Hollywood Camera Work

#pragma once

#include "CoreMinimal.h"

class LINERENDERER_API FStringUtil
{
    // TEMPLATED UTILITY FUNCTIONS

    public: template<typename EnumType>
    static FString EnumToString(EnumType EnumValue)
    {
        const UEnum* EnumPtr = StaticEnum<EnumType>();
        if (EnumPtr) {
            return EnumPtr->GetNameStringByValue(static_cast<int64>(EnumValue));
        }
        return FString("Unknown Enum");
    }

    // INLINE UTILITY FUNCTIONS

    // STATIC UTILITY FUNCTIONS

    public: static FString Truncate(const FString& InputString, const int32 MaxLength);
    public: static FGuid ParseGuid(const FString& Input);
    public: static FString MaterialPath(const FString& MaterialDirectory, const FString& MaterialName);
};
