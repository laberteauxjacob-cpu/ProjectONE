#include "ONE06CameraModifier.h"
#include "ONEPlayer.h"
#include "Camera/CameraTypes.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

static TAutoConsoleVariable<float> CVarONE06ShakeStrength(TEXT("one.CameraShake.Strength"),1.f,
    TEXT("Presentation translation strength:0 disables,1 default. Logical aim is unchanged."));

void UONE06CameraModifier::AddImpulse(float Amount)
{
    if (FMath::IsFinite(Amount)) Amplitude=FMath::Clamp(Amplitude+FMath::Max(0.f,Amount),0.f,1.8f);
}
bool UONE06CameraModifier::ModifyCamera(float Dt,FMinimalViewInfo& POV)
{
    const auto* Player=CameraOwner ? Cast<AONEPlayer>(CameraOwner->GetViewTarget()) : nullptr;
    if (!Player || Player->IsDead()) { Amplitude=0.f; Phase=0.f; return false; }
    if (UGameplayStatics::IsGamePaused(this)) return false;
    Phase+=FMath::Max(0.f,Dt);
    Amplitude*=FMath::Exp(-18.f*FMath::Max(0.f,Dt));
    if (Amplitude<.005f) Amplitude=0.f;
    const float Strength=FMath::Clamp(CVarONE06ShakeStrength.GetValueOnGameThread(),0.f,1.f);
    const FVector Offset(0,FMath::Sin(Phase*117.f)*Amplitude,FMath::Sin(Phase*93.f+.8f)*Amplitude*.65f);
    POV.Location+=POV.Rotation.RotateVector(Offset)*Strength;
    return false;
}
