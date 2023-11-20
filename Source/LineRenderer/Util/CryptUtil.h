// Copyright Hollywood Camera Work

#pragma once

#include "CoreMinimal.h"

class LINERENDERER_API FCryptUtil
{
    // HASHING

    public: static TArray<uint8> MD5HashBytes(const FString& InputString);

    // SETTINGS FINGERPRINTING

    private: static void FingerprintHelper(TArray<uint8>& Array, int32 Value);
    private: static void FingerprintHelper(TArray<uint8>& Array, int64 Value);
    private: static void FingerprintHelper(TArray<uint8>& Array, const bool Value);
    private: static void FingerprintHelper(TArray<uint8>& Array, float Value);
    private: static void FingerprintHelper(TArray<uint8>& Array, double Value);
    private: static void FingerprintHelper(TArray<uint8>& Array, const FString& Value);
    private: static void FingerprintHelper(TArray<uint8>& Array, const FVector& Value);
    private: static void FingerprintHelper(TArray<uint8>& Array, const FLinearColor& Color);
    private: static void FingerprintHelper(TArray<uint8>& Array, const FColor& Color);
    private: static void FingerprintHelper(TArray<uint8>& Array, const TArray<float>& FloatValues);
    private: static void FingerprintHelper(TArray<uint8>& Array, const TArray<FVector>& Positions);
    private: static void FingerprintHelper(TArray<uint8>& Array, const TArray<uint8>& Fingerprint);
    private: static void FingerprintHelper(TArray<uint8>& Array, const TArray<TArray<uint8>>& Fingerprints);

    private: template<typename T, typename... Args>
    static void FingerprintInternal(TArray<uint8>& Array, T First, Args... Arguments)
    {
        FingerprintHelper(Array, First);
        FingerprintInternal(Array, Arguments...);
    }
    
    // Fingerprint function definition
    public: template<typename... Args>
    static TArray<uint8> Fingerprint(Args... Arguments)
    {
        TArray<uint8> Result;
        
        ([&Result](const auto& Arg) {
            FingerprintHelper(Result, Arg);
        }(Arguments), ...); // The comma operator is used here to separate calls for each argument

        return MoveTemp(Result);
    }

    public: static bool FingerprintMatch(const TArray<uint8>& Fingerprint1, const TArray<uint8>& Fingerprint2);
};
