// Populates streamed terrain with foliage. Reads the foliage configs of the biome under
// each chunk and scatters instances with a deterministic hash, batching them into shared
// hierarchical instanced static mesh components.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BiomeTypes.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "FoliageManager.generated.h"

class ABiomeManager;
class ATerrainGenerator;
class AChunkManager;

USTRUCT()
struct FFoliageChunkData
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<UHierarchicalInstancedStaticMeshComponent*> MeshComponents;

    bool bIsGenerated = false;
};

UCLASS(Blueprintable)
class SHADOWEDSPHERE_API AFoliageManager : public AActor
{
    GENERATED_BODY()

public:
    AFoliageManager();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // How many foliage instances to spawn per frame (performance)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 MaxSpawnsPerFrame = 50;

    // Distance to spawn foliage (in chunks)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Performance")
    int32 FoliageLoadRadius = 2;

    // Seed for deterministic placement
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
    int32 Seed = 12345;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
    AChunkManager* ChunkManager;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
    ABiomeManager* BiomeManager;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
    ATerrainGenerator* TerrainGenerator;

    UFUNCTION(BlueprintCallable, Category = "Foliage")
    void GenerateFoliageForChunk(FIntPoint ChunkCoord);

    UFUNCTION(BlueprintCallable, Category = "Foliage")
    void ClearFoliageForChunk(FIntPoint ChunkCoord);

    UFUNCTION(BlueprintCallable, Category = "Foliage")
    void ClearAllFoliage();

protected:
    // Chunk coord -> foliage data
    UPROPERTY()
    TMap<FIntPoint, FFoliageChunkData> ChunkFoliage;

    // Mesh -> HISM component (shared across chunks)
    UPROPERTY()
    TMap<UStaticMesh*, UHierarchicalInstancedStaticMeshComponent*> MeshToHISM;

    FIntPoint LastPlayerChunk;
    float TimeSinceUpdate = 0.0f;

    void UpdateFoliageChunks();
    UHierarchicalInstancedStaticMeshComponent* GetOrCreateHISM(UStaticMesh* Mesh);
    FVector GetPlayerPosition() const;

    float HashFloat(int32 X, int32 Y, int32 S) const;
    uint32 Hash(int32 X, int32 Y, int32 S) const;
};
