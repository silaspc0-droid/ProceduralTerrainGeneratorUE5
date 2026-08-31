// Central biome authority for the world. Combines terrain height, slope and island data
// into a single biome query (island biome, elevation zone, ocean biome, depth, distance
// to shore) and answers which foliage, props and entities may spawn at a world position.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BiomeTypes.h"
#include "TerrainTypes.h"
#include "BiomeManager.generated.h"

class ATerrainGenerator;

USTRUCT(BlueprintType)
struct SHADOWEDSPHERE_API FBiomeQueryResult
{
    GENERATED_BODY()

    // Data asset pointers (null if no asset assigned for this biome type)
    UPROPERTY(BlueprintReadOnly)
    UIslandBiomeData* IslandBiome = nullptr;

    UPROPERTY(BlueprintReadOnly)
    UElevationZoneData* ElevationZone = nullptr;

    UPROPERTY(BlueprintReadOnly)
    UOceanBiomeData* OceanBiome = nullptr;

    // Biome types (Empty if no data asset available)
    UPROPERTY(BlueprintReadOnly)
    EIslandBiome IslandBiomeType = EIslandBiome::Empty;

    UPROPERTY(BlueprintReadOnly)
    EElevationZone ElevationZoneType = EElevationZone::Empty;

    UPROPERTY(BlueprintReadOnly)
    EOceanBiome OceanBiomeType = EOceanBiome::Empty;

    UPROPERTY(BlueprintReadOnly)
    bool bIsOnLand = false;

    UPROPERTY(BlueprintReadOnly)
    bool bIsUnderwater = false;

    UPROPERTY(BlueprintReadOnly)
    float DistanceToShore = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float TerrainHeight = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float WaterDepth = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    float SlopeAngle = 0.0f;
};

UCLASS(Blueprintable)
class SHADOWEDSPHERE_API ABiomeManager : public AActor
{
    GENERATED_BODY()

public:
    ABiomeManager();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biomes")
    TArray<UIslandBiomeData*> IslandBiomes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biomes")
    TArray<UOceanBiomeData*> OceanBiomes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biomes")
    TArray<UElevationZoneData*> ElevationZones;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "References")
    ATerrainGenerator* TerrainGenerator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    int32 Seed = 12345;

    // Height thresholds for elevation zones (relative to sea level)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevation")
    float BeachMaxHeight = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevation")
    float CoastalMaxHeight = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevation")
    float InteriorMaxHeight = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elevation")
    float HighlandMaxHeight = 1500.0f;

    // Ocean depth thresholds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
    float ShallowMaxDepth = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
    float DeepOceanMinDepth = 300.0f;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    FBiomeQueryResult QueryBiomeAt(float WorldX, float WorldY) const;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    UIslandBiomeData* GetIslandBiomeData(int32 IslandType) const;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    UIslandBiomeData* GetIslandBiomeForType(int32 IslandType) const;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    TArray<EIslandBiome> GetAvailableIslandBiomes() const;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    UOceanBiomeData* GetOceanBiomeAt(float WorldX, float WorldY) const;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    UOceanBiomeData* GetOceanBiomeByType(EOceanBiome OceanType) const;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    EElevationZone GetElevationZone(float HeightAboveSeaLevel) const;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    EElevationZone CalculateElevationZone(float HeightAboveSeaLevel) const;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    EOceanBiome CalculateOceanBiome(float Depth, float WorldX, float WorldY) const;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    UElevationZoneData* GetElevationZoneData(EElevationZone Zone) const;

    UFUNCTION(BlueprintCallable, Category = "Biome")
    FLinearColor GetTerrainColorAt(float WorldX, float WorldY) const;

    UFUNCTION(BlueprintCallable, Category = "Spawning")
    bool CanSpawnFoliageAt(const FFoliageConfig& Config, float WorldX, float WorldY, float Height, float SlopeAngle) const;

    UFUNCTION(BlueprintCallable, Category = "Spawning")
    TArray<FFoliageConfig> GetFoliageConfigsAt(float WorldX, float WorldY) const;

    UFUNCTION(BlueprintCallable, Category = "Spawning")
    TArray<FPropConfig> GetPropConfigsAt(float WorldX, float WorldY) const;

    UFUNCTION(BlueprintCallable, Category = "Spawning")
    TArray<FEntitySpawnConfig> GetEntityConfigsAt(float WorldX, float WorldY) const;

    float BiomeNoise(float X, float Y, int32 NoiseSeed) const;

protected:
    uint32 Hash(int32 X, int32 Y, int32 S) const;
    float HashFloat(int32 X, int32 Y, int32 S) const;
};
