// Implementation of AFoliageManager: generates and releases per-chunk foliage around the
// player, rejecting placements by height range and slope, and reusing one HISM component
// per static mesh.

#include "FoliageManager.h"
#include "ChunkManager.h"
#include "BiomeManager.h"
#include "TerrainGenerator.h"
#include "Kismet/GameplayStatics.h"

AFoliageManager::AFoliageManager()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.1f;
}

void AFoliageManager::BeginPlay()
{
    Super::BeginPlay();

    if (!ChunkManager)
    {
        TArray<AActor*> Found;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChunkManager::StaticClass(), Found);
        if (Found.Num() > 0)
        {
            ChunkManager = Cast<AChunkManager>(Found[0]);
        }
    }

    if (ChunkManager)
    {
        BiomeManager = ChunkManager->GetBiomeManager();
        TerrainGenerator = ChunkManager->TerrainGenerator;
    }

    if (BiomeManager)
    {
        UE_LOG(LogTemp, Log, TEXT("FoliageManager initialized with BiomeManager"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("FoliageManager: No BiomeManager found!"));
    }
}

void AFoliageManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    TimeSinceUpdate += DeltaTime;
    if (TimeSinceUpdate < 0.5f)
    {
        return;
    }
    TimeSinceUpdate = 0.0f;

    UpdateFoliageChunks();
}

void AFoliageManager::UpdateFoliageChunks()
{
    if (!ChunkManager || !BiomeManager || !TerrainGenerator)
    {
        return;
    }

    FVector PlayerPos = GetPlayerPosition();
    float ChunkSize = ChunkManager->WorldSettings.ChunkSize;

    FIntPoint PlayerChunk;
    PlayerChunk.X = FMath::FloorToInt(PlayerPos.X / ChunkSize);
    PlayerChunk.Y = FMath::FloorToInt(PlayerPos.Y / ChunkSize);

    for (int32 DY = -FoliageLoadRadius; DY <= FoliageLoadRadius; DY++)
    {
        for (int32 DX = -FoliageLoadRadius; DX <= FoliageLoadRadius; DX++)
        {
            FIntPoint Coord(PlayerChunk.X + DX, PlayerChunk.Y + DY);

            if (!ChunkFoliage.Contains(Coord) || !ChunkFoliage[Coord].bIsGenerated)
            {
                GenerateFoliageForChunk(Coord);
            }
        }
    }

    TArray<FIntPoint> ToRemove;
    for (auto& Pair : ChunkFoliage)
    {
        int32 Dist = FMath::Max(FMath::Abs(Pair.Key.X - PlayerChunk.X), FMath::Abs(Pair.Key.Y - PlayerChunk.Y));
        if (Dist > FoliageLoadRadius + 1)
        {
            ToRemove.Add(Pair.Key);
        }
    }

    for (const FIntPoint& Coord : ToRemove)
    {
        ClearFoliageForChunk(Coord);
    }

    LastPlayerChunk = PlayerChunk;
}

void AFoliageManager::GenerateFoliageForChunk(FIntPoint ChunkCoord)
{
    if (!BiomeManager || !TerrainGenerator)
    {
        return;
    }

    float ChunkSize = ChunkManager->WorldSettings.ChunkSize;
    float ChunkWorldX = ChunkCoord.X * ChunkSize;
    float ChunkWorldY = ChunkCoord.Y * ChunkSize;

    float CenterX = ChunkWorldX + ChunkSize * 0.5f;
    float CenterY = ChunkWorldY + ChunkSize * 0.5f;
    FBiomeQueryResult BiomeQuery = BiomeManager->QueryBiomeAt(CenterX, CenterY);

    TArray<FFoliageConfig> FoliageConfigs;
    if (BiomeQuery.IslandBiome)
    {
        FoliageConfigs = BiomeQuery.IslandBiome->Foliage;
    }

    if (FoliageConfigs.Num() == 0)
    {
        FFoliageChunkData& Data = ChunkFoliage.FindOrAdd(ChunkCoord);
        Data.bIsGenerated = true;
        return;
    }

    FFoliageChunkData& Data = ChunkFoliage.FindOrAdd(ChunkCoord);

    for (const FFoliageConfig& Config : FoliageConfigs)
    {
        if (!Config.Mesh)
        {
            continue;
        }

        UHierarchicalInstancedStaticMeshComponent* HISM = GetOrCreateHISM(Config.Mesh);
        if (!HISM)
        {
            continue;
        }

        float AreaFactor = (ChunkSize / 100.0f) * (ChunkSize / 100.0f);
        int32 NumInstances = FMath::RoundToInt(Config.Density * AreaFactor);

        int32 SpawnSeed = Seed + ChunkCoord.X * 73856093 + ChunkCoord.Y * 19349663;

        for (int32 i = 0; i < NumInstances; i++)
        {
            float RandX = HashFloat(i, SpawnSeed, 1);
            float RandY = HashFloat(i, SpawnSeed, 2);

            float WorldX = ChunkWorldX + RandX * ChunkSize;
            float WorldY = ChunkWorldY + RandY * ChunkSize;

            float Height = TerrainGenerator->GetHeightAt(WorldX, WorldY);

            float SeaLevel = TerrainGenerator->WorldSettings.SeaLevel;
            float HeightAboveSea = Height - SeaLevel;

            if (HeightAboveSea < Config.HeightRange.X || HeightAboveSea > Config.HeightRange.Y)
            {
                continue;
            }

            FVector Normal = TerrainGenerator->GetNormalAt(WorldX, WorldY);
            float SlopeAngle = FMath::RadiansToDegrees(FMath::Acos(Normal.Z));
            if (SlopeAngle > Config.MaxSlopeAngle)
            {
                continue;
            }

            float Scale = FMath::Lerp(Config.ScaleRange.X, Config.ScaleRange.Y, HashFloat(i, SpawnSeed, 3));

            float Yaw = Config.bRandomYaw ? HashFloat(i, SpawnSeed, 4) * 360.0f : 0.0f;

            FTransform Transform;
            Transform.SetLocation(FVector(WorldX, WorldY, Height));

            FRotator Rotation(0, Yaw, 0);
            if (Config.bAlignToSlope)
            {
                FVector Up = Normal;
                FVector Forward = FVector::CrossProduct(FVector::RightVector, Up).GetSafeNormal();
                FVector Right = FVector::CrossProduct(Up, Forward);
                Rotation = FRotationMatrix::MakeFromXZ(Forward, Up).Rotator();
                Rotation.Yaw += Yaw;
            }
            Transform.SetRotation(Rotation.Quaternion());
            Transform.SetScale3D(FVector(Scale));

            HISM->AddInstance(Transform, true);
        }

        Data.MeshComponents.AddUnique(HISM);
    }

    Data.bIsGenerated = true;

    UE_LOG(LogTemp, Log, TEXT("Foliage generated for chunk (%d, %d)"), ChunkCoord.X, ChunkCoord.Y);
}

void AFoliageManager::ClearFoliageForChunk(FIntPoint ChunkCoord)
{
    ChunkFoliage.Remove(ChunkCoord);
}

void AFoliageManager::ClearAllFoliage()
{
    for (auto& Pair : MeshToHISM)
    {
        if (Pair.Value)
        {
            Pair.Value->ClearInstances();
        }
    }
    ChunkFoliage.Empty();
}

UHierarchicalInstancedStaticMeshComponent* AFoliageManager::GetOrCreateHISM(UStaticMesh* Mesh)
{
    if (!Mesh)
    {
        return nullptr;
    }

    if (MeshToHISM.Contains(Mesh))
    {
        return MeshToHISM[Mesh];
    }

    UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
    HISM->SetStaticMesh(Mesh);
    HISM->SetMobility(EComponentMobility::Static);
    HISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    HISM->SetCollisionResponseToAllChannels(ECR_Block);
    HISM->RegisterComponent();
    HISM->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

    MeshToHISM.Add(Mesh, HISM);

    return HISM;
}

FVector AFoliageManager::GetPlayerPosition() const
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC && PC->GetPawn())
    {
        return PC->GetPawn()->GetActorLocation();
    }
    return FVector::ZeroVector;
}

uint32 AFoliageManager::Hash(int32 X, int32 Y, int32 S) const
{
    uint32 H = (uint32)(X * 374761393 + Y * 668265263 + S * 1013904223);
    H = (H ^ (H >> 13)) * 1274126177;
    return H ^ (H >> 16);
}

float AFoliageManager::HashFloat(int32 X, int32 Y, int32 S) const
{
    return (float)(Hash(X, Y, S) & 0x7FFFFFFF) / (float)0x7FFFFFFF;
}
