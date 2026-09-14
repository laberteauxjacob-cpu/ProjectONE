#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ONE07PhysicsAssets.generated.h"

UCLASS()
class PROJECTONE_API UONE07PhysicsAssets : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="ONE|Authoring")
    static bool BuildInfectedAssets();
};
