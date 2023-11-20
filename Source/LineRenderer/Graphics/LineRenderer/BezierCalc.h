// Copyright Hollywood Camera Work

#pragma once

#include <tuple>
#include <limits>

#include "LineRendererIncludes.h"

#include "CoreMinimal.h"

class LINERENDERER_API FBezierCalc
{
	// METHODS

	public: void Calculate();
	private: void CalculateTangents();
	private: void CalculateBezier();
	private: void TessellateSegment(const int32 SegmentIndex, const float T0, const FVector& P0, const float T1, const FVector& P1, TArray<FVector>& TesPoints);
	public: FVector CalculateBezierPoint(const int32 Segment, const float Progress);
	public: FVector CalculateBezierPoint(float FloatProgress);
	public: void DecomposeFloatProgress(float FloatProgress, int32& Segment, float& Progress) const;
	public: FVector SlopeAtPoint(float FloatProgress);
	public: FVector PerpendicularAtPoint(const float FloatProgress, const FVector& UpVector);
	public: FVector CalculateLinearPoint(float Progress);
	public: FHitDetectionResult HitDetectPoints(const APlayerController* Player, const FVector2D& HitPos);
	public: FHitDetectionResult HitDetectSpline(const APlayerController* Player, const FVector2D& HitPos);
	public: void DumpTessellated() const;

	// PROPERTIES

	// Raw points and settings copied from outside.
	public: TArray<FVector> Points;
	public: bool HardCorners = false;
	public: float TangentStrength = 0.3; // In fraction of a segment. Must not be greater than 0.5.
	public: float TessellationQuality = 0.95;

	// DERIVED

	// Tangents are automatically created for a smooth line through the points.
	private: TArray<FVector> InTangents;
	private: TArray<FVector> OutTangents;
	// Tessellated points go into a single array. SegmentIndexes are where segments start in this
	// array. Segment lengths the length of each segment.
	public: TArray<FVector> Tessellated;
	public: TArray<int32> SegmentTessIndexes;
	public: TArray<float> SegmentLengths;
	public: TArray<float> SegmentStartLengths;
	public: float TotalLength = 0;
	
	// PRIVATE PROPERTIES
};
