// Copyright Hollywood Camera Work

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "LineControlPoint.generated.h"

class UStaticMesh;

UCLASS()
class LINERENDERER_API ULineControlPoint : public UStaticMeshComponent
{
    GENERATED_BODY()

    public: void Init();
    public: void UpdatePosition();
    public: void UpdateMaterial();
    public: void SetPosition(const FVector& Pos);
    
    // PROPERTIES

    public: FVector Position = FVector(0, 0, 0);
    public: float LineWidth = 10;
    public: float ControlPointScale = 2;
    public: FLinearColor ControlPointColor = FLinearColor(1, 1, 1, 1);
    
    private: UPROPERTY()
    UStaticMesh* SphereMesh = nullptr;

    private: UPROPERTY()
    UMaterialInstanceDynamic* MaterialInstance = nullptr;
};
