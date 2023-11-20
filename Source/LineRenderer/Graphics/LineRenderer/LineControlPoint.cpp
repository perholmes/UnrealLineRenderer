// Copyright Hollywood Camera Work

#include "LineControlPoint.h"
#include "LineRendererIncludes.h"

void ULineControlPoint::Init()
{
    SphereMesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, TEXT("StaticMesh'/Engine/BasicShapes/Sphere.Sphere'")));
    if (SphereMesh) {
        SetStaticMesh(SphereMesh);
        // SetWorldLocation(FVector(155.0f, 165.0f, 45.0f));
        SetWorldLocation(FVector(0, 0, 0));
        // SetRelativeLocation(FVector(0, 0, 0));
        SetWorldScale3D(FVector(10, 10, 10));
        // AttachToComponent(this, FAttachmentTransformRules::KeepRelativeTransform);
        RegisterComponent();
        bCastCinematicShadow = false;
        bCastContactShadow = false;
        bCastDynamicShadow = false;
        bCastFarShadow = false;
        bCastHiddenShadow = false;
        bCastInsetShadow = false;
        bCastStaticShadow = false;
        bCastVolumetricTranslucentShadow = false;
        bCastDistanceFieldIndirectShadow = false;
        bAffectDistanceFieldLighting = false;
        bAffectDynamicIndirectLighting = false;
        SetCastShadow(false);
        SetCastContactShadow(false);
        SetCastHiddenShadow(false);
        SetCastInsetShadow(false);
    }
}

void ULineControlPoint::UpdatePosition()
{
    SetWorldLocation(Position);

    // This is a scale of the static mesh, which is already 100 wide, multiplied by 1.5 because
    // control points by default are 1.5 times the line width.
    const float Scale = (LineWidth * ControlPointScale * 1.5) / 100;
    SetWorldScale3D(FVector(Scale, Scale, Scale));
}

void ULineControlPoint::UpdateMaterial()
{
    if (!MaterialInstance) {
        const FString MaterialName = "SolidColor";
        const FString FullPath = TEXT(LINERENDERER_MATERIALS_PATH) + MaterialName + TEXT(".") + MaterialName;
        UMaterial* LoadedMaterial = LoadObject<UMaterial>(nullptr, *FullPath);
        if (LoadedMaterial != nullptr) {
            MaterialInstance = UMaterialInstanceDynamic::Create(LoadedMaterial, this);
            SetMaterial(0, MaterialInstance);
        } else {
            UE_LOG(LogTemp, Warning, TEXT("Couldn't find material %s"), *FullPath);
            MaterialInstance = nullptr;
        }
    }
    
    if (MaterialInstance) {
        MaterialInstance->SetVectorParameterValue(FName("Color1"), ControlPointColor);
    } else {
        UE_LOG(LogTemp, Log, TEXT("No material instance on control point"));
    }
}

void ULineControlPoint::SetPosition(const FVector& Pos)
{
    // Only intended to be used by demo animations, to move one of the control points around to
    // preview movement.
    SetWorldLocation(Position);
}
