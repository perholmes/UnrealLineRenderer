// Copyright Hollywood Camera Work

#include "BezierCalc.h"

void FBezierCalc::Calculate()
{
    // Calculate Tangents
    
    if (HardCorners) {
        // These are straight line segments. Copy them directly in.
        Tessellated.SetNum(Points.Num());
        SegmentTessIndexes.SetNumZeroed(Points.Num());
        SegmentLengths.SetNumZeroed(Points.Num());

        for (int i = 0; i < Points.Num(); ++i) {
            Tessellated[i] = Points[i];
            SegmentTessIndexes[i] = i;

            if (i < Points.Num() - 1) {
                SegmentLengths[i] = (Points[i + 1] - Points[i]).Size();
            } else {
                SegmentLengths[i] = 0; // Last point is not a segment.
            }
        }
    } else {
        // Soft line. Calculate auto tangents.
        CalculateTangents();
        CalculateBezier();
    }
}

void FBezierCalc::CalculateTangents()
{
    // Ensure the tangents arrays are empty and then set to the correct size
    OutTangents.Empty(Points.Num());
    OutTangents.AddZeroed(Points.Num());
    InTangents.Empty(Points.Num());
    InTangents.AddZeroed(Points.Num());

    // Calculate for the middle points' incoming and outgoing tangents
    for (int32 i = 1; i < Points.Num() - 1; ++i) {
        FVector PrevPoint = Points[i - 1];
        FVector CurrPoint = Points[i];
        FVector NextPoint = Points[i + 1];

        // Calculate normalized direction vectors for segments
        FVector DirToPrev = (CurrPoint - PrevPoint).GetSafeNormal();
        FVector DirToNext = (NextPoint - CurrPoint).GetSafeNormal();

        // The tangent direction is the normalized average of the vectors to the previous and next points
        FVector TangentDir = (DirToPrev + DirToNext).GetSafeNormal();

        // Calculate the distances from the current point to the previous and next points
        float DistToPrev = (CurrPoint - PrevPoint).Size();
        float DistToNext = (NextPoint - CurrPoint).Size();

        // The outgoing tangent has the same direction as the average direction
        OutTangents[i] = TangentDir * TangentStrength * DistToNext;

        // The incoming tangent has the inverted direction
        InTangents[i] = TangentDir * TangentStrength * DistToPrev;
    }

    if (Points.Num() > 1) {
        FVector SecondPoint = Points[1];
        FVector FirstPoint = Points[0];
        // Calculate the target point for the outgoing tangent of the first point
        // It's the second point's position minus its incoming tangent (which points towards the first point)
        FVector TargetPointForTangent = SecondPoint - InTangents[1];
        FVector DirToNextIncomingTangent = (TargetPointForTangent - FirstPoint).GetSafeNormal();
        float DistToNext = (SecondPoint - FirstPoint).Size();
        OutTangents[0] = DirToNextIncomingTangent * TangentStrength * DistToNext;
    }

    // Calculate for the last point's incoming tangent, which should now point towards the outgoing tangent of the second-to-last point
    if (Points.Num() > 2) {
        FVector SecondToLastPoint = Points[Points.Num() - 2];
        FVector LastPoint = Points.Last();
        // The direction should be from the last point towards the position that is the second-to-last point plus its outgoing tangent
        FVector TargetPointForTangent = SecondToLastPoint + OutTangents[Points.Num() - 2]; // This gives us the target point in world space
        FVector DirFromSecondToLastOutgoingTangent = (TargetPointForTangent - LastPoint).GetSafeNormal();
        float DistToLast = (LastPoint - SecondToLastPoint).Size();
        InTangents.Last() = -DirFromSecondToLastOutgoingTangent * TangentStrength * DistToLast;
    }
}

void FBezierCalc::CalculateBezier()
{
    Tessellated.Empty();
    SegmentTessIndexes.SetNumZeroed(Points.Num());
    SegmentLengths.SetNumZeroed(Points.Num());
    SegmentStartLengths.SetNumZeroed(Points.Num());
    
    if (Points.Num() < 2) {
        return;
    }

    TotalLength = 0;
    
    for (int32 i = 0; i < Points.Num(); ++i) {
        SegmentTessIndexes[i] = Tessellated.Num();
        float SegmentLength = 0;

        if (i < Points.Num() - 1) {
            FVector P0 = Points[i];
            FVector P1 = Points[i + 1];

            TessellateSegment(i, 0, P0, 1, P1, Tessellated); // Recursive

            // Add distance to next point to complete the segment.
            // SegmentLength += FVector::Dist(Tessellated.Last(), P1);

            SegmentLength = 0;
            for (int32 j = SegmentTessIndexes[i]; j < Tessellated.Num() - 1; ++j) {
                SegmentLength += FVector::Dist(Tessellated[j], Tessellated[j + 1]);
            }
            SegmentLength += FVector::Dist(Tessellated.Last(), P1);
        } else {
            Tessellated.Add(Points[i]); // Last point is not a full segment
        }
        
        SegmentLengths[i] = SegmentLength;
        SegmentStartLengths[i] = TotalLength;
        TotalLength += SegmentLength;
    }
    
    // UE_LOG(LogTemp, Log, TEXT("Total length after calculate bezier: %f"), TotalLength);
    // for (int32 i = 0; i < Points.Num(); ++i) {
    //     UE_LOG(LogTemp, Log, TEXT("Segment %d tess index: %d"), i, SegmentTessIndexes[i]);
    // }
    // for (int32 i = 0; i < Points.Num(); ++i) {
    //     UE_LOG(LogTemp, Log, TEXT("Segment %d length: %f"), i, SegmentLengths[i]);
    // }
}

void FBezierCalc::TessellateSegment(const int32 SegmentIndex, const float T0, const FVector& P0, const float T1, const FVector& P1, TArray<FVector>& TesPoints)
{
    constexpr float NearPoint = 0.2f;
    constexpr float FarPoint = 0.8f;
    const FVector CurvedMidPoint = CalculateBezierPoint(SegmentIndex, FMath::Lerp(T0, T1, 0.5f));
    const float EffectiveQuality = FMath::Lerp(50.0, 0.01, (TessellationQuality));

    // Compare the curved samples against linear samples to see if the deviation is too great and we
    // need to tessellate this segment. This is crammed into a convoluted if statement to benefit
    // from short-circuiting. The decision can almost always be made just with the center point, but
    // there are some cases where the center point is exactly equal to the middle of a very curved
    // line, so we have to sample also the near and far points.
    
    if (
        (CurvedMidPoint - FMath::Lerp(P0, P1, 0.5f)).Size() > EffectiveQuality ||
        (CalculateBezierPoint(SegmentIndex, FMath::Lerp(T0, T1, NearPoint)) - FMath::Lerp(P0, P1, NearPoint)).Size() > EffectiveQuality ||
        (CalculateBezierPoint(SegmentIndex, FMath::Lerp(T0, T1, FarPoint)) - FMath::Lerp(P0, P1, FarPoint)).Size() > EffectiveQuality
    ) {
        TessellateSegment(SegmentIndex, T0, P0, (T0 + T1) * 0.5f, CurvedMidPoint, TesPoints);
        TessellateSegment(SegmentIndex, (T0 + T1) * 0.5f, CurvedMidPoint, T1, P1, TesPoints);
    } else {
        // Doesn't need any more tessellation.
        // auto AddPoint = [&TesPoints](const FVector& CurPoint) ->void {
        //     if (TesPoints.Num() != 0) {
        //         const FVector& PrevPoint = TesPoints.Last();
        //     }
        //     TesPoints.Add(CurPoint);
        // };

        TesPoints.Add(P0);
        TesPoints.Add(CurvedMidPoint);

        // AddPoint(P0);
        // AddPoint(CurvedMidPoint);
    }
}

FVector FBezierCalc::CalculateBezierPoint(float FloatProgress)
{
    // UE_LOG(LogTemp, Log, TEXT("CalculateBezierPoint"));

    if (FloatProgress >= Points.Num() - 1) {
        return Points.Last();
    }
    
    FloatProgress = FMath::Clamp(FloatProgress, 0, Points.Num());
    int32 Segment = 0;
    float Progress = 0;
    DecomposeFloatProgress(FloatProgress, Segment, Progress);

    // Ensure the segment index is within the bounds of the Points array
    if (Segment < 0 || Segment >= Points.Num() - 1) {
        UE_LOG(LogTemp, Warning, TEXT("Segment index out of bounds"));
        return FVector::ZeroVector; // Return a default value to avoid crashing
    }

    // Get the control points for this segment
    const FVector P0 = Points[Segment];
    const FVector P1 = P0 + OutTangents[Segment];
    const FVector P2 = Points[Segment + 1] - InTangents[Segment + 1];
    const FVector P3 = Points[Segment + 1];

    // Calculate the cubic Bezier point at SegmentProgress
    const float T = Progress;
    const float OneMinusT = 1.0f - T;
    const FVector Point = OneMinusT * OneMinusT * OneMinusT * P0 + 
                    3.0f * OneMinusT * OneMinusT * T * P1 + 
                    3.0f * OneMinusT * T * T * P2 + 
                    T * T * T * P3;

    return Point;
}

FVector FBezierCalc::CalculateBezierPoint(const int32 Segment, const float Progress)
{
    const float FloatProgress = FMath::Clamp(Segment, 0, Points.Num()) + FMath::Clamp(Progress, 0, 1);
    return CalculateBezierPoint(FloatProgress);
}

void FBezierCalc::DecomposeFloatProgress(float FloatProgress, int32& Segment, float& Progress) const
{
    // Splits a float progress like 1.7 into Segment = (int) 1, Progress = (float) 0.7
    FloatProgress = FMath::Clamp(FloatProgress, 0, Points.Num());
    float SegmentFloat = 0;
    Progress = FMath::Modf(FloatProgress, &SegmentFloat);
    Segment = static_cast<int32>(SegmentFloat);
}

FVector FBezierCalc::SlopeAtPoint(float FloatProgress)
{
    // UE_LOG(LogTemp, Log, TEXT("SlopeAtPoint"));

    const float FloatMax = Points.Num();
    FloatProgress = FMath::Clamp(FloatProgress, 0, FloatMax);
    constexpr float Margin = 0.01;
    
    // Get a left and a right that are plus/minus 0.01. Clamp that to the allowed range from 0 to
    // Segment+1. Then ensure that there's still a gap of Margin*2 between them.

    const float Left = FMath::Clamp(FMath::Clamp(FloatProgress - Margin, 0, FloatMax), 0, FloatMax - 2 * Margin);
    const float Right = FMath::Clamp(FMath::Clamp(FloatProgress + Margin, 0, FloatMax), 2 * Margin, FloatMax);
    
    const FVector P0 = CalculateBezierPoint(Left);
    const FVector P1 = CalculateBezierPoint(Right);
    const FVector Slope = (P1 - P0).GetSafeNormal();
    return Slope;
}

FVector FBezierCalc::PerpendicularAtPoint(float FloatProgress, const FVector& UpVector)
{
    // UE_LOG(LogTemp, Log, TEXT("PerpendicularAtPoint"));
    
    FloatProgress = FMath::Clamp(FloatProgress, 0, Points.Num() - 1);
    const FVector Slope = SlopeAtPoint(FloatProgress);
    const FVector Perpendicular = FVector::CrossProduct(Slope, UpVector).GetSafeNormal();
    return Perpendicular;
}

FVector FBezierCalc::CalculateLinearPoint(const float Progress)
{
    // Only works when bezier is calculated. First early exits.

    if (Points.Num() == 0) {
        return FVector::ZeroVector;
    } else if (Points.Num() == 1) {
        return Points[0];
    }

    if (Progress <= 0) {
        return Points[0];
    } else if (Progress >= 1) {
        return Points[Points.Num() - 1];
    }

    const float PathLength = FMath::Lerp(0, TotalLength, Progress);

    for (int32 Segment = 0; Segment < Points.Num() - 1; ++Segment) {
        // Get left and right length bounds for this segment.
        const float LeftLength = SegmentStartLengths[Segment];
        const float RightLength = SegmentStartLengths[Segment + 1];

        // If we're within this segment, find our progress within it.
        if (LeftLength <= PathLength && PathLength < RightLength) {
            if (LeftLength != RightLength) {
                return CalculateBezierPoint(Segment, (PathLength - LeftLength) / (RightLength - LeftLength));
            }

            // We shouldn't ever be here, because if the segment has no length, we wouldn't be in
            // the outer if in the first place.
            UE_LOG(LogTemp, Warning, TEXT("Cannot calculate linear position for zero-length segment. Should be impossible."));
            return CalculateBezierPoint(Segment, 0);
        }
    }

    // If nothing found, there's an error in the underlying data. Log an error and return first
    // point.

    UE_LOG(LogTemp, Warning, TEXT("Cannot calculate linear position for zero-length segment. Should be impossible."));
    return CalculateBezierPoint(0, 0);
}

//
// HIT DETECTION
//

FHitDetectionResult FBezierCalc::HitDetectPoints(const APlayerController* Player, const FVector2D& HitPos)
{
    FVector2D Projected(0, 0);
    FHitDetectionResult Result;
    
    for (int32 i = 0; i < Points.Num(); ++i) {
        const FVector& Point = Points[i];
        
        const bool IsOnScreen = Player->ProjectWorldLocationToScreen(Point, Projected, false);
        const float Distance = FVector2D::Distance(HitPos, Projected);
        
        if (IsOnScreen && Distance < Result.Distance) {
            // Result.IsOnScreen = true;
            Result.Segment = i;
            Result.Distance = Distance;
            Result.Valid = true;
        }
    }

    return MoveTemp(Result);
}

FHitDetectionResult FBezierCalc::HitDetectSpline(const APlayerController* Player, const FVector2D& HitPos)
{
    TArray<FVector2D> ScreenLinePoints;

    // Convert line to screen coordinates
    
    for (const FVector& TessPoint : Tessellated) {
        FVector2D Projected;
        Player->ProjectWorldLocationToScreen(TessPoint, Projected, false);
        ScreenLinePoints.Add(Projected);
    }

    // Find line fragment with closest match
    
    FHitDetectionResult Result;
    int32 Segment = 0;
    float ProgressLength = 0;
    
    for (int32 i = 0; i < Tessellated.Num() - 1; ++i) {
        while (i >= SegmentTessIndexes[Segment + 1]) {
            ++Segment;
            ProgressLength = 0;
        }
        
        // Prepare 2D values
        const FVector2D& FromScreenPoint = ScreenLinePoints[i];
        const FVector2D& ToScreenPoint = ScreenLinePoints[i + 1];
        const FVector2D ScreenLineVector = ToScreenPoint - FromScreenPoint;
        const FVector2D HitPointVector = HitPos - FromScreenPoint;

        // Prepare 3D values
        const FVector& FromTessPoint = Tessellated[i];
        const FVector& ToTessPoint = Tessellated[i + 1];
        const float FragmentLength = (ToTessPoint - FromTessPoint).Size();
        // UE_LOG(LogTemp, Log, TEXT("Fragment %d length %f"), i, FragmentLength);
        
        // Project PointVector onto LineVector
        const float LineLengthSquared = ScreenLineVector.SizeSquared();
        const float Projection = FVector2D::DotProduct(HitPointVector, ScreenLineVector) / LineLengthSquared;

        // Clamp the projection value to the range [0, 1] to stay on the line segment
        const float FragmentProgress = FMath::Clamp(Projection, 0.0f, 1.0f);

        // Find the closest point on the line
        const FVector2D ClosestPoint = FromScreenPoint + ScreenLineVector * FragmentProgress;

        // Calculate distance and position on line
        const float DistanceToPoint = (HitPos - ClosestPoint).Size();

        if (DistanceToPoint < Result.Distance) {
            const float LengthAlongLine = ProgressLength + FragmentProgress * FragmentLength;
            Result.Progress = LengthAlongLine / SegmentLengths[Segment];
            Result.Distance = DistanceToPoint;
            Result.Segment = Segment;
            Result.Valid = true;

            if (Result.Progress >= 1) {
                Result.Progress -= 1;
                Result.Segment += 1;
            }
        }

        ProgressLength += FragmentLength;
        // UE_LOG(LogTemp, Log, TEXT("Segment: %d, ProgressLength: %f, SegmentLength: %f"), Segment, ProgressLength, SegmentLengths[Segment]);
    }
    
    return MoveTemp(Result);
}

//
// UTILITY
//

void FBezierCalc::DumpTessellated() const
{
    UE_LOG(LogTemp, Log, TEXT("*************** DUMP TESSELLATED ***************"));

    if (Tessellated.Num() == 0)
        return;

    FVector PreviousPoint = FVector::ZeroVector;
    float TotalDist = 0;
    
    for (int i = 0; i < Tessellated.Num(); ++i) {
        const FVector& CurrentPoint = Tessellated[i];
        const float Distance = (i == 0) ? 0 : FVector::Dist(PreviousPoint, CurrentPoint);
        TotalDist += Distance;

        UE_LOG(LogTemp, Log, TEXT("Point %d: (%s), Distance from Previous: %f"), i, *CurrentPoint.ToString(), Distance);

        PreviousPoint = CurrentPoint;
    }

    UE_LOG(LogTemp, Log, TEXT("Total length: %f, Total tess points: %d"), TotalDist, Tessellated.Num());
}
