// Copyright Hollywood Camera Work

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LineRendererTestWidget.generated.h"

UCLASS()
class LINERENDERER_API ULineRendererTestWidget : public UUserWidget
{
    GENERATED_BODY()

    public: void SetHitDetectionResult(const FString& Text) { HitDetectionResult = FText::FromString(Text); }
    
    public: UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText HitDetectionResult;
};
