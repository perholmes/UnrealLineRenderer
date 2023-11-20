// Copyright Hollywood Camera Work

#pragma once

#include "CoreMinimal.h"
#include "ProceduralMeshComponent.h"
#include "LineRendererIncludes.h"
#include "LineMesh.generated.h"

class FBezierCalc;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LINERENDERER_API ULineMesh : public UProceduralMeshComponent
{
    GENERATED_BODY()
    
    private: struct FMeshParams
    {
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector2D> Uvs;
    };

    // PUBLIC METHODS

    public: void AutoInit();
    public: void CreateMesh();
    public: void UpdatePosition();
    public: void UpdateMaterial();

    // PRIVATE METHODS
    
    private: void CalculateVertexPositions();

    private: static void AddArrowHeadTriangles(FMeshParams& ArrowMesh, const bool Active);
    private: void CalculateAllArrowHeadVertices();
    private: void CalculateArrowHeadVertices(FMeshParams& ArrowMesh, const int32 N0, const int32 N1, const int32 N2, const int32 N3);

    public: void DrawDebugLines(const TArray<FVector>& WorldPoints) const;
    public: void DrawDebugTessellated() const;

    // PUBLIC PROPERTIES

    public: TSharedPtr<FBezierCalc> Bezier;
    public: FLinearColor Color1 = FLinearColor(0, 0, 0);
    public: FLinearColor Color2 = FLinearColor(0, 0, 0);
    public: float UvDensity = 1;
    public: float AnimationSpeed = 1;
    public: FVector UpVector = FVector(0, 0, 1);
    public: float LineWidth = 10;
    public: ELineRendererStyle LineStyle = ELineRendererStyle::SolidColor;
    public: ELineRendererStyle ArrowHeadStyle = ELineRendererStyle::SolidColor;
    public: bool StartArrow = false;
    public: bool EndArrow = false;
    public: float ArrowScale = 1;
    public: int32 DataCycle = 0;

    // PRIVATE PROPERTIES

    private: int32 LastVertexPositionCalculation = 0;
    
    private: TArray<FVector> LineVertices;
    private: TArray<int32> LineTriangles;
    private: TArray<FVector2D> LineUvs;
    
    FMeshParams StartArrowMesh;
    FMeshParams EndArrowMesh;
    
    private: ELineRendererStyle OldLineStyle = ELineRendererStyle::None;
    private: ELineRendererStyle OldArrowHeadStyle = ELineRendererStyle::None;

    private: UPROPERTY()
    UMaterialInstanceDynamic* LineMaterialInstance = nullptr;

    private: UPROPERTY()
    UMaterialInstanceDynamic* ArrowHeadMaterialInstance = nullptr;

    private: TArray<ELineRendererStyle> ElementStyles;
};
