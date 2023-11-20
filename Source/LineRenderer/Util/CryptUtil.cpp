// Copyright Hollywood Camera Work


#include "CryptUtil.h"
#include "HelperUtil.h"

//
// HASHING
//

TArray<uint8> FCryptUtil::MD5HashBytes(const FString& InputString)
{
    FMD5 Md5Gen;

    // Convert the input string to ANSI and update the hash
    Md5Gen.Update(reinterpret_cast<const uint8*>(TCHAR_TO_ANSI(*InputString)), InputString.Len());

    // Finalize the hash and store the result
    uint8 Hash[16];
    Md5Gen.Final(Hash);

    // Convert the hash to a TArray<uint8>
    TArray<uint8> HashBytes;
    HashBytes.Append(Hash, 16);

    return HashBytes;
}

//
// SETTINGS FINGERPRINTING
//

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, int32 Value)
{
    const uint8* Bytes = reinterpret_cast<uint8*>(&Value);
    Array.Append(Bytes, sizeof(int32));
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, int64 Value)
{
    const uint8* Bytes = reinterpret_cast<uint8*>(&Value);
    Array.Append(Bytes, sizeof(int64));
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, const bool Value)
{
    Array.Add(Value ? 1 : 0);
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, float Value)
{
    const int32 IntValue = *reinterpret_cast<int32*>(&Value);
    FingerprintHelper(Array, IntValue);
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, double Value)
{
    const uint8* Bytes = reinterpret_cast<uint8*>(&Value);
    Array.Append(Bytes, sizeof(int64));
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, const FString& Value)
{
    const TArray<uint8> HashedString = FCryptUtil::MD5HashBytes(Value);
    Array.Append(HashedString);
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, const FVector& Position)
{
    FingerprintHelper(Array, Position.X);
    FingerprintHelper(Array, Position.Y);
    FingerprintHelper(Array, Position.Z);
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, const FLinearColor& Color)
{
    FingerprintHelper(Array, Color.R);
    FingerprintHelper(Array, Color.G);
    FingerprintHelper(Array, Color.B);
    FingerprintHelper(Array, Color.A);
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, const FColor& Color)
{
    FingerprintHelper(Array, Color.R);
    FingerprintHelper(Array, Color.G);
    FingerprintHelper(Array, Color.B);
    FingerprintHelper(Array, Color.A);
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, const TArray<float>& FloatValues)
{
    for (const auto& Value: FloatValues) {
        FingerprintHelper(Array, Value);
    }
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, const TArray<FVector>& Positions)
{
    for (const auto& Position: Positions) {
        FingerprintHelper(Array, Position);
    }
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, const TArray<uint8>& Fingerprint)
{
    Array.Append(Fingerprint);
}

void FCryptUtil::FingerprintHelper(TArray<uint8>& Array, const TArray<TArray<uint8>>& Fingerprints)
{
    for (const auto& Fingerprint: Fingerprints) {
        Array.Append(Fingerprint);
    }
}

bool FCryptUtil::FingerprintMatch(const TArray<uint8>& Fingerprint1, const TArray<uint8>& Fingerprint2)
{
    return FHelperUtil::AreArraysIdentical(Fingerprint1, Fingerprint2);
}
