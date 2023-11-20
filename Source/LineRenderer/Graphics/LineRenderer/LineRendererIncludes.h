// Copyright Hollywood Camera Work

#pragma once

#include "CoreMinimal.h"

#define LINERENDERER_MATERIALS_PATH "/Game/Graphics/LineRenderer/"

// ENUMS (DETAIL PANEL)

UENUM(BlueprintType)
enum class ELineRendererStyle : uint8
{
    None UMETA(Hidden),
    SolidColor UMETA(DisplayName = "Solid Color"),
    RulerStripes UMETA(DisplayName = "Ruler Stripes"),
    Dashed UMETA(DisplayName = "Dashed"),
    Dotted UMETA(DisplayName = "Dotted"),
    Electricity UMETA(DisplayName = "Electricity"),
    Pulsing UMETA(DisplayName = "Pulsing"),
    TheEnd UMETA(Hidden), // Used to get number of enums
};

struct FHitDetectionResult
{
    bool Valid = false;
    int32 Segment = 0;
    float Progress = 0; // Only applicable to HitDetectSpline
    float Distance = std::numeric_limits<float>::max();
};

template<typename T>
void Lr_RemoveNullPointers(TArray<T*>& Array)
{
    Array.RemoveAll([](T* Ptr) { return Ptr == nullptr; });
}
