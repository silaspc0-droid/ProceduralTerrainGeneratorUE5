// Terrain streaming coordinator. Spawns and owns the TerrainGenerator and BiomeManager,
// then loads and unloads ATerrainChunk actors in a radius around the player, throttling
// mesh generation to a few chunks per frame.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrainTypes.h"
#include "BiomeTypes.h"
#include "ChunkManager.generated.h"

class ATerrainGenerator;
class ATerrainChunk;
class ABiomeManager;

UCLASS(Blueprintable)
class SHADOWEDSPHERE_API AChunkManager : public AActor
{
    GENERATED_BODY()

public:
    AChunkManager();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    FTerrainWorldSettings WorldSettings;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    UMaterialInterface* TerrainMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    TSubclassOf<ATerrainChunk> ChunkClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    int32 MaxChunksPerFrame = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biomes|Islands", meta = (TitleProperty = "BiomeName"))
    TArray<UIslandBiomeData*> IslandBiomes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biomes|Ocean", meta = (TitleProperty = "BiomeName"))
    TArray<UOceanBiomeData*> OceanBiomes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biomes|Elevation", meta = (TitleProperty = "BiomeName"))
    TArray<UElevationZoneData*> ElevationZones;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "References")
    ATerrainGenerator* TerrainGenerator;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "References")
    ABiomeManager* BiomeManager;

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    float GetHeightAt(float WorldX, float WorldY) const;

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    ABiomeManager* GetBiomeManager() const { return BiomeManager; }

protected:
    UPROPERTY()
    TMap<FChunkCoord, ATerrainChunk*> LoadedChunks;

    TArray<FChunkCoord> GenerationQueue;
    FChunkCoord LastPlayerChunk;
    float TimeSinceUpdate = 0.0f;
    bool bFirstUpdate = true;

    void SpawnTerrainGenerator();
    void SpawnBiomeManager();
    void UpdateChunks();
    void LoadChunk(FChunkCoord Coord);
    void UnloadChunk(FChunkCoord Coord);
    FVector GetPlayerPosition() const;
    FChunkCoord WorldToChunk(const FVector& Pos) const;
};
