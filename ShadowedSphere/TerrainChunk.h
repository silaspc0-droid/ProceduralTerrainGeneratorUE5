// One streamed square of terrain. Owns a procedural mesh component and builds its vertices,
// normals and per-biome material sections on demand from the TerrainGenerator and
// BiomeManager it is initialised with.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "TerrainTypes.h"
#include "TerrainChunk.generated.h"

class ATerrainGenerator;
class ABiomeManager;

UCLASS()
class SHADOWEDSPHERE_API ATerrainChunk : public AActor
{
    GENERATED_BODY()

public:
    ATerrainChunk();

    UFUNCTION(BlueprintCallable)
    void Initialize(FChunkCoord InCoord, ATerrainGenerator* InGenerator, UMaterialInterface* InMaterial, ABiomeManager* InBiomeManager = nullptr);

    UFUNCTION(BlueprintCallable)
    void GenerateMesh();

    UFUNCTION(BlueprintCallable)
    FChunkCoord GetCoord() const { return Coord; }

    UFUNCTION(BlueprintCallable)
    bool IsGenerated() const { return bIsGenerated; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    UProceduralMeshComponent* MeshComponent;

protected:
    FChunkCoord Coord;

    UPROPERTY()
    ATerrainGenerator* Generator;

    UPROPERTY()
    ABiomeManager* BiomeManager;

    UPROPERTY()
    UMaterialInterface* TerrainMaterial;

    bool bIsGenerated = false;
};
