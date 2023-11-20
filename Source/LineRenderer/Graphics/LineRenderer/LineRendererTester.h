// Copyright Hollywood Camera Work

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LineRendererTester.generated.h"

class ULineRendererTestWidget;
class ALineRenderer;

UCLASS()
class LINERENDERER_API ALineRendererTester : public AActor
{
    GENERATED_BODY()

    public: ALineRendererTester();
    protected: virtual void BeginPlay() override;
    public: virtual void Tick(const float DeltaTime) override;
    private: void UpdateLineCache();

    private: void InitHitDetectionTest();
    private: void TestHitDetection();
    private: void InitLinearAnimationTest();
    private: void TestLinearMovement();

    // PUBLIC UPROPERTIES
    
    public: UPROPERTY(EditAnywhere, Category = "Line Renderer|Testing", meta=(EditCondition="HitDetectionTest"))
    TSubclassOf<ULineRendererTestWidget> TesterWidgetClass;

    // PRIVATE UPROPERTIES

    private: UPROPERTY()
    ULineRendererTestWidget* TesterWidgetInstance = nullptr;

    private: UPROPERTY()
    UStaticMeshComponent* Sphere = nullptr;

    private: UPROPERTY()
    UStaticMesh* SphereMesh = nullptr;

    private: UPROPERTY()
    TArray<ALineRenderer*> LineRenderers;
    
    // PROPERTIES

    private: bool EnableTesting = false;
    private: FVector2D MiddlePointScreen = FVector2D(0, 0);
    private: float HitMarginPixels = 20;
    private: FVector CameraForward = FVector(0, 0, -1);
    private: FVector OldCameraForward = FVector(0, 0, -1);
    private: FVector CameraLocation = FVector(0, 0, 1);
    private: FVector OldCameraLocation = FVector(0, 0, 1);
    private: bool CameraMoved = false;
    private: double LastCacheUpdate = -1;
};
