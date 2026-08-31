// On-screen debug overlay. Locates the ChunkManager, BiomeManager and TerrainGenerator
// at BeginPlay and each frame draws the player's position, terrain height, biome query
// result, current chunk and island data.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "BiomeDebugHUD.generated.h"

class AChunkManager;
class ABiomeManager;
class ATerrainGenerator;

UCLASS()
class SHADOWEDSPHERE_API ABiomeDebugHUD : public AHUD
{
    GENERATED_BODY()

public:
    ABiomeDebugHUD();

    virtual void DrawHUD() override;
    virtual void BeginPlay() override;

    // Toggle debug display
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    bool bShowDebug = true;

    // Text settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FColor TextColor = FColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FColor HeaderColor = FColor::Yellow;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    float TextScale = 1.0f;

protected:
    UPROPERTY()
    AChunkManager* ChunkManager;

    UPROPERTY()
    ABiomeManager* BiomeManager;

    UPROPERTY()
    ATerrainGenerator* TerrainGenerator;

    void FindManagers();
    void DrawDebugInfo();
};
