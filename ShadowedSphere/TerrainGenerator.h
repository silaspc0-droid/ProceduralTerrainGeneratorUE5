// Deterministic source of world terrain. Places islands on a hashed grid and answers height,
// surface normal and island lookups for any world coordinate without storing a heightmap.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrainTypes.h"
#include "TerrainGenerator.generated.h"

UCLASS(Blueprintable)
class SHADOWEDSPHERE_API ATerrainGenerator : public AActor
{
    GENERATED_BODY()

public:
    ATerrainGenerator();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FTerrainWorldSettings WorldSettings;

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    float GetHeightAt(float WorldX, float WorldY) const;

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    FVector GetNormalAt(float WorldX, float WorldY) const;

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    bool GetIslandAt(float WorldX, float WorldY, FIslandData& OutIsland) const;

    UFUNCTION(BlueprintCallable, Category = "Terrain")
    TArray<FIslandData> GetIslandsInRadius(float WorldX, float WorldY, float Radius) const;

protected:
    FIslandData GetSpawnIsland() const;
    bool CellHasIsland(int32 CellX, int32 CellY) const;
    FIslandData GenerateIslandForCell(int32 CellX, int32 CellY) const;

    float GetIslandHeightAt(const FIslandData& Island, float WorldX, float WorldY) const;
    float GetIslandInfluence(const FIslandData& Island, float WorldX, float WorldY) const;
    float GetIslandShape(const FIslandData& Island, float Angle) const;
    float GetInteriorHeight(const FIslandData& Island, float LocalX, float LocalY, float NormDist) const;
    float GetBeachProfile(float T) const;
    float GetOceanFloorHeight(float WorldX, float WorldY) const;

    float Noise2D(float X, float Y, int32 Seed) const;
    float FBM(float X, float Y, int32 Octaves, float Freq, int32 Seed) const;

    uint32 Hash(int32 X, int32 Y, int32 Seed) const;
    float HashFloat(int32 X, int32 Y, int32 Seed) const;
    float Smoothstep(float A, float B, float T) const;
};
