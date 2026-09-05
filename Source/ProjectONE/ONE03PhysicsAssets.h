#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ONE03PhysicsAssets.generated.h"

/** Reproducible editor authoring entry point; never generates physics in gameplay. */
UCLASS()
class PROJECTONE_API UONE03PhysicsAssets : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="ONE|Authoring")
    static bool BuildInfectedAssets();
    UFUNCTION(BlueprintCallable, Category="ONE|Authoring")
    static FString InspectEnvironmentCollision();
    UFUNCTION(BlueprintCallable, Category="ONE|Authoring")
    static bool RebuildNavigationAndWait(UWorld* World);
};
