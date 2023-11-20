// Copyright Hollywood Camera Work

#pragma once

#include <tuple>

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "LineRendererIncludes.h"
#include "LineRendererActor.generated.h"

class FBezierCalc;
class ULineMesh;
class ULineControlPoint;

// STRUCTS

USTRUCT(BlueprintType)
struct FSideLine
{
    GENERATED_BODY()

    // These From/To/Start/End properties should ideally be private, but that precludes them being
    // edited in blueprint. Don't read these raw values, as the logic doesn't support inverted
    // lines. Instead, use GetFromTo() and GetArrows(), which order them correctly.
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float FromFloatProgress = 0.25;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ToFloatProgress = 0.75;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool StartArrow = false;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool EndArrow = false;

    // KeepClearVector represents the direction a camera would point at the very start. Causes the
    // sidelines to prefer to be rendered on the opposite side of the line.
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector NotionalCameraVector = FVector(0, 1, 0);

    // Additional unsaved properties. Must be initialized on every usage.
    // TArray<float> ProgressPoints;
    // bool EffectiveStartArrow = false;
    // bool EffectiveEndArrow = false;
    public: int32 Side = 1;
    public: int32 Level = 0;

    public: std::tuple<float, float> GetFromTo() const;
    public: std::tuple<bool, bool> GetArrows() const;
};

UCLASS()
class LINERENDERER_API ALineRenderer : public AActor
{
    GENERATED_BODY()

    // ENUMS
    
    enum class EPhases {
        Start,
        Calculation,
        CreateMesh,
        Position,
        Material,
        End,
    };

    enum class ELineElements {
        MainLine,
        StartArrow,
        EndArrow,
        SideLines,
        TheEnd, // Used to get number of enums
    };

    // METHODS
    
    public: ALineRenderer();
    
    protected: virtual void BeginPlay() override;
    private: void Init();

    public: virtual void Tick(const float DeltaTime) override;
    
#if WITH_EDITOR
    public: virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
    public: virtual void PostLoad() override;
#endif

    // FORWARDERS
    
    public: FVector CalculateLinearPoint(float Progress) const;
    public: FVector CalculateBezierPoint(float Progress) const;
    public: FVector CalculateBezierPoint(int32 Segment, float Progress) const;
    public: FHitDetectionResult HitDetectPoints(const APlayerController* Player, const FVector2D& HitPos) const;
    public: FHitDetectionResult HitDetectSpline(const APlayerController* Player, const FVector2D& HitPos) const;

    // INTERNAL METHODS
    
    private: void CreateLineMesh(const bool ShouldExist);
    private: void SetSideLineMeshQuantity(int32 Desired);
    private: void SetControlPointQuantity(int32 Desired);
    private: void ChangeDetection(const bool Force = false);
    private: void CalculateLineFundamentals() const;
    private: void CreateMesh() const;
    private: void UpdatePosition();
    private: void UpdateMaterials();
    private: void CalculateSideLines();
    private: void UpdateSidelineMaterials();
    private: void ResetDebugLines() const;

    // PUBLIC UPROPERTIES

    // Bezier Section
    
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Bezier")
    TArray<FVector> Points;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Bezier")
    bool HardCorners = true;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Bezier")
    float LineWidth = 10;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Bezier", meta=(EditCondition="!HardCorners", ClampMin = "0.5", ClampMax = "0.99"))
    float TessellationQuality = 0.95;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Bezier", meta=(EditCondition="!HardCorners"))
    float TangentStrength = 0.3; // In fraction of a segment. Must not be greater than 0.5.

    // Appearance Section
    
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Main Appearance")
    FLinearColor LineBodyColor = FLinearColor(0, 0.03, 0.6, 1);
    
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Main Appearance")
    ELineRendererStyle LineStyle = ELineRendererStyle::SolidColor;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Main Appearance")
    ELineRendererStyle ArrowHeadStyle = ELineRendererStyle::SolidColor;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Main Appearance")
    float UvDensity = 1;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Main Appearance")
    float AnimationSpeed = 1;

    // Arrow Heads

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Arrowhead Appearance")
    bool StartArrow = false;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Arrowhead Appearance")
    bool EndArrow = false;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Arrowhead Appearance")
    FLinearColor ArrowheadColor = FLinearColor(0, 0.24, 0.54, 1);
    
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Arrowhead Appearance", meta=(EditCondition="StartArrow || EndArrow"))
    float ArrowScale = 1;

    // Control points

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Control Point Appearance")
    bool ShowControlPoints = true;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Control Point Appearance", meta=(EditCondition="ShowControlPoints"))
    FLinearColor ControlPointColor = FLinearColor(0, 0, 0, 1);
    
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Control Point Appearance", meta=(EditCondition="ShowControlPoints"))
    float ControlPointScale = 1;

    // Sidelines Section

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Sidelines")
    bool ShowSideLines = true;

    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Sidelines")
    TArray<FSideLine> SideLines;
    
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Sidelines")
    FLinearColor SideLineColor;
    
    // Orientation Section
    
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Orientation")
    bool CameraFacing = false;
    
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Line Renderer|Orientation", meta=(EditCondition="!CameraFacing"))
    FVector UpVector = FVector(0, 0, 1);

    // PRIVATE UPROPERTIES

    public: UPROPERTY()
    ULineMesh* LineMesh;

    public: UPROPERTY()
    TArray<ULineMesh*> SideLineMeshes;

    public: UPROPERTY()
    TArray<ULineControlPoint*> ControlPoints;

    // PRIVATE PROPERTIES
    
    private: TArray<uint8> LineFingerprint;
    private: TArray<uint8> TessellationFingerprint;
    private: TArray<uint8> PositionFingerprint;
    private: TArray<uint8> MaterialFingerprint;
    private: FVector CameraForward = FVector(0, 0, -1);
    private: FVector OldCameraForward = FVector(0, 0, -1);
    private: FVector CameraLocation = FVector(0, 0, 1);
    private: FVector OldCameraLocation = FVector(0, 0, 1);
    private: FVector EffectiveUpVector = FVector(0, 0, 1);
};
