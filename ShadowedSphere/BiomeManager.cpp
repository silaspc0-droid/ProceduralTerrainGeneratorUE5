// Implementation of ABiomeManager: resolves biome queries from terrain data, derives
// elevation zones and ocean biomes from height and depth thresholds, produces terrain
// vertex colours, and filters spawn configs by height, slope and zone.

#include "BiomeManager.h"
#include "TerrainGenerator.h"

ABiomeManager::ABiomeManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

FBiomeQueryResult ABiomeManager::QueryBiomeAt(float WorldX, float WorldY) const
{
    FBiomeQueryResult Result;

    Result.IslandBiomeType = EIslandBiome::Empty;
    Result.ElevationZoneType = EElevationZone::Empty;
    Result.OceanBiomeType = EOceanBiome::Empty;

    if (!TerrainGenerator)
    {
        UE_LOG(LogTemp, Warning, TEXT("QueryBiomeAt: No TerrainGenerator!"));
        return Result;
    }

    float SeaLevel = TerrainGenerator->WorldSettings.SeaLevel;
    Result.TerrainHeight = TerrainGenerator->GetHeightAt(WorldX, WorldY);
    float HeightAboveSea = Result.TerrainHeight - SeaLevel;

    Result.bIsOnLand = HeightAboveSea > 0.0f;
    Result.bIsUnderwater = HeightAboveSea < -1.0f;
    Result.WaterDepth = Result.bIsOnLand ? 0.0f : -HeightAboveSea;

    FVector Normal = TerrainGenerator->GetNormalAt(WorldX, WorldY);
    Result.SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(FMath::Abs(Normal.Z)));

    EElevationZone CalculatedZone = CalculateElevationZone(HeightAboveSea);
    Result.ElevationZone = GetElevationZoneData(CalculatedZone);
    if (Result.ElevationZone)
    {
        Result.ElevationZoneType = CalculatedZone;
    }

    if (Result.bIsOnLand)
    {
        FIslandData Island;
        if (TerrainGenerator->GetIslandAt(WorldX, WorldY, Island))
        {
            if (IslandBiomes.Num() > 0)
            {
                int32 BiomeIndex = FMath::Abs(Island.Seed) % IslandBiomes.Num();
                Result.IslandBiome = IslandBiomes[BiomeIndex];
                if (Result.IslandBiome)
                {
                    Result.IslandBiomeType = Result.IslandBiome->BiomeType;
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("QueryBiomeAt: On island but IslandBiomes array is empty!"));
            }

            FVector2D ToCenter = FVector2D(WorldX, WorldY) - Island.Center;
            float DistFromCenter = ToCenter.Size();
            Result.DistanceToShore = Island.Radius - DistFromCenter;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("QueryBiomeAt: bIsOnLand=true but GetIslandAt returned false at (%.0f, %.0f)"), WorldX, WorldY);
        }
    }
    else
    {
        if (OceanBiomes.Num() > 0)
        {
            EOceanBiome CalculatedOcean = CalculateOceanBiome(Result.WaterDepth, WorldX, WorldY);
            Result.OceanBiome = GetOceanBiomeByType(CalculatedOcean);
            if (Result.OceanBiome)
            {
                Result.OceanBiomeType = CalculatedOcean;
            }
            else
            {
                Result.OceanBiome = OceanBiomes[0];
                if (Result.OceanBiome)
                {
                    Result.OceanBiomeType = Result.OceanBiome->OceanType;
                }
            }
        }
    }

    return Result;
}

UIslandBiomeData* ABiomeManager::GetIslandBiomeData(int32 IslandType) const
{
    return GetIslandBiomeForType(IslandType);
}

UIslandBiomeData* ABiomeManager::GetIslandBiomeForType(int32 IslandType) const
{
    if (IslandBiomes.Num() == 0)
    {
        return nullptr;
    }

    int32 Index = FMath::Abs(IslandType) % IslandBiomes.Num();
    return IslandBiomes[Index];
}

TArray<EIslandBiome> ABiomeManager::GetAvailableIslandBiomes() const
{
    TArray<EIslandBiome> Result;
    for (UIslandBiomeData* Biome : IslandBiomes)
    {
        if (Biome)
        {
            Result.AddUnique(Biome->BiomeType);
        }
    }
    return Result;
}

UOceanBiomeData* ABiomeManager::GetOceanBiomeAt(float WorldX, float WorldY) const
{
    if (!TerrainGenerator || OceanBiomes.Num() == 0)
    {
        return nullptr;
    }

    float Height = TerrainGenerator->GetHeightAt(WorldX, WorldY);
    float Depth = TerrainGenerator->WorldSettings.SeaLevel - Height;

    if (Depth <= 0)
    {
        return nullptr;
    }

    EOceanBiome OceanType = CalculateOceanBiome(Depth, WorldX, WorldY);
    return GetOceanBiomeByType(OceanType);
}

UOceanBiomeData* ABiomeManager::GetOceanBiomeByType(EOceanBiome OceanType) const
{
    for (UOceanBiomeData* Biome : OceanBiomes)
    {
        if (Biome && Biome->OceanType == OceanType)
        {
            return Biome;
        }
    }

    return OceanBiomes.Num() > 0 ? OceanBiomes[0] : nullptr;
}

UElevationZoneData* ABiomeManager::GetElevationZoneData(EElevationZone Zone) const
{
    for (UElevationZoneData* ZoneData : ElevationZones)
    {
        if (ZoneData && ZoneData->ZoneType == Zone)
        {
            return ZoneData;
        }
    }
    return nullptr;
}

EElevationZone ABiomeManager::GetElevationZone(float HeightAboveSeaLevel) const
{
    return CalculateElevationZone(HeightAboveSeaLevel);
}

EElevationZone ABiomeManager::CalculateElevationZone(float HeightAboveSeaLevel) const
{
    if (HeightAboveSeaLevel < -1.0f)
    {
        return EElevationZone::Underwater;
    }
    else if (HeightAboveSeaLevel < BeachMaxHeight)
    {
        return EElevationZone::Beach;
    }
    else if (HeightAboveSeaLevel < CoastalMaxHeight)
    {
        return EElevationZone::Coastal;
    }
    else if (HeightAboveSeaLevel < InteriorMaxHeight)
    {
        return EElevationZone::Interior;
    }
    else if (HeightAboveSeaLevel < HighlandMaxHeight)
    {
        return EElevationZone::Highland;
    }
    else
    {
        return EElevationZone::Peak;
    }
}

EOceanBiome ABiomeManager::CalculateOceanBiome(float Depth, float WorldX, float WorldY) const
{
    if (Depth > DeepOceanMinDepth)
    {
        return EOceanBiome::DeepOcean;
    }

    if (Depth < ShallowMaxDepth)
    {
        float Noise = BiomeNoise(WorldX * 0.0001f, WorldY * 0.0001f, Seed + 5000);

        if (Noise > 0.65f)
        {
            return EOceanBiome::CoralReef;
        }
        else if (Noise < 0.25f)
        {
            return EOceanBiome::KelpForest;
        }
        else
        {
            return EOceanBiome::Shallows;
        }
    }

    return EOceanBiome::OpenOcean;
}

FLinearColor ABiomeManager::GetTerrainColorAt(float WorldX, float WorldY) const
{
    if (!TerrainGenerator)
    {
        return FLinearColor::Gray;
    }

    FBiomeQueryResult Query = QueryBiomeAt(WorldX, WorldY);

    if (Query.bIsUnderwater)
    {
        float DepthT = FMath::Clamp(Query.WaterDepth / 500.0f, 0.0f, 1.0f);
        return FMath::Lerp(FLinearColor(0.2f, 0.4f, 0.6f), FLinearColor(0.05f, 0.15f, 0.3f), DepthT);
    }

    UIslandBiomeData* BiomeData = Query.IslandBiome;

    FLinearColor BeachColor = FLinearColor(0.9f, 0.85f, 0.6f);
    FLinearColor GrassColor = FLinearColor(0.3f, 0.6f, 0.2f);
    FLinearColor RockColor = FLinearColor(0.5f, 0.45f, 0.4f);
    FLinearColor PeakColor = FLinearColor(0.6f, 0.6f, 0.55f);

    if (BiomeData)
    {
        BeachColor = BiomeData->BeachColor;
        GrassColor = BiomeData->GrassColor;
        RockColor = BiomeData->RockColor;
        PeakColor = BiomeData->PeakColor;
    }

    FLinearColor BaseColor;

    switch (Query.ElevationZoneType)
    {
    case EElevationZone::Beach:
        BaseColor = BeachColor;
        break;

    case EElevationZone::Coastal:
    case EElevationZone::Interior:
        BaseColor = GrassColor;
        break;

    case EElevationZone::Highland:
    {
        float T = (Query.TerrainHeight - InteriorMaxHeight) / (HighlandMaxHeight - InteriorMaxHeight);
        BaseColor = FMath::Lerp(GrassColor, RockColor, T);
    }
    break;

    case EElevationZone::Peak:
        BaseColor = PeakColor;
        break;

    default:
        BaseColor = FLinearColor::Gray;
        break;
    }

    if (Query.SlopeAngle > 30.0f)
    {
        float SlopeBlend = FMath::Clamp((Query.SlopeAngle - 30.0f) / 30.0f, 0.0f, 1.0f);
        BaseColor = FMath::Lerp(BaseColor, RockColor, SlopeBlend);
    }

    return BaseColor;
}

bool ABiomeManager::CanSpawnFoliageAt(const FFoliageConfig& Config, float WorldX, float WorldY, float Height, float SlopeAngle) const
{
    if (!TerrainGenerator)
    {
        return false;
    }

    float SeaLevel = TerrainGenerator->WorldSettings.SeaLevel;
    float HeightAboveSea = Height - SeaLevel;

    if (HeightAboveSea < Config.HeightRange.X || HeightAboveSea > Config.HeightRange.Y)
    {
        return false;
    }

    if (SlopeAngle > Config.MaxSlopeAngle)
    {
        return false;
    }

    if (Config.AllowedZones.Num() > 0)
    {
        EElevationZone Zone = GetElevationZone(HeightAboveSea);
        if (!Config.AllowedZones.Contains(Zone))
        {
            return false;
        }
    }

    return true;
}

TArray<FFoliageConfig> ABiomeManager::GetFoliageConfigsAt(float WorldX, float WorldY) const
{
    TArray<FFoliageConfig> Result;

    FBiomeQueryResult Query = QueryBiomeAt(WorldX, WorldY);

    if (Query.bIsOnLand)
    {
        if (Query.IslandBiome)
        {
            for (const FFoliageConfig& Config : Query.IslandBiome->Foliage)
            {
                if (CanSpawnFoliageAt(Config, WorldX, WorldY, Query.TerrainHeight, Query.SlopeAngle))
                {
                    Result.Add(Config);
                }
            }
        }

        if (Query.ElevationZone)
        {
            for (const FFoliageConfig& Config : Query.ElevationZone->Foliage)
            {
                if (CanSpawnFoliageAt(Config, WorldX, WorldY, Query.TerrainHeight, Query.SlopeAngle))
                {
                    Result.Add(Config);
                }
            }
        }
    }
    else
    {
        if (Query.OceanBiome)
        {
            Result.Append(Query.OceanBiome->Foliage);
        }
    }

    return Result;
}

TArray<FPropConfig> ABiomeManager::GetPropConfigsAt(float WorldX, float WorldY) const
{
    TArray<FPropConfig> Result;

    FBiomeQueryResult Query = QueryBiomeAt(WorldX, WorldY);

    if (Query.bIsOnLand)
    {
        if (Query.IslandBiome)
        {
            for (const FPropConfig& Config : Query.IslandBiome->Props)
            {
                float HeightAboveSea = Query.TerrainHeight - TerrainGenerator->WorldSettings.SeaLevel;
                if (HeightAboveSea >= Config.HeightRange.X &&
                    HeightAboveSea <= Config.HeightRange.Y &&
                    Query.SlopeAngle <= Config.MaxSlopeAngle)
                {
                    if (Config.AllowedZones.Num() == 0 || Config.AllowedZones.Contains(Query.ElevationZoneType))
                    {
                        Result.Add(Config);
                    }
                }
            }
        }
    }
    else
    {
        if (Query.OceanBiome)
        {
            Result.Append(Query.OceanBiome->Props);
        }
    }

    return Result;
}

TArray<FEntitySpawnConfig> ABiomeManager::GetEntityConfigsAt(float WorldX, float WorldY) const
{
    TArray<FEntitySpawnConfig> Result;

    FBiomeQueryResult Query = QueryBiomeAt(WorldX, WorldY);

    if (Query.bIsOnLand)
    {
        if (Query.IslandBiome)
        {
            for (const FEntitySpawnConfig& Config : Query.IslandBiome->Entities)
            {
                if (Config.AllowedZones.Num() == 0 || Config.AllowedZones.Contains(Query.ElevationZoneType))
                {
                    if (!Config.bAquatic)
                    {
                        Result.Add(Config);
                    }
                }
            }
        }
    }
    else
    {
        if (Query.OceanBiome)
        {
            for (const FEntitySpawnConfig& Config : Query.OceanBiome->Entities)
            {
                if (Config.bAquatic)
                {
                    Result.Add(Config);
                }
            }
        }
    }

    return Result;
}

uint32 ABiomeManager::Hash(int32 X, int32 Y, int32 S) const
{
    uint32 H = (uint32)(X * 374761393 + Y * 668265263 + S * 1013904223);
    H = (H ^ (H >> 13)) * 1274126177;
    return H ^ (H >> 16);
}

float ABiomeManager::HashFloat(int32 X, int32 Y, int32 S) const
{
    return (float)(Hash(X, Y, S) & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

float ABiomeManager::BiomeNoise(float X, float Y, int32 NoiseSeed) const
{
    int32 Xi = FMath::FloorToInt(X);
    int32 Yi = FMath::FloorToInt(Y);
    float Xf = X - Xi;
    float Yf = Y - Yi;

    float N00 = HashFloat(Xi, Yi, NoiseSeed);
    float N10 = HashFloat(Xi + 1, Yi, NoiseSeed);
    float N01 = HashFloat(Xi, Yi + 1, NoiseSeed);
    float N11 = HashFloat(Xi + 1, Yi + 1, NoiseSeed);

    float U = Xf * Xf * (3.0f - 2.0f * Xf);
    float V = Yf * Yf * (3.0f - 2.0f * Yf);

    return FMath::Lerp(FMath::Lerp(N00, N10, U), FMath::Lerp(N01, N11, U), V);
}
