// Copyright Hollywood Camera Work


#include "StringUtil.h"

FString FStringUtil::Truncate(const FString& InputString, const int32 MaxLength)
{
    if (InputString.Len() <= MaxLength) {
        return InputString;
    }

    return InputString.Left(MaxLength);
}

FGuid FStringUtil::ParseGuid(const FString& Input)
{
    FGuid Guid;
    if (FGuid::Parse(Input, Guid)) {
        return Guid;
    }
    return FGuid();
}

FString FStringUtil::MaterialPath(const FString& MaterialDirectory, const FString& MaterialName)
{
    FString FullPath = MaterialDirectory + MaterialName + TEXT(".") + MaterialName;
    return MoveTemp(FullPath);
}
