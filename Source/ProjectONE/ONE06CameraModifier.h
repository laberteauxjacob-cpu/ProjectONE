#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraModifier.h"
#include "ONE06CameraModifier.generated.h"

/** Translation-only presentation. Logical aiming reads the untouched camera component. */
UCLASS()
class PROJECTONE_API UONE06CameraModifier : public UCameraModifier
{
    GENERATED_BODY()
public:
    virtual bool ModifyCamera(float DeltaTime,FMinimalViewInfo& InOutPOV) override;
    void AddImpulse(float Amount);
    float GetAmplitude() const { return Amplitude; }
private:
    float Amplitude=0.f,Phase=0.f;
};
