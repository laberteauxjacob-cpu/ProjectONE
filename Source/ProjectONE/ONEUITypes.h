#pragma once
#include "CoreMinimal.h"
#include "ONEUITypes.generated.h"

/** UI requests only; the production controller owns all game/state mutations. */
UENUM()
enum class EONEUIAction : uint8
{
    None, Resume, Restart, Quit, ToggleSandbox,
    SpawnOne, SpawnSix, Refill, GrantPoints,
    ResetSandbox, ClearGore, ToggleLighting,
    ForcePistol, ForceCarbine, ForceShotgun, RandomBox,
    CloseTools
};
