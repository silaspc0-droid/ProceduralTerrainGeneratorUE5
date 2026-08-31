// Implementation of AChunkManager: spawns the terrain and biome services, tracks which
// chunk the player occupies, and queues nearby chunks for generation while unloading
// distant ones.

#include "ChunkManager.h"
#include "TerrainGenerator.h"
#include "TerrainChunk.h"
#include "BiomeManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AChunkManager::AChunkManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AChunkManager::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("=== ChunkManager Starting ==="));
    UE_LOG(LogTemp, Warning, TEXT("TerrainMaterial: %s"), TerrainMaterial ? *TerrainMaterial->GetName() : TEXT("NULL - TERRAIN WILL BE BLACK!"));

    SpawnTerrainGenerator();
    SpawnBiomeManager();

    if (!ChunkClass)
    {
        ChunkClass = ATerrainChunk::StaticClass();
    }

    bFirstUpdate = true;
    UpdateChunks();
}

void AChunkManager::SpawnBiomeManager()
{
    FActorSpawnParameters Params;
    Params.Owner = this;

    BiomeManager = GetWorld()->SpawnActor<ABiomeManager>(
        ABiomeManager::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        Params
    );

    if (BiomeManager)
    {
        BiomeManager->TerrainGenerator = TerrainGenerator;
        BiomeManager->Seed = WorldSettings.Seed;

        BiomeManager->IslandBiomes = IslandBiomes;
        BiomeManager->OceanBiomes = OceanBiomes;
        BiomeManager->ElevationZones = ElevationZones;

        UE_LOG(LogTemp, Warning, TEXT("BiomeManager spawned with %d island biomes, %d ocean biomes"),
            IslandBiomes.Num(), OceanBiomes.Num());
    }
}

void AChunkManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    int32 Generated = 0;
    while (GenerationQueue.Num() > 0 && Generated < MaxChunksPerFrame)
    {
        FChunkCoord Coord = GenerationQueue[0];
        GenerationQueue.RemoveAt(0);

        if (ATerrainChunk** Found = LoadedChunks.Find(Coord))
        {
            if (*Found && !(*Found)->IsGenerated())
            {
                (*Found)->GenerateMesh();
                Generated++;
            }
        }
    }

    TimeSinceUpdate += DeltaTime;
    if (TimeSinceUpdate >= 0.3f)
    {
        TimeSinceUpdate = 0.0f;
        UpdateChunks();
    }
}

void AChunkManager::SpawnTerrainGenerator()
{
    FActorSpawnParameters Params;
    Params.Owner = this;

    TerrainGenerator = GetWorld()->SpawnActor<ATerrainGenerator>(
        ATerrainGenerator::StaticClass(),
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        Params
    );

    if (TerrainGenerator)
    {
        TerrainGenerator->WorldSettings = WorldSettings;

        float TestHeight = TerrainGenerator->GetHeightAt(0, 0);
        UE_LOG(LogTemp, Warning, TEXT("Height at origin: %.1f"), TestHeight);
    }
}

void AChunkManager::UpdateChunks()
{
    FVector PlayerPos = GetPlayerPosition();
    FChunkCoord CurrentChunk = WorldToChunk(PlayerPos);

    if (!bFirstUpdate && CurrentChunk == LastPlayerChunk && LoadedChunks.Num() > 0)
    {
        return;
    }

    bFirstUpdate = false;
    LastPlayerChunk = CurrentChunk;

    TSet<FChunkCoord> Desired;
    int32 Rad = WorldSettings.LoadRadius;

    for (int32 Y = -Rad; Y <= Rad; Y++)
    {
        for (int32 X = -Rad; X <= Rad; X++)
        {
            if (X * X + Y * Y <= Rad * Rad)
            {
                Desired.Add(FChunkCoord(CurrentChunk.X + X, CurrentChunk.Y + Y));
            }
        }
    }

    TArray<FChunkCoord> ToUnload;
    int32 UnloadRad = WorldSettings.UnloadRadius;

    for (auto& Pair : LoadedChunks)
    {
        int32 Dx = Pair.Key.X - CurrentChunk.X;
        int32 Dy = Pair.Key.Y - CurrentChunk.Y;
        if (Dx * Dx + Dy * Dy > UnloadRad * UnloadRad)
        {
            ToUnload.Add(Pair.Key);
        }
    }

    for (const FChunkCoord& Coord : ToUnload)
    {
        UnloadChunk(Coord);
    }

    TArray<FChunkCoord> ToLoad;
    for (const FChunkCoord& Coord : Desired)
    {
        if (!LoadedChunks.Contains(Coord))
        {
            ToLoad.Add(Coord);
        }
    }

    ToLoad.Sort([&CurrentChunk](const FChunkCoord& A, const FChunkCoord& B)
        {
            int32 DA = (A.X - CurrentChunk.X) * (A.X - CurrentChunk.X) +
                (A.Y - CurrentChunk.Y) * (A.Y - CurrentChunk.Y);
            int32 DB = (B.X - CurrentChunk.X) * (B.X - CurrentChunk.X) +
                (B.Y - CurrentChunk.Y) * (B.Y - CurrentChunk.Y);
            return DA < DB;
        });

    for (const FChunkCoord& Coord : ToLoad)
    {
        LoadChunk(Coord);
    }
}

void AChunkManager::LoadChunk(FChunkCoord Coord)
{
    if (LoadedChunks.Contains(Coord) || !TerrainGenerator || !ChunkClass)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.Owner = this;

    ATerrainChunk* Chunk = GetWorld()->SpawnActor<ATerrainChunk>(
        ChunkClass,
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        Params
    );

    if (Chunk)
    {
        Chunk->Initialize(Coord, TerrainGenerator, TerrainMaterial, BiomeManager);
        LoadedChunks.Add(Coord, Chunk);
        GenerationQueue.Add(Coord);

        if (!TerrainMaterial)
        {
            UE_LOG(LogTemp, Error, TEXT("LoadChunk %s: TerrainMaterial is NULL!"), *Coord.ToString());
        }
    }
}

void AChunkManager::UnloadChunk(FChunkCoord Coord)
{
    if (ATerrainChunk** Found = LoadedChunks.Find(Coord))
    {
        if (*Found)
        {
            (*Found)->Destroy();
        }
        LoadedChunks.Remove(Coord);
        GenerationQueue.Remove(Coord);
    }
}

FVector AChunkManager::GetPlayerPosition() const
{
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    return Player ? Player->GetActorLocation() : FVector::ZeroVector;
}

FChunkCoord AChunkManager::WorldToChunk(const FVector& Pos) const
{
    return FChunkCoord(
        FMath::FloorToInt(Pos.X / WorldSettings.ChunkSize),
        FMath::FloorToInt(Pos.Y / WorldSettings.ChunkSize)
    );
}

float AChunkManager::GetHeightAt(float WorldX, float WorldY) const
{
    if (TerrainGenerator)
    {
        return TerrainGenerator->GetHeightAt(WorldX, WorldY);
    }
    return WorldSettings.SeaLevel;
}
