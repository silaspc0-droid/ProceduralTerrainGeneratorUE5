// Data definitions for the biome system: the island, elevation-zone and ocean-biome enums,
// the foliage, prop, entity and terrain-material config structs, and the UDataAsset classes
// (base, island, ocean, elevation zone) that designers author biomes with in the editor.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "BiomeTypes.generated.h"

UENUM(BlueprintType)
enum class EIslandBiome : uint8
{
    Empty       UMETA(DisplayName = "Empty"),
    Tropical    UMETA(DisplayName = "Tropical"),
    Volcanic    UMETA(DisplayName = "Volcanic"),
    Jungle      UMETA(DisplayName = "Jungle"),
    Rocky       UMETA(DisplayName = "Rocky"),
    Desert      UMETA(DisplayName = "Desert"),
    Mangrove    UMETA(DisplayName = "Mangrove")
};

UENUM(BlueprintType)
enum class EElevationZone : uint8
{
    Empty       UMETA(DisplayName = "Empty"),
    Underwater  UMETA(DisplayName = "Underwater"),
    Beach       UMETA(DisplayName = "Beach"),
    Coastal     UMETA(DisplayName = "Coastal"),
    Interior    UMETA(DisplayName = "Interior"),
    Highland    UMETA(DisplayName = "Highland"),
    Peak        UMETA(DisplayName = "Peak")
};

UENUM(BlueprintType)
enum class EOceanBiome : uint8
{
    Empty       UMETA(DisplayName = "Empty"),
    DeepOcean   UMETA(DisplayName = "Deep Ocean"),
    OpenOcean   UMETA(DisplayName = "Open Ocean"),
    Shallows    UMETA(DisplayName = "Shallows"),
    CoralReef   UMETA(DisplayName = "Coral Reef"),
    KelpForest  UMETA(DisplayName = "Kelp Forest"),
    Shipwreck   UMETA(DisplayName = "Shipwreck Zone")
};

USTRUCT(BlueprintType)
struct SHADOWEDSPHERE_API FFoliageConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
    UStaticMesh* Mesh = nullptr;

    // Instances per 100x100 meter area
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Density", meta = (ClampMin = "0", ClampMax = "50"))
    float Density = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    FVector2D ScaleRange = FVector2D(0.8f, 1.2f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    bool bRandomYaw = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    bool bAlignToSlope = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    float MaxSlopeAngle = 30.0f;

    // Height relative to sea level
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    FVector2D HeightRange = FVector2D(5.0f, 2000.0f);

    // Elevation zones where this can spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    TArray<EElevationZone> AllowedZones;

    // LOD distance multiplier (lower = disappears sooner)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD", meta = (ClampMin = "0.1", ClampMax = "2.0"))
    float LODDistanceScale = 1.0f;
};

USTRUCT(BlueprintType)
struct SHADOWEDSPHERE_API FPropConfig
{
    GENERATED_BODY()

    // Actor to spawn (for complex props with logic)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    TSubclassOf<AActor> ActorClass;

    // Or static mesh for simple props
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    UStaticMesh* StaticMesh = nullptr;

    // Spawn chance when a valid location is found
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Density", meta = (ClampMin = "0", ClampMax = "1"))
    float SpawnChance = 0.1f;

    // Min distance between instances
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Density")
    float MinSpacing = 1000.0f;

    // Max instances per chunk
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Density")
    int32 MaxPerChunk = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transform")
    FVector2D ScaleRange = FVector2D(1.0f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    FVector2D HeightRange = FVector2D(10.0f, 1000.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    float MaxSlopeAngle = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    TArray<EElevationZone> AllowedZones;

    // Unique props only spawn once per island
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    bool bUniquePerIsland = false;
};

USTRUCT(BlueprintType)
struct SHADOWEDSPHERE_API FEntitySpawnConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    TSubclassOf<AActor> EntityClass;

    // Display name for debugging
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
    FName EntityName;

    // Max alive at once in this biome
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Density")
    int32 MaxCount = 5;

    // Spawn weight relative to other entities
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Density")
    float SpawnWeight = 1.0f;

    // Min/max group size
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups")
    int32 MinGroupSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Groups")
    int32 MaxGroupSize = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    TArray<EElevationZone> AllowedZones;

    // Can spawn in water?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Placement")
    bool bAquatic = false;
};

USTRUCT(BlueprintType)
struct SHADOWEDSPHERE_API FTerrainMaterialConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Material")
    UMaterialInterface* Material = nullptr;

    // Blend based on height (relative to sea level)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend")
    FVector2D HeightRange = FVector2D(-1000.0f, 5000.0f);

    // Blend based on slope (0 = flat, 90 = vertical)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend")
    FVector2D SlopeRange = FVector2D(0.0f, 90.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Blend", meta = (ClampMin = "0.1", ClampMax = "10"))
    float BlendSharpness = 1.0f;
};

UCLASS(BlueprintType, Abstract)
class SHADOWEDSPHERE_API UBiomeDataBase : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
    FName BiomeName = "Default";

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Info")
    FColor DebugColor = FColor::White;

    // Main terrain material
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    UMaterialInterface* TerrainMaterial = nullptr;

    // Foliage to spawn
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage")
    TArray<FFoliageConfig> Foliage;

    // Props and structures
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Props")
    TArray<FPropConfig> Props;

    // Entities (animals, enemies)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Entities")
    TArray<FEntitySpawnConfig> Entities;

    // Ambient sounds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
    USoundBase* AmbientLoop = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio", meta = (ClampMin = "0", ClampMax = "1"))
    float AmbientVolume = 1.0f;
};

UCLASS(BlueprintType)
class SHADOWEDSPHERE_API UIslandBiomeData : public UBiomeDataBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Island")
    EIslandBiome BiomeType = EIslandBiome::Tropical;

    // Override terrain colors for vertex painting
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    FLinearColor BeachColor = FLinearColor(0.9f, 0.85f, 0.6f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    FLinearColor GrassColor = FLinearColor(0.3f, 0.6f, 0.2f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    FLinearColor RockColor = FLinearColor(0.5f, 0.45f, 0.4f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    FLinearColor PeakColor = FLinearColor(0.6f, 0.6f, 0.55f);

    // Terrain shape modifiers
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "0", ClampMax = "1"))
    float CliffFrequency = 0.3f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "0", ClampMax = "1"))
    float Hilliness = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shape", meta = (ClampMin = "0", ClampMax = "1"))
    float BeachWidth = 0.1f;
};

UCLASS(BlueprintType)
class SHADOWEDSPHERE_API UOceanBiomeData : public UBiomeDataBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
    EOceanBiome OceanType = EOceanBiome::OpenOcean;

    // Depth range where this biome appears
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean")
    FVector2D DepthRange = FVector2D(0.0f, 1000.0f);

    // Water surface color tint
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    FLinearColor WaterColor = FLinearColor(0.1f, 0.3f, 0.5f);

    // Underwater fog
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    FLinearColor UnderwaterFogColor = FLinearColor(0.05f, 0.2f, 0.3f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    float UnderwaterFogDensity = 0.02f;

    // Wave intensity modifier
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water", meta = (ClampMin = "0", ClampMax = "2"))
    float WaveIntensity = 1.0f;

    // Current strength
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    float CurrentStrength = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Water")
    FVector2D CurrentDirection = FVector2D(1, 0);
};

UCLASS(BlueprintType)
class SHADOWEDSPHERE_API UElevationZoneData : public UBiomeDataBase
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
    EElevationZone ZoneType = EElevationZone::Coastal;

    // Height range relative to sea level
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Zone")
    FVector2D HeightRange = FVector2D(0.0f, 100.0f);

    // This zone's color contribution
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
    FLinearColor ZoneColor = FLinearColor::White;
};
