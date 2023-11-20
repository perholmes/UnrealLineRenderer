// Copyright Hollywood Camera Work

#include "LineRendererTester.h"

#include "BezierCalc.h"
#include "Blueprint/UserWidget.h"
#include "Runtime/Engine/Classes/Engine/UserInterfaceSettings.h"
#include "EngineUtils.h"
#include "LineMesh.h"

#include "LineRendererTestWidget.h"
#include "LineRendererIncludes.h"
#include "LineRendererActor.h"

ALineRendererTester::ALineRendererTester()
{
    PrimaryActorTick.bCanEverTick = true;

    // Enable here to run testing
    EnableTesting = true;
}

void ALineRendererTester::BeginPlay()
{
    Super::BeginPlay();
    
    if (!EnableTesting) {
        return;
    }
    
    InitLinearAnimationTest();
    InitHitDetectionTest();
}

void ALineRendererTester::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);
    
    if (!EnableTesting) {
        return;
    }

    UpdateLineCache();

    // Detect camera movement

    CameraMoved = false;

    const APlayerCameraManager *CamManager = GetWorld()->GetFirstPlayerController()->PlayerCameraManager;
    CameraLocation = CamManager->GetCameraLocation();
    CameraForward  = CamManager->GetCameraRotation().Vector();

    const float Dist = FVector::Dist(CameraLocation, OldCameraLocation);
    const float Dot = FVector::DotProduct(CameraForward, OldCameraForward);

    if (Dist > 1 || Dot < 0.99999) {
        OldCameraLocation = CameraLocation;
        OldCameraForward = CameraForward;
        CameraMoved = true;
    }

    // Run testers

    TestLinearMovement();

    if (CameraMoved) {
        TestHitDetection();
    }
}

//
// LINE CACHE
//

void ALineRendererTester::UpdateLineCache()
{
    // When hit detecting, we iterate this cache of LineRenderers detected from the scene. This is
    // inefficient, and if these line renderers are created programatically (and you already have
    // their pointers), it would be more efficient to iterate that list directly.
    
    const double CurrentTime = FPlatformTime::Seconds();
    if (CurrentTime < LastCacheUpdate + 0.5) {
        return;
    }
    LastCacheUpdate = CurrentTime;

    // Re-cache line renderers
    
    LineRenderers.Empty();
    const UWorld* World = GetWorld();

    for (TActorIterator<ALineRenderer> It(World); It; ++It) {
        ALineRenderer* LineRenderer = *It;
        if (IsValid(LineRenderer)) {
            LineRenderers.Add(LineRenderer);
        }
    }

    // UE_LOG(LogTemp, Log, TEXT("Found %d line renderers"), LineRenderers.Num());
}

//
// HIT DETECTION TEST
//

void ALineRendererTester::InitHitDetectionTest()
{
    UE_LOG(LogTemp, Log, TEXT("Initializing tester"));
    
    if (!TesterWidgetInstance && TesterWidgetClass && GetWorld()) {
        // Instantiate a widget blueprint with crosshairs for trying hit detection.
        UE_LOG(LogTemp, Log, TEXT("Instantiate a widget blueprint with crosshairs for trying hit detection"));
        APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
        if (PlayerController) {
            UE_LOG(LogTemp, Log, TEXT("Has playercontroller"));
            TesterWidgetInstance = CreateWidget<ULineRendererTestWidget>(PlayerController, TesterWidgetClass);

            if (TesterWidgetInstance) {
                TesterWidgetInstance->AddToViewport();
                TesterWidgetInstance->HitDetectionResult = FText::FromString("Hit Detection Result:");
            }
            
            // Additionally, calculate a margin in inches (converted to screen pixels)
            // that is the hit tolerance. This ensures the clicking or touching has the
            // same physical tolerance on all devices. You could additionally make this
            // value smaller on mouse-controlled devices, and larger on
            // finger-controlled devices.
            float DPIScale = 1;
            
            const UUserInterfaceSettings* UISettings = GetDefault<UUserInterfaceSettings>();
            if (UISettings) {
                UE_LOG(LogTemp, Log, TEXT("Has UISettings"));
                const FVector2D CurrentScreenSize2D = FVector2D(GEngine->GameViewport->Viewport->GetSizeXY());
                const FIntPoint CurrentScreenSize(CurrentScreenSize2D.X, CurrentScreenSize2D.Y);
                DPIScale = UISettings->GetDPIScaleBasedOnSize(CurrentScreenSize);
                MiddlePointScreen = FVector2D(CurrentScreenSize2D.X / 2, CurrentScreenSize2D.Y / 2);
                
                HitMarginPixels = 20 * DPIScale;

                UE_LOG(LogTemp, Log, TEXT("Screen Size: %f,%f. Middle Point Screen: %f,%f. DPI Scale: %f. Hit Margin Pixels: %f"), CurrentScreenSize2D.X, CurrentScreenSize2D.Y, MiddlePointScreen.X, MiddlePointScreen.Y, DPIScale, HitMarginPixels);
            }
        }
    }
}

void ALineRendererTester::TestHitDetection()
{
    const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();

    // Hit detect points

    {
        FHitDetectionResult FinalResult;
        const ALineRenderer* FinalLine = nullptr;
    
        for (const auto& Line: LineRenderers) {
            if (!IsValid(Line)) {
                continue;
            }
    
            if (!Line->ShowControlPoints) {
                continue;
            }
            
            const FHitDetectionResult HitResult = Line->HitDetectPoints(PlayerController, MiddlePointScreen);
            if (HitResult.Valid && HitResult.Distance < FinalResult.Distance) {
                FinalResult = HitResult;
                FinalLine = Line;
            }
        }
    
        if (FinalLine && FinalResult.Distance < HitMarginPixels) {
            TesterWidgetInstance->SetHitDetectionResult(FString::Printf(TEXT("Line: %s, Point: %d"), *FinalLine->GetActorLabel(), FinalResult.Segment));
            return;
        }
    }
    
    // Hit detect splines. This only runs if we didn't get any points, because it's more expensive,
    // and points always get click priority anyway.

    {
        FHitDetectionResult FinalResult;
        const ALineRenderer* FinalLine = nullptr;

        for (const auto& Line: LineRenderers) {
            if (!IsValid(Line)) {
                continue;
            }
            
            const FHitDetectionResult HitResult = Line->HitDetectSpline(PlayerController, MiddlePointScreen);
            if (HitResult.Valid && HitResult.Distance < FinalResult.Distance) {
                FinalResult = HitResult;
                FinalLine = Line;
            }
        }

        if (FinalLine && FinalResult.Distance < HitMarginPixels) {
            TesterWidgetInstance->SetHitDetectionResult(FString::Printf(TEXT("Line: %s, Segment: %d, Progress: %f"), *FinalLine->GetActorLabel(), FinalResult.Segment, FinalResult.Progress));
            return;
        }
    }
    
    TesterWidgetInstance->SetHitDetectionResult(FString::Printf(TEXT("No line")));
}

//
// LINEAR MOVEMENT TEST
//

void ALineRendererTester::InitLinearAnimationTest()
{
    Sphere = NewObject<UStaticMeshComponent>(this, UStaticMeshComponent::StaticClass(), TEXT("LinearAnimationSphere"));

    SphereMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'")));
    if (SphereMesh) {
        Sphere->SetStaticMesh(SphereMesh);
        Sphere->SetWorldLocation(FVector(100, -200, 100));
        Sphere->SetWorldScale3D(FVector(0.3, 0.3, 0.3));
        Sphere->RegisterComponent();
        Sphere->bCastCinematicShadow = false;
        Sphere->bCastContactShadow = false;
        Sphere->bCastDynamicShadow = false;
        Sphere->bCastFarShadow = false;
        Sphere->bCastHiddenShadow = false;
        Sphere->bCastInsetShadow = false;
        Sphere->bCastStaticShadow = false;
        Sphere->bCastVolumetricTranslucentShadow = false;
        Sphere->bCastDistanceFieldIndirectShadow = false;
        Sphere->bAffectDistanceFieldLighting = false;
        Sphere->bAffectDynamicIndirectLighting = false;
        Sphere->SetCastShadow(false);
        Sphere->SetCastContactShadow(false);
        Sphere->SetCastHiddenShadow(false);
        Sphere->SetCastInsetShadow(false);
        UMaterialInstanceDynamic* MaterialInstance = nullptr;

        const FString MaterialName = "SolidColor";
        const FString FullPath = TEXT(LINERENDERER_MATERIALS_PATH) + MaterialName + TEXT(".") + MaterialName;
        UMaterial* LoadedMaterial = LoadObject<UMaterial>(nullptr, *FullPath);
        if (LoadedMaterial != nullptr) {
            MaterialInstance = UMaterialInstanceDynamic::Create(LoadedMaterial, this);
            Sphere->SetMaterial(0, MaterialInstance);
            MaterialInstance->SetVectorParameterValue(FName("Color1"), FLinearColor(1, 0, 0, 1));
        }
    }
}

void ALineRendererTester::TestLinearMovement()
{
    for (const auto& LineRenderer: LineRenderers) {
        if (IsValid(LineRenderer) && Sphere) {
            // Animate the sphere on the first line renderer encountered.
            
            constexpr float AnimLength = 5; // Seconds
            const float Progress = FMath::Fmod(FPlatformTime::Seconds(), AnimLength) / AnimLength;
            // UE_LOG(LogTemp, Log, TEXT("Progress: %f"), Progress);
            const FVector Pos = LineRenderer->CalculateLinearPoint(Progress);
            Sphere->SetWorldLocation(Pos);

            break;
        }
    }
}
