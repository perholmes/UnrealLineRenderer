// Copyright Hollywood Camera Work

#include "LineRendererActor.h"
#include "CryptUtil.h"
#include "LineMesh.h"
#include "LineControlPoint.h"
#include "BezierCalc.h"
#include "Util/MathUtil.h"

#ifndef ETOINT
#define ETOINT(EnumValue) static_cast<int32>(static_cast<std::underlying_type<decltype(EnumValue)>::type>(EnumValue))
#endif

ALineRenderer::ALineRenderer()
{
    PrimaryActorTick.bCanEverTick = true;
    LineMesh = nullptr;
}

void ALineRenderer::BeginPlay()
{
    Super::BeginPlay();
    Init();
}

void ALineRenderer::Init()
{
    CreateLineMesh(false);
    SetSideLineMeshQuantity(0);
    SetControlPointQuantity(0);
    ChangeDetection(true);
}

void ALineRenderer::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CameraFacing) {
        const APlayerCameraManager *CamManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
        CameraLocation = CamManager->GetCameraLocation();
        CameraForward  = CamManager->GetCameraRotation().Vector();

        const float Dist = FVector::Dist(CameraLocation, OldCameraLocation);
        const float Dot = FVector::DotProduct(CameraForward, OldCameraForward);

        // If camera has moved or reoriented enough
        if (Dist > 1 || Dot < 0.999) {
            OldCameraLocation = CameraLocation;
            OldCameraForward = CameraForward;
            ChangeDetection();
        }
    }
    
    // UE_LOG(LogTemp, Log, TEXT("Camera location: %f,%f,%f. Forward: %f,%f,%f"), CamLocation.X, CamLocation.Y, CamLocation.Z, CamForward.X, CamForward.Y, CamForward.Z);
}

void ALineRenderer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    ChangeDetection();
}

void ALineRenderer::PostLoad()
{
    Super::PostLoad();
    // Init();
    // ChangeDetection();
}

//
// SUBOBJECT LIFE-CYCLE
//

void ALineRenderer::CreateLineMesh(const bool ShouldExist)
{
    if (LineMesh == nullptr && ShouldExist) {
        LineMesh = NewObject<ULineMesh>(this, ULineMesh::StaticClass(), TEXT("LineMesh"));
        LineMesh->RegisterComponent();
        LineMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
    }

    if (LineMesh != nullptr && !ShouldExist) {
        LineMesh->DestroyComponent();
        LineMesh = nullptr;
    }
}

void ALineRenderer::SetSideLineMeshQuantity(const int32 Desired)
{
    Lr_RemoveNullPointers(SideLineMeshes); // Hot reload protection
    
    while (SideLineMeshes.Num() < Desired) {
        // if (GetOwner() == nullptr) {
        //     UE_LOG(LogTemp, Warning, TEXT("Skipping SetSideLineMeshQuantity. No owner."));
        //     break;
        // }

        ULineMesh* Mesh = NewObject<ULineMesh>(this, ULineMesh::StaticClass(), *FString::Printf(TEXT("SideLine_%d"), SideLineMeshes.Num()));
        Mesh->RegisterComponent();
        Mesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        SideLineMeshes.Add(Mesh);
    }

    while (SideLineMeshes.Num() > Desired) {
        ULineMesh* Mesh = SideLineMeshes.Pop(true);
        Mesh->DestroyComponent();
    }
}

void ALineRenderer::SetControlPointQuantity(const int32 Desired)
{
    Lr_RemoveNullPointers(ControlPoints); // Hot reload protection

    // UE_LOG(LogTemp, Log, TEXT("Setting control point quantity to %d"), Desired);
    
    while (ControlPoints.Num() < Desired) {
        ULineControlPoint* ControlPoint = NewObject<ULineControlPoint>(this, ULineControlPoint::StaticClass(), *FString::Printf(TEXT("CP_%d"), ControlPoints.Num()));
        ControlPoint->RegisterComponent();
        ControlPoint->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
        ControlPoints.Add(ControlPoint);
        ControlPoint->Init();
    }

    while (ControlPoints.Num() > Desired) {
        ULineControlPoint* ControlPoint = ControlPoints.Pop(true);
        ControlPoint->DestroyComponent();
    }
}

//
// CHANGE DETECTION
//

void ALineRenderer::ChangeDetection(const bool Force)
{
    // Call change detection when done manipulating input parameters. While in the editor, this is
    // called automatically when parameters are changed in the details panel.
    
    CreateLineMesh(true);
    SetControlPointQuantity(ShowControlPoints ? Points.Num() : 0);
    SetSideLineMeshQuantity(ShowSideLines ? SideLines.Num() : 0);

    // New data cycle for all subobjects
    
    LineMesh->AutoInit();
    LineMesh->DataCycle++;

    for (const auto& Mesh: SideLineMeshes) {
        Mesh->AutoInit();
        Mesh->DataCycle++;
    }

    // ResetDebugLines();

    EffectiveUpVector = CameraFacing ? (-CameraForward) : UpVector;
    
    // Do change detection backwards, and start at the latest phase needed.
    
    EPhases StartPhase = Force ? EPhases::Start : EPhases::End;

    // Material change detection

    if (ETOINT(StartPhase) > ETOINT(EPhases::Material)) {
        TArray<uint8> Fingerprint = FCryptUtil::Fingerprint(
            LineBodyColor,
            ArrowheadColor,
            SideLineColor,
            static_cast<int32>(LineStyle),
            static_cast<int32>(ArrowHeadStyle),
            UvDensity,
            AnimationSpeed,
            ControlPointColor
        );
        if (!FCryptUtil::FingerprintMatch(Fingerprint, MaterialFingerprint)) {
            MaterialFingerprint = MoveTemp(Fingerprint);
            StartPhase = EPhases::Material;
        }
    }

    // Position/orientation change detection

    if (ETOINT(StartPhase) > ETOINT(EPhases::Position)) {
        TArray<uint8> Fingerprint = FCryptUtil::Fingerprint(
            CameraFacing,
            EffectiveUpVector,
            Points,
            ShowControlPoints,
            ControlPointScale, LineWidth // LineWidth is used by LineControlPoint
        );
        if (!FCryptUtil::FingerprintMatch(Fingerprint, PositionFingerprint)) {
            PositionFingerprint = MoveTemp(Fingerprint);
            StartPhase = EPhases::Position;
        }
    }

    // Create mesh change detection

    if (ETOINT(StartPhase) > ETOINT(EPhases::CreateMesh)) {
        TArray<uint8> Fingerprint = FCryptUtil::Fingerprint(
            LineWidth,
            StartArrow,
            EndArrow,
            ArrowScale
        );
        if (!FCryptUtil::FingerprintMatch(Fingerprint, TessellationFingerprint)) {
            TessellationFingerprint = MoveTemp(Fingerprint);
            StartPhase = EPhases::CreateMesh;
        }
    }

    // Points change detection (and tangent config)

    if (ETOINT(StartPhase) > ETOINT(EPhases::Calculation)) {
        // Pre-fingerprint the SideLine structs, to avoid having to read a FSideLine struct in
        // FCryptUtil.
        TArray<TArray<uint8>> SideLineFingerprints;
        for (const FSideLine& SideLine : SideLines) {
            auto [SideLineFrom, SideLineTo] = SideLine.GetFromTo();
            auto [SideLineArrowStart, SideLineEndArrow] = SideLine.GetArrows();
            SideLineFingerprints.Add(FCryptUtil::Fingerprint(
                SideLineFrom,
                SideLineTo,
                SideLine.NotionalCameraVector,
                SideLineArrowStart,
                SideLineEndArrow
            ));
        }
        
        TArray<uint8> Fingerprint = FCryptUtil::Fingerprint(
            Points,
            SideLineFingerprints,
            ShowSideLines,
            HardCorners,
            TessellationQuality,
            TangentStrength
        );
        if (!FCryptUtil::FingerprintMatch(Fingerprint, LineFingerprint)) {
            LineFingerprint = MoveTemp(Fingerprint);
            StartPhase = EPhases::Calculation;
        }
    }

    // Execute phases

    if (ETOINT(EPhases::Calculation) >= ETOINT(StartPhase)) {
        CalculateLineFundamentals();
    }
    
    if (ETOINT(EPhases::CreateMesh) >= ETOINT(StartPhase)) {
        CreateMesh();
    }
    
    if (ETOINT(EPhases::Position) >= ETOINT(StartPhase)) {
        UpdatePosition();
    }
    
    if (ETOINT(EPhases::Material) >= ETOINT(StartPhase)) {
        UpdateMaterials();
    }
}

void ALineRenderer::CalculateLineFundamentals() const
{
    // UE_LOG(LogTemp, Log, TEXT("Calculate Line Fundamentals"));

    // This needs to be upgraded so that sideline meshes receive offset and sectional points.
    LineMesh->Bezier->Points = Points;
    LineMesh->Bezier->HardCorners = HardCorners;
    LineMesh->Bezier->TangentStrength = TangentStrength;
    LineMesh->Bezier->TessellationQuality = TessellationQuality;
    
    // Calculation will have to be based on calculating the first line and then deriving the sidelines.
    LineMesh->Bezier->Calculate();
    
    // LineMesh->Bezier->DumpTessellated();
    // Mesh->DrawDebugTessellated();
}

void ALineRenderer::CreateMesh() const
{
    // UE_LOG(LogTemp, Log, TEXT("Create Mesh"));

    LineMesh->UpVector = EffectiveUpVector; // Also needed for tessellation because orientations are calculated.
    LineMesh->LineWidth = LineWidth;
    LineMesh->StartArrow = StartArrow;
    LineMesh->EndArrow = EndArrow;
    LineMesh->ArrowScale = ArrowScale;
    LineMesh->CreateMesh();
}

void ALineRenderer::UpdatePosition()
{
    // UE_LOG(LogTemp, Log, TEXT("Position/orientation"));

    LineMesh->UpVector = EffectiveUpVector;
    LineMesh->UpdatePosition();

    CalculateSideLines();
    
    if (Points.Num() == ControlPoints.Num()) {
        for (int32 i = 0; i < Points.Num(); ++i) {
            const auto& ControlPoint = ControlPoints[i];
            ControlPoint->Position = Points[i];
            ControlPoint->ControlPointScale = ControlPointScale;
            ControlPoint->UpdatePosition();
        }
    }
}

void ALineRenderer::UpdateMaterials()
{
    // UE_LOG(LogTemp, Log, TEXT("Material"));

    LineMesh->LineStyle = LineStyle;
    LineMesh->ArrowHeadStyle = ArrowHeadStyle;
    LineMesh->Color1 = LineBodyColor;
    LineMesh->Color2 = ArrowheadColor;
    LineMesh->UvDensity = UvDensity;
    LineMesh->AnimationSpeed = AnimationSpeed;
    LineMesh->UpdateMaterial();

    for (const auto& ControlPoint: ControlPoints) {
        ControlPoint->ControlPointColor = ControlPointColor;
        ControlPoint->UpdateMaterial();
    }
    
    UpdateSidelineMaterials();
}

//
// SIDELINES. These are Shot Designer-specific movement arrows that are rendered right next to the
// main path to illustrate moving a camera back and forth on the same line. Sidelines are always
// completely calculated during the Position/Orientation update, because orienting them towards the
// camera involves sloping them differently against the main path depending on view angle.
//

void ALineRenderer::CalculateSideLines()
{
    if (LineMesh == nullptr) {
        UE_LOG(LogTemp, Log, TEXT("No main line mesh. Cannot draw sidelines"));
        return;
    }

    // UE_LOG(LogTemp, Log, TEXT("-------------------------------------------------"));

    // Sidelines are only meant to be drawn in the camera diagram or when we're looking from above.
    // Check if the camera is physically over any point on the main bezier (that the angle from a
    // point up to the camera is close to world up).
    bool AboveLine = false;
    for (const FVector& Point: Points) {
        const float Dot = FVector::DotProduct((CameraLocation - Point).GetSafeNormal(), FVector::UpVector);
        if (Dot > 0.7) {
            AboveLine = true;
            break;
        }
    }
    if (!(ShowSideLines && AboveLine)) {
        SetSideLineMeshQuantity(0);
        return;
    }

    // Sidelines are always drawn to be seen from above.
    const FVector SideLineUpVector = FVector(0, 0, 1);
    
    SetSideLineMeshQuantity(SideLines.Num());
    FBezierCalc& Bezier = *LineMesh->Bezier;
    
    TArray<FSideLine*> Stacking;

    for (int i = 0; i < SideLines.Num(); ++i) {
        FSideLine& SideLine = SideLines[i];
        ULineMesh* SideLineMesh = SideLineMeshes[i];

        auto [SideLineFrom, SideLineTo] = SideLine.GetFromTo();
        auto [SideLineStartArrow, SideLineEndArrow] = SideLine.GetArrows();
        
        // Determine side. Get the standard perpendicular direction for the main bezier at this
        // point. This will point to the left side of the line if looking along the line.
        FVector StartPerpendicular = Bezier.PerpendicularAtPoint(SideLineFrom, SideLineUpVector);
        float Side = 1;
        
        // Get dot-product of perpendicular vector and notional (assumed) camera angle. If greater
        // than 0, they're pointing towards the same side, and we need to invert the side in order
        // to try to get sidelines on the opposite sides that cameras are pointing.
        if (FVector::DotProduct(StartPerpendicular, SideLine.NotionalCameraVector) > 0) {
            Side = -1;
        }

        // Scan placed sidelines, and see if we can find a level where we can place this one. Any
        // time we encounter a sideline with the same or lower level than us, set level to one above
        // that level. At the end, we'll know which level is clear to place the current sideline on.
        int32 Level = 0;

        for (int j = 0; j < i; ++j) {
            const FSideLine& CheckLine = SideLines[j];

            if (CheckLine.Side != Side) {
                // Ignore any lines not on the same side
                continue;
            }

            constexpr float Margin = 0.05;

            auto [CheckLineFrom, CheckLineTo] = CheckLine.GetFromTo();

            if (SideLineTo + Margin <= CheckLineFrom || 
                CheckLineTo <= SideLineFrom - Margin) {
                // Ranges do not overlap, continue with the loop
                continue;
            }
            
            if (CheckLine.Level <= Level) {
                // Move up to at least one above the tested level
                Level = FMath::Max(Level, CheckLine.Level + 1);
            }
        }

        SideLine.Side = Side;
        SideLine.Level = Level;

        // UE_LOG(LogTemp, Log, TEXT("From: %f, To: %f, Side: %d, Level: %d"), SideLineFrom, SideLineTo, SideLine.Side, SideLine.Level);

        // Draw the sideline at the current Side and Level. Make an array of progress points that
        // should get a control point, starting with the beginning and ending point, and any whole
        // segment point in-between.

        TArray<float> Progresses;
        const float SnapFrom = FMath::Clamp(FMathUtil::SnapToWholeNumber(SideLineFrom, 0.02f), 0, Points.Num());
        const float SnapTo = FMath::Clamp(FMathUtil::SnapToWholeNumber(SideLineTo, 0.02f), 0, Points.Num());
        
        Progresses.Add(SnapFrom);

        const int32 WholeFrom = FMath::CeilToInt(SnapFrom + 0.000001);
        const int32 WholeTo = FMath::FloorToInt(SnapTo - 0.000001);

        for (int32 j = WholeFrom; j >= WholeFrom && j <= WholeTo; ++j) {
            Progresses.Add(j);
        }

        Progresses.Add(SnapTo);

        // for (const auto& Progress: Progresses) {
        //     UE_LOG(LogTemp, Log, TEXT("Raw Progress: %f"), Progress);
        // }

        // Subdivide long spans of progress points, in order to ensure that the sideline reasonably
        // follows the slope of the main line, even if it didn't start and end where the main line
        // did and doesn't have the same tangents.

        constexpr float MaxSpan = 0.1;
        TArray<float> Subdivided;

        for (int j = 0; j < Progresses.Num() - 1; ++j) {
            const float& Current = Progresses[j];
            const float& Next = Progresses[j + 1];

            Subdivided.Add(Current);

            const float Span = Next - Current;
            if (Span > MaxSpan) {
                const int NumPoints = static_cast<int>(Span / MaxSpan);
                const float Step = Span / (NumPoints + 1);

                for (int t = 1; t <= NumPoints; ++t) {
                    Subdivided.Add(Current + t * Step);
                }
            }
        }

        Subdivided.Add(Progresses.Last());

        // for (const float Sub: Subdivided) {
        //     UE_LOG(LogTemp, Log, TEXT("Subdivided: %f"), Sub);
        // }

        // Create the points extended out from chosen side, the chosen distance. Discard points that
        // are too close to the previous point, in order to prevent folding in sharp corners.

        constexpr float Avoidance = 4; // Points can't be closer than this.
        bool First = true;
        FVector PrevPoint = FVector::ZeroVector;
        TArray<FVector> FinalPoints;

        for (const float Sub: Subdivided) {
            // Calculate perpendicular point for this progress point.
            FVector CurvePoint = Bezier.CalculateBezierPoint(Sub);
            FVector Perpendicular = Bezier.PerpendicularAtPoint(Sub, SideLineUpVector);

            // Perpendicular vectors can flip, so if the vector is more than 90 degrees wrong for
            // the UpVector, we flip it.
            // const float Flip = (FVector::DotProduct(Perpendicular, SideLineUpVector) < 0) ? -1 : 1;
            constexpr float Flip = 1;
            
            // Calculate the point
            FVector Point = CurvePoint + Perpendicular * SideLine.Side * Flip * (20 + 15 * SideLine.Level);
            if (!First && (Point - PrevPoint).Size() < Avoidance) {
                continue;
            }

            FinalPoints.Add(Point);
            
            First = false;
            PrevPoint = Point;
        }
        
        // for (const auto& Point: FinalPoints) {
        //     UE_LOG(LogTemp, Log, TEXT("Point: %f,%f,%f"), Point.X, Point.Y, Point.Z);
        // }

        // Transfer points to line mesh

        SideLineMesh->Bezier->Points.SetNum(FinalPoints.Num());
        for (int32 t = 0; t < FinalPoints.Num(); ++t) {
            SideLineMesh->Bezier->Points[t] = FinalPoints[t];
        }

        SideLineMesh->LineWidth = 1.5;
        SideLineMesh->ArrowScale = 1;
        SideLineMesh->UpVector = EffectiveUpVector;
        SideLineMesh->StartArrow = SideLineStartArrow;
        SideLineMesh->EndArrow = SideLineEndArrow;
        SideLineMesh->Bezier->TessellationQuality = 0.98;
        SideLineMesh->Bezier->HardCorners = false;
        
        SideLineMesh->Bezier->Calculate();
        SideLineMesh->CreateMesh();
    }
}

void ALineRenderer::UpdateSidelineMaterials()
{
    for (const auto& SideLineMesh: SideLineMeshes) {
        SideLineMesh->Color1 = FLinearColor(0, 0, 0, 1);
        SideLineMesh->UpdateMaterial();
    }
}

std::tuple<float, float> FSideLine::GetFromTo() const
{
    // Observe that this is a method in FSideLine.
    if (FromFloatProgress <= ToFloatProgress) {
        return std::make_tuple(FromFloatProgress, ToFloatProgress);
    } else {
        return std::make_tuple(ToFloatProgress, FromFloatProgress);
    }
}

std::tuple<bool, bool> FSideLine::GetArrows() const
{
    // Observe that this is a method in FSideLine.
    if (FromFloatProgress <= ToFloatProgress) {
        return std::make_tuple(StartArrow, EndArrow);
    } else {
        return std::make_tuple(EndArrow, StartArrow);
    }
}

//
// FORWARDERS
//

FVector ALineRenderer::CalculateLinearPoint(const float Progress) const
{
    // Forwarder to get linear position on bezier. Suitable for animation, because speed will be
    // constant even with segments of varying lengths. Remember to call ChangeDetection() first.
    
    if (LineMesh != nullptr && LineMesh->Bezier.IsValid()) {
        return LineMesh->Bezier->CalculateLinearPoint(Progress);
    } else {
        return FVector::ZeroVector;
    }
}

FVector ALineRenderer::CalculateBezierPoint(const float Progress) const
{
    // Forwarder to get position on bezier based purely on segment + progress (e.g. Progress 1.7 for
    // Segment 1 Progress 0.7). Not suitable for animation, as it doesn't compensate for segments of
    // different lengths. Remember to call ChangeDetection() first.
    
    if (LineMesh != nullptr && LineMesh->Bezier.IsValid()) {
        return LineMesh->Bezier->CalculateBezierPoint(Progress);
    } else {
        return FVector::ZeroVector;
    }
}

FVector ALineRenderer::CalculateBezierPoint(const int32 Segment, const float Progress) const
{
    // Forwarder to get position on bezier based purely on segment and progress as separate values.
    // Not suitable for animation, as it doesn't compensate for segments of different lengths.
    // Remember to call ChangeDetection() first.
    
    if (LineMesh != nullptr && LineMesh->Bezier.IsValid()) {
        return LineMesh->Bezier->CalculateBezierPoint(Segment, Progress);
    } else {
        return FVector::ZeroVector;
    }
}

FHitDetectionResult ALineRenderer::HitDetectPoints(const APlayerController* Player, const FVector2D& HitPos) const
{
    if (LineMesh != nullptr && LineMesh->Bezier.IsValid()) {
        return LineMesh->Bezier->HitDetectPoints(Player, HitPos);
    } else {
        return FHitDetectionResult{};
    }
}

FHitDetectionResult ALineRenderer::HitDetectSpline(const APlayerController* Player, const FVector2D& HitPos) const
{
    if (LineMesh != nullptr && LineMesh->Bezier.IsValid()) {
        return LineMesh->Bezier->HitDetectSpline(Player, HitPos);
    } else {
        return FHitDetectionResult{};
    }
}

//
// UTILITY
//

void ALineRenderer::ResetDebugLines() const
{
    FlushPersistentDebugLines(GetWorld());
}
