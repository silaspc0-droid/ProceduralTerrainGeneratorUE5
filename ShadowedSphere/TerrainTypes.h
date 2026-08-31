// Shared terrain data structures: the world generation settings exposed in the editor, the
// per-island parameters produced by the generator, and the chunk coordinate key used by the
// streaming map.

#pragma once

#include "CoreMinimal.h"
#include "TerrainTypes.generated.h"

USTRUCT(BlueprintType)
struct SHADOWEDSPHERE_API FTerrainWorldSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    float SeaLevel = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    int32 Seed = 12345;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    float OceanFloorDepth = 500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunks")
    float ChunkSize = 8192.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunks")
    int32 ChunkResolution = 64;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunks")
    int32 LoadRadius = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunks")
    int32 UnloadRadius = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Islands")
    float IslandGridSize = 80000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Islands")
    float IslandDensity = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Islands")
    float MinIslandRadius = 6000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Islands")
    float MaxIslandRadius = 30000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Islands")
    float MaxIslandHeight = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Islands")
    float SpawnIslandRadius = 25000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Islands")
    float SpawnIslandHeight = 1500.0f;
};

USTRUCT(BlueprintType)
struct SHADOWEDSPHERE_API FIslandData
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    FVector2D Center = FVector2D::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float Radius = 10000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float MaxHeight = 1500.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float BeachWidth = 1000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 IslandType = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 NumPeaks = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    float Hilliness = 0.4f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bIsSpawnIsland = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bHasVolcano = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bHasLagoon = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bHasCliffs = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Seed = 0;

    TArray<FVector2D> PeakOffsets;
    TArray<float> PeakHeights;
};

USTRUCT(BlueprintType)
struct SHADOWEDSPHERE_API FChunkCoord
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 X = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Y = 0;

    FChunkCoord() = default;
    FChunkCoord(int32 InX, int32 InY) : X(InX), Y(InY) {}

    bool operator==(const FChunkCoord& Other) const { return X == Other.X && Y == Other.Y; }

    friend uint32 GetTypeHash(const FChunkCoord& C) { return HashCombine(GetTypeHash(C.X), GetTypeHash(C.Y)); }

    FString ToString() const { return FString::Printf(TEXT("(%d,%d)"), X, Y); }
};
