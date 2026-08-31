// Implementation of ATerrainGenerator: seeds a fixed spawn island plus hash-placed islands
// per grid cell, then composes ocean floor, wide graded beach profiles, peak falloff, fBm
// hills, cliffs and lagoons into a single height value per world position.

#include "TerrainGenerator.h"

ATerrainGenerator::ATerrainGenerator()
{
    PrimaryActorTick.bCanEverTick = false;
}

float ATerrainGenerator::GetHeightAt(float WorldX, float WorldY) const
{
    float Height = GetOceanFloorHeight(WorldX, WorldY);

    FIslandData SpawnIsland = GetSpawnIsland();
    float SpawnHeight = GetIslandHeightAt(SpawnIsland, WorldX, WorldY);
    Height = FMath::Max(Height, SpawnHeight);

    TArray<FIslandData> Islands = GetIslandsInRadius(WorldX, WorldY, WorldSettings.MaxIslandRadius * 1.2f);
    for (const FIslandData& Island : Islands)
    {
        if (!Island.bIsSpawnIsland)
        {
            float IslandHeight = GetIslandHeightAt(Island, WorldX, WorldY);
            Height = FMath::Max(Height, IslandHeight);
        }
    }

    return Height;
}

FVector ATerrainGenerator::GetNormalAt(float WorldX, float WorldY) const
{
    const float D = 50.0f;
    float Hc = GetHeightAt(WorldX, WorldY);
    float Hx = GetHeightAt(WorldX + D, WorldY);
    float Hy = GetHeightAt(WorldX, WorldY + D);
    return FVector(Hc - Hx, Hc - Hy, D).GetSafeNormal();
}

FIslandData ATerrainGenerator::GetSpawnIsland() const
{
    FIslandData Island;
    Island.Center = FVector2D(0, 0);
    Island.Radius = WorldSettings.SpawnIslandRadius;
    Island.MaxHeight = WorldSettings.SpawnIslandHeight;
    Island.BeachWidth = Island.Radius * 0.5f;
    Island.IslandType = 0;
    Island.NumPeaks = 2;
    Island.Hilliness = 0.25f;
    Island.bIsSpawnIsland = true;
    Island.Seed = WorldSettings.Seed;

    FRandomStream R(WorldSettings.Seed);
    for (int32 i = 0; i < Island.NumPeaks; i++)
    {
        float Angle = R.FRandRange(0.0f, 2.0f * PI);
        float Dist = R.FRandRange(0.1f, 0.3f) * Island.Radius;
        Island.PeakOffsets.Add(FVector2D(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist));
        Island.PeakHeights.Add(Island.MaxHeight * R.FRandRange(0.6f, 1.0f));
    }

    return Island;
}

bool ATerrainGenerator::GetIslandAt(float WorldX, float WorldY, FIslandData& OutIsland) const
{
    FIslandData Spawn = GetSpawnIsland();
    if (GetIslandInfluence(Spawn, WorldX, WorldY) > 0.05f)
    {
        OutIsland = Spawn;
        return true;
    }

    TArray<FIslandData> Islands = GetIslandsInRadius(WorldX, WorldY, WorldSettings.MaxIslandRadius);
    for (const FIslandData& Island : Islands)
    {
        if (GetIslandInfluence(Island, WorldX, WorldY) > 0.05f)
        {
            OutIsland = Island;
            return true;
        }
    }

    return false;
}

TArray<FIslandData> ATerrainGenerator::GetIslandsInRadius(float WorldX, float WorldY, float Radius) const
{
    TArray<FIslandData> Result;

    float SearchRadius = Radius + WorldSettings.MaxIslandRadius;
    float GridSize = WorldSettings.IslandGridSize;

    int32 MinCellX = FMath::FloorToInt((WorldX - SearchRadius) / GridSize);
    int32 MaxCellX = FMath::FloorToInt((WorldX + SearchRadius) / GridSize);
    int32 MinCellY = FMath::FloorToInt((WorldY - SearchRadius) / GridSize);
    int32 MaxCellY = FMath::FloorToInt((WorldY + SearchRadius) / GridSize);

    for (int32 CellY = MinCellY; CellY <= MaxCellY; CellY++)
    {
        for (int32 CellX = MinCellX; CellX <= MaxCellX; CellX++)
        {
            if (CellX == 0 && CellY == 0) continue;

            if (CellHasIsland(CellX, CellY))
            {
                FIslandData Island = GenerateIslandForCell(CellX, CellY);
                float Dist = FVector2D::Distance(FVector2D(WorldX, WorldY), Island.Center);
                if (Dist < Radius + Island.Radius)
                {
                    Result.Add(Island);
                }
            }
        }
    }

    return Result;
}

bool ATerrainGenerator::CellHasIsland(int32 CellX, int32 CellY) const
{
    float Noise = HashFloat(CellX, CellY, WorldSettings.Seed + 777);
    return Noise < WorldSettings.IslandDensity;
}

FIslandData ATerrainGenerator::GenerateIslandForCell(int32 CellX, int32 CellY) const
{
    FIslandData Island;
    Island.Seed = Hash(CellX, CellY, WorldSettings.Seed);
    FRandomStream R(Island.Seed);

    float GridSize = WorldSettings.IslandGridSize;
    float OffsetX = R.FRandRange(0.25f, 0.75f) * GridSize;
    float OffsetY = R.FRandRange(0.25f, 0.75f) * GridSize;
    Island.Center = FVector2D(CellX * GridSize + OffsetX, CellY * GridSize + OffsetY);

    Island.IslandType = R.RandRange(0, 4);

    switch (Island.IslandType)
    {
    case 0:
        Island.Radius = R.FRandRange(15000.0f, 30000.0f);
        Island.MaxHeight = R.FRandRange(300.0f, 800.0f);
        Island.Hilliness = R.FRandRange(0.15f, 0.3f);
        Island.NumPeaks = R.RandRange(1, 2);
        break;
    case 1:
        Island.Radius = R.FRandRange(12000.0f, 22000.0f);
        Island.MaxHeight = R.FRandRange(1500.0f, 2500.0f);
        Island.Hilliness = R.FRandRange(0.3f, 0.5f);
        Island.NumPeaks = 1;
        Island.bHasVolcano = true;
        break;
    case 2:
        Island.Radius = R.FRandRange(18000.0f, 35000.0f);
        Island.MaxHeight = R.FRandRange(1200.0f, 2200.0f);
        Island.Hilliness = R.FRandRange(0.4f, 0.6f);
        Island.NumPeaks = R.RandRange(2, 4);
        break;
    case 3:
        Island.Radius = R.FRandRange(10000.0f, 18000.0f);
        Island.MaxHeight = R.FRandRange(600.0f, 1200.0f);
        Island.Hilliness = R.FRandRange(0.3f, 0.5f);
        Island.NumPeaks = R.RandRange(1, 2);
        Island.bHasCliffs = true;
        break;
    default:
        Island.Radius = R.FRandRange(14000.0f, 28000.0f);
        Island.MaxHeight = R.FRandRange(400.0f, 1000.0f);
        Island.Hilliness = R.FRandRange(0.2f, 0.4f);
        Island.NumPeaks = R.RandRange(1, 3);
        break;
    }

    Island.Radius = FMath::Clamp(Island.Radius, WorldSettings.MinIslandRadius, WorldSettings.MaxIslandRadius);
    Island.MaxHeight = FMath::Min(Island.MaxHeight, WorldSettings.MaxIslandHeight);

    Island.BeachWidth = Island.Radius * R.FRandRange(0.40f, 0.55f);

    for (int32 i = 0; i < Island.NumPeaks; i++)
    {
        FVector2D Offset;
        if (Island.bHasVolcano && i == 0)
        {
            Offset = FVector2D::ZeroVector;
        }
        else
        {
            float Angle = R.FRandRange(0.0f, 2.0f * PI);
            float Dist = R.FRandRange(0.05f, 0.30f) * Island.Radius;
            Offset = FVector2D(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist);
        }
        Island.PeakOffsets.Add(Offset);
        Island.PeakHeights.Add(Island.MaxHeight * R.FRandRange(0.5f, 1.0f));
    }

    return Island;
}

float ATerrainGenerator::GetOceanFloorHeight(float WorldX, float WorldY) const
{
    float Base = WorldSettings.SeaLevel - WorldSettings.OceanFloorDepth;
    float Noise = FBM(WorldX * 0.00003f, WorldY * 0.00003f, 2, 1.0f, WorldSettings.Seed + 999);
    return Base + Noise * 100.0f;
}

float ATerrainGenerator::GetIslandInfluence(const FIslandData& Island, float WorldX, float WorldY) const
{
    FVector2D ToPoint = FVector2D(WorldX, WorldY) - Island.Center;
    float Distance = ToPoint.Size();
    float Angle = FMath::Atan2(ToPoint.Y, ToPoint.X);
    float ShapedRadius = Island.Radius * GetIslandShape(Island, Angle);

    if (Distance > ShapedRadius * 1.2f) return 0.0f;

    float NormDist = Distance / ShapedRadius;
    return 1.0f - Smoothstep(0.7f, 1.1f, NormDist);
}

float ATerrainGenerator::GetIslandHeightAt(const FIslandData& Island, float WorldX, float WorldY) const
{
    FVector2D ToPoint = FVector2D(WorldX, WorldY) - Island.Center;
    float Distance = ToPoint.Size();
    float Angle = FMath::Atan2(ToPoint.Y, ToPoint.X);
    float ShapedRadius = Island.Radius * GetIslandShape(Island, Angle);

    if (Distance > ShapedRadius * 1.15f)
    {
        return WorldSettings.SeaLevel - WorldSettings.OceanFloorDepth;
    }

    float NormDist = Distance / ShapedRadius;

    const float BeachStart = 0.40f;

    if (NormDist > BeachStart)
    {
        float BeachT = (NormDist - BeachStart) / (1.0f - BeachStart);
        float BeachHeight = GetBeachProfile(BeachT);

        float Ripple = FMath::Sin(Distance * 0.01f + Angle * 2.0f) * 0.5f;
        return WorldSettings.SeaLevel + BeachHeight + Ripple;
    }

    float InteriorH = GetInteriorHeight(Island, ToPoint.X, ToPoint.Y, NormDist);

    float EdgeBlend = Smoothstep(0.20f, BeachStart, NormDist);
    float BeachEdgeH = WorldSettings.SeaLevel + 60.0f;

    return FMath::Lerp(InteriorH, BeachEdgeH, EdgeBlend);
}

float ATerrainGenerator::GetIslandShape(const FIslandData& Island, float Angle) const
{
    float Shape = 1.0f;

    Shape += FMath::Sin(Angle * 2.0f + Island.Seed * 0.01f) * 0.15f;
    Shape += FMath::Cos(Angle * 1.0f + Island.Seed * 0.02f) * 0.10f;

    Shape += FMath::Sin(Angle * 4.0f + Island.Seed * 0.03f) * 0.03f;

    float NoiseX = FMath::Cos(Angle) * 20.0f;
    float NoiseY = FMath::Sin(Angle) * 20.0f;
    Shape += Noise2D(NoiseX, NoiseY, Island.Seed) * 0.04f;

    return FMath::Clamp(Shape, 0.7f, 1.3f);
}

float ATerrainGenerator::GetInteriorHeight(const FIslandData& Island, float LocalX, float LocalY, float NormDist) const
{
    float Height = WorldSettings.SeaLevel + 40.0f;

    float WorldX = LocalX + Island.Center.X;
    float WorldY = LocalY + Island.Center.Y;

    for (int32 i = 0; i < Island.PeakOffsets.Num(); i++)
    {
        FVector2D PeakWorld = Island.Center + Island.PeakOffsets[i];
        float PeakDist = FVector2D::Distance(FVector2D(WorldX, WorldY), PeakWorld);
        float PeakRadius = Island.Radius * 0.6f;

        if (PeakDist < PeakRadius)
        {
            float PeakT = 1.0f - Smoothstep(0.0f, PeakRadius, PeakDist);
            PeakT = FMath::Pow(PeakT, 0.5f);
            float PeakH = Island.PeakHeights[i] * PeakT;

            if (Island.bHasVolcano && i == 0 && PeakT > 0.9f)
            {
                float CraterT = (PeakT - 0.9f) / 0.1f;
                PeakH *= 1.0f - CraterT * 0.4f;
            }

            Height = FMath::Max(Height, WorldSettings.SeaLevel + PeakH);
        }
    }

    float Hills = FBM(WorldX * 0.00008f, WorldY * 0.00008f, 3, 1.0f, Island.Seed);
    Height += Hills * Island.MaxHeight * 0.15f * Island.Hilliness;

    float Detail = FBM(WorldX * 0.0003f, WorldY * 0.0003f, 2, 1.0f, Island.Seed + 500);
    Height += Detail * 25.0f * Island.Hilliness;

    if (Island.bHasCliffs && NormDist > 0.15f && NormDist < 0.35f)
    {
        float CliffT = (NormDist - 0.15f) / 0.2f;
        float CliffBoost = 100.0f * Smoothstep(0.0f, 0.4f, CliffT) * (1.0f - Smoothstep(0.6f, 1.0f, CliffT));
        Height += CliffBoost;
    }

    if (Island.bHasLagoon && NormDist < 0.25f)
    {
        float LagoonT = Smoothstep(0.25f, 0.1f, NormDist);
        Height = FMath::Min(Height, WorldSettings.SeaLevel - 80.0f * LagoonT);
    }

    return Height;
}

float ATerrainGenerator::GetBeachProfile(float T) const
{
    T = FMath::Clamp(T, 0.0f, 1.0f);

    if (T < 0.15f)
    {
        float LocalT = T / 0.15f;
        LocalT = FMath::Pow(LocalT, 0.6f);
        return FMath::Lerp(80.0f, 50.0f, LocalT);
    }
    else if (T < 0.30f)
    {
        float LocalT = (T - 0.15f) / 0.15f;
        return FMath::Lerp(50.0f, 25.0f, LocalT);
    }
    else if (T < 0.45f)
    {
        float LocalT = (T - 0.30f) / 0.15f;
        return FMath::Lerp(25.0f, 10.0f, LocalT);
    }
    else if (T < 0.55f)
    {
        float LocalT = (T - 0.45f) / 0.10f;
        return FMath::Lerp(10.0f, -50.0f, LocalT);
    }
    else if (T < 0.70f)
    {
        float LocalT = (T - 0.55f) / 0.15f;
        return FMath::Lerp(-50.0f, -150.0f, LocalT);
    }
    else if (T < 0.85f)
    {
        float LocalT = (T - 0.70f) / 0.15f;
        return FMath::Lerp(-150.0f, -300.0f, LocalT);
    }
    else
    {
        float LocalT = (T - 0.85f) / 0.15f;
        LocalT = FMath::Pow(LocalT, 1.3f);
        return FMath::Lerp(-300.0f, -500.0f, LocalT);
    }
}

uint32 ATerrainGenerator::Hash(int32 X, int32 Y, int32 Seed) const
{
    uint32 H = (uint32)(X * 374761393 + Y * 668265263 + Seed * 1013904223);
    H = (H ^ (H >> 13)) * 1274126177;
    return H ^ (H >> 16);
}

float ATerrainGenerator::HashFloat(int32 X, int32 Y, int32 Seed) const
{
    return (float)(Hash(X, Y, Seed) & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}

float ATerrainGenerator::Noise2D(float X, float Y, int32 Seed) const
{
    int32 Xi = FMath::FloorToInt(X);
    int32 Yi = FMath::FloorToInt(Y);
    float Xf = X - Xi;
    float Yf = Y - Yi;

    float N00 = HashFloat(Xi, Yi, Seed) * 2.0f - 1.0f;
    float N10 = HashFloat(Xi + 1, Yi, Seed) * 2.0f - 1.0f;
    float N01 = HashFloat(Xi, Yi + 1, Seed) * 2.0f - 1.0f;
    float N11 = HashFloat(Xi + 1, Yi + 1, Seed) * 2.0f - 1.0f;

    float U = Xf * Xf * Xf * (Xf * (Xf * 6.0f - 15.0f) + 10.0f);
    float V = Yf * Yf * Yf * (Yf * (Yf * 6.0f - 15.0f) + 10.0f);

    float Nx0 = FMath::Lerp(N00, N10, U);
    float Nx1 = FMath::Lerp(N01, N11, U);

    return FMath::Lerp(Nx0, Nx1, V);
}

float ATerrainGenerator::FBM(float X, float Y, int32 Octaves, float Freq, int32 Seed) const
{
    float Total = 0.0f;
    float Amplitude = 1.0f;
    float MaxVal = 0.0f;

    for (int32 i = 0; i < Octaves; i++)
    {
        Total += Noise2D(X * Freq, Y * Freq, Seed + i * 1000) * Amplitude;
        MaxVal += Amplitude;
        Amplitude *= 0.5f;
        Freq *= 2.0f;
    }

    return Total / MaxVal;
}

float ATerrainGenerator::Smoothstep(float A, float B, float T) const
{
    float X = FMath::Clamp((T - A) / (B - A), 0.0f, 1.0f);
    return X * X * (3.0f - 2.0f * X);
}
