// Copyright Hollywood Camera Work

#include "LineMesh.h"
#include "BezierCalc.h"
#include "LineRendererIncludes.h"
#include "MeshBuild.h"

void ULineMesh::AutoInit()
{
    if (!Bezier.IsValid()) {
        Bezier = MakeShared<FBezierCalc>();
    }

    SetCanEverAffectNavigation(false);
}

void ULineMesh::CreateMesh()
{
    // Initialize mesh arrays to correct sizes for the current mesh, and set up triangles. Every
    // arrowhead adds 3 vertices and 6 triangle vertex indexes.

    const int32 NumPoints = Bezier->Tessellated.Num();
    if (NumPoints >= 2) {
        // Number of vertices and triangles that will be created by the line. We don't create triangles
        // for the last point.
        const int32 NumLineVertices = NumPoints * 2;
        const int32 NumLineTriangles = (NumPoints - 1) * 6;
        
        // Pre-allocate vertices, UVs and triangles.
        LineVertices.SetNum(NumLineVertices);
        LineUvs.SetNum(NumLineVertices );
        LineTriangles.SetNum(NumLineTriangles);

        // Populate triangles. We end at < length-1, because the we reference the following index at
        // each step.
        
        for (int32 i = 0; i < NumPoints - 1; ++i) {
            const int32 VertexBase = i * 2;
            const int32 TriangleBase = i * 6;
            
            LineTriangles[TriangleBase + 0] = VertexBase;
            LineTriangles[TriangleBase + 1] = VertexBase + 1;
            LineTriangles[TriangleBase + 2] = VertexBase + 3;

            LineTriangles[TriangleBase + 3] = VertexBase;
            LineTriangles[TriangleBase + 4] = VertexBase + 3;
            LineTriangles[TriangleBase + 5] = VertexBase + 2;
        }

        CalculateVertexPositions();

        // Add arrowheads. Start at the end of the line vertices and triangles.
        
        AddArrowHeadTriangles(StartArrowMesh, StartArrow);
        AddArrowHeadTriangles(EndArrowMesh, EndArrow);
        CalculateAllArrowHeadVertices();
        
        // UE_LOG(LogTemp, Log, TEXT("Start arrow: %d, End arrow: %d"), StartArrow, EndArrow);
    } else {
        LineVertices.Empty();
        LineTriangles.Empty();
        LineUvs.Empty();
        StartArrowMesh.Vertices.Empty();
        StartArrowMesh.Triangles.Empty();
        StartArrowMesh.Uvs.Empty();
        EndArrowMesh.Vertices.Empty();
        EndArrowMesh.Triangles.Empty();
        EndArrowMesh.Uvs.Empty();
    }
    
    CreateMeshSection_LinearColor(0, LineVertices, LineTriangles, {}, LineUvs, {}, {}, false);
    CreateMeshSection_LinearColor(1, StartArrowMesh.Vertices, StartArrowMesh.Triangles, {}, StartArrowMesh.Uvs, {}, {}, false);
    CreateMeshSection_LinearColor(2, EndArrowMesh.Vertices, EndArrowMesh.Triangles, {}, EndArrowMesh.Uvs, {}, {}, false);
}

void ULineMesh::UpdatePosition()
{
    CalculateVertexPositions();
    CalculateAllArrowHeadVertices();
    UpdateMeshSection_LinearColor(0, LineVertices, {}, LineUvs, {}, {}, false);
    UpdateMeshSection_LinearColor(1, StartArrowMesh.Vertices, {}, StartArrowMesh.Uvs, {}, {}, false);
    UpdateMeshSection_LinearColor(2, EndArrowMesh.Vertices, {}, EndArrowMesh.Uvs, {}, {}, false);
}

void ULineMesh::CalculateVertexPositions()
{
    // Is called both when tessellating and while orienting, but only runs once for each cycle.

    if (Bezier->Points.Num() < 2) {
        return;
    }
    
    if (LastVertexPositionCalculation == DataCycle) {
        return;
    }
    LastVertexPositionCalculation = DataCycle;

    // Bezier->DumpTessellated();
    
    // UE_LOG(LogTemp, Log, TEXT("------------------------------------------------------"));
    
    float TotalDistance = 0;

    for (int32 i = 0; i < Bezier->Tessellated.Num(); ++i) {
        // Load current, previous and next points. Some may be nullptr.
        const FVector& CurPoint = Bezier->Tessellated[i];
        const int32 VertexBase = i * 2;
        const bool IsFirstPoint = (i == 0);
        const bool IsLastPoint = (i == Bezier->Tessellated.Num() - 1);

        FVector PrevPoint = !IsFirstPoint ? Bezier->Tessellated[i - 1] : FVector::ZeroVector;
        FVector NextPoint = !IsLastPoint ? Bezier->Tessellated[i + 1] : FVector::ZeroVector;
        
        // Extend first and last point with fake, linear points.
        
        if (IsFirstPoint) {
            // Beginning point. Make fake, linear previous point.
            PrevPoint = CurPoint - (NextPoint - CurPoint);
        } else if (IsLastPoint) {
            // Ending point. Make fake, linear next point.
            NextPoint = CurPoint + (CurPoint - PrevPoint);
        }

        // UE_LOG(LogTemp, Log, TEXT("Previous: (%f, %f, %f), Current: (%f, %f, %f), Next: (%f, %f, %f)"), PrevPoint.X, PrevPoint.Y, PrevPoint.Z, CurPoint.X, CurPoint.Y, CurPoint.Z, NextPoint.X, NextPoint.Y, NextPoint.Z);

        // Calculate cross-line. First, calculate the direction vectors
        const FVector IncomingDirection = (CurPoint - PrevPoint).GetSafeNormal();
        const FVector OutgoingDirection = (NextPoint - CurPoint).GetSafeNormal();

        // Calculate the average direction
        // const FVector AverageDirection = (IncomingDirection + OutgoingDirection).GetSafeNormal();
        const FVector AverageDirection = (IncomingDirection + OutgoingDirection).GetSafeNormal();

        // Calculate a direction perpendicular to both the average direction and the UpVector
        const FVector PerpendicularDirection = FVector::CrossProduct(UpVector, AverageDirection).GetSafeNormal();
        
        // Amplify line width in sharp corners. AngleDot is the cosine of the angle of the
        // incoming/outgoing lines. We need to transform that into an actual angle (using Acos).
        // Then we divide by 2, because we're interested in a 45 degree cross-line for a 90 degree
        // turn. The Cos then maps it to an effective range of 0 to 1 for bent corners to straight
        // lines. Clamped on the low end to prevent infinitely wide cross-lines in very sharp
        // corners.
        const float AngleDot = FVector::DotProduct(IncomingDirection, OutgoingDirection);
        const float Factor = FMath::Clamp(FMath::Cos(FMath::Acos(AngleDot) / 2), 0.2, 1);
        const float EffectiveLineWidth = LineWidth / Factor;
        // UE_LOG(LogTemp, Log, TEXT("Angledot: %f, Acos(AngleDot): %f, FMath::Sin(FMath::Acos(AngleDot)/2): %f"), AngleDot, FMath::Acos(AngleDot), FMath::Sin(FMath::Acos(AngleDot)/2));

        // Calculate the two points of the cross-line
        const FVector C0 = CurPoint + PerpendicularDirection * (EffectiveLineWidth / 2);
        const FVector C1 = CurPoint - PerpendicularDirection * (EffectiveLineWidth / 2);

        // UE_LOG(LogTemp, Log, TEXT("Cross line (%f, %f, %f) to (%f, %f, %f)"), C0.X, C0.Y, C0.Z, C1.X, C1.Y, C1.Z);

        LineVertices[VertexBase + 0] = C0;
        LineVertices[VertexBase + 1] = C1;

        // Set UVs

        if (!IsFirstPoint) {
            TotalDistance += FVector::Dist(PrevPoint, CurPoint);
        }
        
        // Set UVs. The U-axis of the UVs is left to right on the line, which simply goes 0 to 1.
        // The V-axis is the distance along the line.
        LineUvs[VertexBase] = FVector2D(0, TotalDistance / 100);
        LineUvs[VertexBase + 1] = FVector2D(1, TotalDistance / 100);
    }
}

constexpr int32 ArrowRectLeftIndex = 0;
constexpr int32 ArrowRectRightIndex = 1;
constexpr int32 ArrowBarb1Index = 2;
constexpr int32 ArrowBarb2Index = 3;
constexpr int32 ArrowMiddleIndex = 4;
constexpr int32 ArrowTipIndex = 5;

void ULineMesh::AddArrowHeadTriangles(FMeshParams& ArrowMesh, const bool Active)
{    
    if (Active) {
        ArrowMesh.Vertices.SetNumZeroed(6);
        ArrowMesh.Uvs.SetNumZeroed(6);
        ArrowMesh.Triangles.SetNumZeroed(12);
        
        int32 TIndex = 0;
        ArrowMesh.Triangles[TIndex + 0] = ArrowRectLeftIndex;
        ArrowMesh.Triangles[TIndex + 1] = ArrowTipIndex;
        ArrowMesh.Triangles[TIndex + 2] = ArrowBarb1Index;

        TIndex += 3;
        ArrowMesh.Triangles[TIndex + 0] = ArrowMiddleIndex;
        ArrowMesh.Triangles[TIndex + 1] = ArrowTipIndex;
        ArrowMesh.Triangles[TIndex + 2] = ArrowRectLeftIndex;

        TIndex += 3;
        ArrowMesh.Triangles[TIndex + 0] = ArrowMiddleIndex;
        ArrowMesh.Triangles[TIndex + 1] = ArrowRectRightIndex;
        ArrowMesh.Triangles[TIndex + 2] = ArrowTipIndex;

        TIndex += 3;
        ArrowMesh.Triangles[TIndex + 0] = ArrowRectRightIndex;
        ArrowMesh.Triangles[TIndex + 1] = ArrowBarb2Index;
        ArrowMesh.Triangles[TIndex + 2] = ArrowTipIndex;
    } else {
        ArrowMesh.Vertices.Empty();
        ArrowMesh.Uvs.Empty();
        ArrowMesh.Triangles.Empty();
    }
}

void ULineMesh::CalculateAllArrowHeadVertices()
{
    if (StartArrow) {
        CalculateArrowHeadVertices(StartArrowMesh, 3, 2, 1, 0);
    }

    if (EndArrow) {
        const int32 Last = LineVertices.Num() - 1;
        CalculateArrowHeadVertices(EndArrowMesh, Last - 3, Last - 2, Last - 1, Last);
    }
}

void ULineMesh::CalculateArrowHeadVertices(FMeshParams& ArrowMesh, const int32 N0, const int32 N1, const int32 N2, const int32 N3)
{
    // Input represents the vertex indexes of the rectangle at the end of the line. Resolve the
    // points.
    const FVector& P0 = LineVertices[N0];
    const FVector& P1 = LineVertices[N1];
    const FVector& P2 = LineVertices[N2];
    const FVector& P3 = LineVertices[N3];

    // Get the the middle point suspended between each cross-line.
    const FVector M0 = (P0 + P1) / 2.0f;
    const FVector M1 = (P2 + P3) / 2.0f;
    const FVector2D M1Uv = (LineUvs[N2] + LineUvs[N3]) / 2;
    
    ///// Calculate the barb on each side by extending out the cross-line, and moving it a bit back
    ///// in the direction of the bezier line (the line going through M0 and M1).
    
    // Extract line size and orientation
    const FVector Direction = P3 - P2;
    const float LineSize = Direction.Size();
    const FVector NormalizedDirection = Direction.GetSafeNormal();

    // Size the barb based on the line size and general scale of the arrow.
    const float BarbSizeFactor = 2 * ArrowScale;
    const float BackShiftFactor = 0.75f * ArrowScale;

    // Extend barb out from ending line of rectangle.
    const float ExtensionLength = LineSize * BarbSizeFactor;
    FVector B0 = P2 - NormalizedDirection * ExtensionLength;
    FVector B1 = P3 + NormalizedDirection * ExtensionLength;

    // Calculate the back shift vector
    const FVector BackShiftDirection = (M0 - M1).GetSafeNormal();
    const float BackShiftLength = LineSize * BackShiftFactor;

    // Shift B0 and B1 backwards towards M0. B0 and B1 are now the adjusted points.
    B0 += BackShiftDirection * BackShiftLength;
    B1 += BackShiftDirection * BackShiftLength;
    
    ///// Calculate the tip by extending the line going through M0 and M1. Grab UV from the original
    ///// ending edge in the rectangle.

    const float TipSize = 3 * ArrowScale;
    const FVector TipDirection = (M1 - M0).GetSafeNormal();
    const float ExtPixels = LineSize * TipSize;
    const FVector T0 = M1 + TipDirection * ExtPixels;
    const float UvPolarity = (LineUvs[N2].Y > LineUvs[N0].Y) ? 1 : -1;
    const FVector2D T0Uv((LineUvs[N2].X + LineUvs[N3].X) / 2, LineUvs[N2].Y + ExtPixels * UvPolarity / 100);
    
    ///// Add to mesh. First add barb, middle and tip vertices. The two existing corners of the
    ///// rectangle are at index N2 and N3.
    
    ArrowMesh.Vertices[ArrowRectLeftIndex] = LineVertices[N2];
    ArrowMesh.Uvs[ArrowRectLeftIndex] = LineUvs[N2]; // Copy from rectangle point that barb was extended from.

    ArrowMesh.Vertices[ArrowRectRightIndex] = LineVertices[N3];
    ArrowMesh.Uvs[ArrowRectRightIndex] = LineUvs[N3]; // Copy from rectangle point that barb was extended from.

    ArrowMesh.Vertices[ArrowBarb1Index] = B0;
    ArrowMesh.Uvs[ArrowBarb1Index] = ArrowMesh.Uvs[ArrowRectLeftIndex]; // Copy from rectangle point that barb was extended from.
    
    ArrowMesh.Vertices[ArrowBarb2Index] = B1;
    ArrowMesh.Uvs[ArrowBarb2Index] = ArrowMesh.Uvs[ArrowRectRightIndex]; // Copy from rectangle point that barb was extended from.
    
    ArrowMesh.Vertices[ArrowMiddleIndex] = M1;
    ArrowMesh.Uvs[ArrowMiddleIndex] = M1Uv;
    
    ArrowMesh.Vertices[ArrowTipIndex] = T0;
    ArrowMesh.Uvs[ArrowTipIndex] = T0Uv;
}

void ULineMesh::UpdateMaterial()
{
    // Style map (static for all instances)

    static TMap<ELineRendererStyle, FString> MaterialNames;
    static bool FirstLoad = true;
    if (FirstLoad) {
        FirstLoad = false;
        MaterialNames.Add(ELineRendererStyle::SolidColor, "SolidColor");
        MaterialNames.Add(ELineRendererStyle::RulerStripes, "RulerStripes");
        MaterialNames.Add(ELineRendererStyle::Dashed, "Dashed");
        MaterialNames.Add(ELineRendererStyle::Dotted, "Dotted");
        MaterialNames.Add(ELineRendererStyle::Electricity, "Electricity");
        MaterialNames.Add(ELineRendererStyle::Pulsing, "Pulsing");
    }
    
    // Reset rendering parameters

    bCastCinematicShadow = false;
    bCastContactShadow = false;
    bCastDynamicShadow = false;
    bCastFarShadow = false;
    bCastHiddenShadow = false;
    bCastInsetShadow = false;
    bCastStaticShadow = false;
    bCastVolumetricTranslucentShadow = false;
    // bCastDistanceFieldIndirectShadow = false;
    bAffectDistanceFieldLighting = false;
    bAffectDynamicIndirectLighting = false;
    SetCastShadow(false);
    SetCastContactShadow(false);
    SetCastHiddenShadow(false);
    SetCastInsetShadow(false);
    
    auto GetMaterialInstance = [this](const ELineRendererStyle Style) ->UMaterialInstanceDynamic* {
        const FString* MaterialName = MaterialNames.Find(Style);
        const FString FullPath = TEXT(LINERENDERER_MATERIALS_PATH) + *MaterialName + TEXT(".") + *MaterialName;
        
        UMaterial* LoadedMaterial = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, *FullPath));
        if (LoadedMaterial != nullptr) {
            return UMaterialInstanceDynamic::Create(LoadedMaterial, this);
        } else {
            UE_LOG(LogTemp, Warning, TEXT("Couldn't find material %s"), *FullPath);
            return nullptr;
        }
    };
    
    // Line material

    if (LineStyle != OldLineStyle) {
        OldLineStyle = LineStyle;
        LineMaterialInstance = GetMaterialInstance(LineStyle);
        SetMaterial(0, LineMaterialInstance);
    }

    if (ArrowHeadStyle != OldArrowHeadStyle) {
        OldArrowHeadStyle = ArrowHeadStyle;
        ArrowHeadMaterialInstance = GetMaterialInstance(ArrowHeadStyle);
        SetMaterial(1, ArrowHeadMaterialInstance);
        SetMaterial(2, ArrowHeadMaterialInstance);
   }

    auto SetMaterialParameters = [this](UMaterialInstanceDynamic* MaterialInstance) ->void {
        if (MaterialInstance == nullptr) {
            return;
        }
        MaterialInstance->SetVectorParameterValue(FName("Color1"), Color1);
        MaterialInstance->SetVectorParameterValue(FName("Color2"), Color2);
        MaterialInstance->SetScalarParameterValue(FName("UvDensity"), UvDensity);
        MaterialInstance->SetScalarParameterValue(FName("AnimationSpeed"), AnimationSpeed);
    };

    SetMaterialParameters(LineMaterialInstance);
    SetMaterialParameters(ArrowHeadMaterialInstance);
}

//
// UTILITY
//

void ULineMesh::DrawDebugLines(const TArray<FVector>& WorldPoints) const
{
    if (WorldPoints.Num() < 2) {
        return;
    }
    
    for (int32 i = 0; i < WorldPoints.Num() - 1; ++i) {
        DrawDebugLine(
            GetWorld(),             // World context
            WorldPoints[i],      // Start point
            WorldPoints[i + 1],  // End point
            FColor::Cyan,        // Line color
            true,                // Persistent
            10,                  // Lifetime
            100,                   // Depth priority
            1                    // Line width
        );
    }
}

void ULineMesh::DrawDebugTessellated() const
{
    // This bezier debug drawing is placed here, because we need a World().
    
    if (Bezier->Tessellated.Num() == 0) {
        return;
    }

    FVector LastPoint = FVector::ZeroVector;
    bool First = true;
    UWorld* World = GetWorld();
    
    auto DrawNextDebugPoint = [&First, &LastPoint, World](const FVector& NextPoint) -> void {
        if (!First) {
            DrawDebugLine(
                World,             // World context
                LastPoint,          // Start point
                NextPoint,          // End point
                FColor::Cyan,        // Line color
                true,                // Persistent
                10,                  // Lifetime
                100,                   // Depth priority
                1                    // Line width
            );
        } else {
            First = false;
        }
        LastPoint = NextPoint;
    };

    for (const FVector& Point: Bezier->Tessellated) {
        DrawNextDebugPoint(Point);
    }
}
