// Implementation of ABiomeDebugHUD: finds the terrain managers in the world and renders
// the position, biome and chunk readouts to the canvas every frame.

#include "BiomeDebugHUD.h"
#include "ChunkManager.h"
#include "BiomeManager.h"
#include "TerrainGenerator.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

ABiomeDebugHUD::ABiomeDebugHUD()
{
}

void ABiomeDebugHUD::BeginPlay()
{
    Super::BeginPlay();
    FindManagers();
}

void ABiomeDebugHUD::FindManagers()
{
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AChunkManager::StaticClass(), FoundActors);
    if (FoundActors.Num() > 0)
    {
        ChunkManager = Cast<AChunkManager>(FoundActors[0]);
        if (ChunkManager)
        {
            BiomeManager = ChunkManager->GetBiomeManager();
            TerrainGenerator = ChunkManager->TerrainGenerator;
        }
    }
}

void ABiomeDebugHUD::DrawHUD()
{
    Super::DrawHUD();

    if (bShowDebug)
    {
        DrawDebugInfo();
    }
}

void ABiomeDebugHUD::DrawDebugInfo()
{
    if (!Canvas)
    {
        return;
    }

    if (!ChunkManager)
    {
        FindManagers();
    }

    APawn* PlayerPawn = nullptr;
    APlayerController* PC = GetOwningPlayerController();
    if (PC)
    {
        PlayerPawn = PC->GetPawn();
    }

    FVector PlayerPos = FVector::ZeroVector;
    if (PlayerPawn)
    {
        PlayerPos = PlayerPawn->GetActorLocation();
    }

    float X = 20.0f;
    float Y = 20.0f;
    float LineHeight = 18.0f * TextScale;

    DrawText(TEXT("=== TERRAIN DEBUG ==="), HeaderColor, X, Y, nullptr, TextScale * 1.2f);
    Y += LineHeight * 1.5f;

    DrawText(FString::Printf(TEXT("Position: X=%.1f  Y=%.1f  Z=%.1f"),
        PlayerPos.X, PlayerPos.Y, PlayerPos.Z), TextColor, X, Y, nullptr, TextScale);
    Y += LineHeight;

    float SeaLevel = 0.0f;
    if (ChunkManager)
    {
        SeaLevel = ChunkManager->WorldSettings.SeaLevel;
    }
    DrawText(FString::Printf(TEXT("Sea Level: %.1f"), SeaLevel), TextColor, X, Y, nullptr, TextScale);
    Y += LineHeight;

    float RelativeHeight = PlayerPos.Z - SeaLevel;
    FString HeightStatus = RelativeHeight >= 0 ? TEXT("Above") : TEXT("Below");
    DrawText(FString::Printf(TEXT("Height vs Sea: %.1f (%s)"), RelativeHeight, *HeightStatus),
        TextColor, X, Y, nullptr, TextScale);
    Y += LineHeight;

    if (TerrainGenerator)
    {
        float TerrainHeight = TerrainGenerator->GetHeightAt(PlayerPos.X, PlayerPos.Y);
        DrawText(FString::Printf(TEXT("Terrain Height: %.1f"), TerrainHeight),
            TextColor, X, Y, nullptr, TextScale);
        Y += LineHeight;

        float HeightAboveTerrain = PlayerPos.Z - TerrainHeight;
        DrawText(FString::Printf(TEXT("Above Terrain: %.1f"), HeightAboveTerrain),
            TextColor, X, Y, nullptr, TextScale);
        Y += LineHeight;
    }

    Y += LineHeight * 0.5f;

    DrawText(TEXT("=== BIOME INFO ==="), HeaderColor, X, Y, nullptr, TextScale * 1.2f);
    Y += LineHeight * 1.5f;

    if (BiomeManager)
    {
        FBiomeQueryResult Query = BiomeManager->QueryBiomeAt(PlayerPos.X, PlayerPos.Y);

        FString LocationType = Query.bIsOnLand ? TEXT("On Land") : TEXT("In Water");
        if (Query.bIsUnderwater)
        {
            LocationType = TEXT("Underwater");
        }
        DrawText(FString::Printf(TEXT("Location: %s"), *LocationType), TextColor, X, Y, nullptr, TextScale);
        Y += LineHeight;

        if (Query.IslandBiome)
        {
            DrawText(FString::Printf(TEXT("Island Biome: %s"), *Query.IslandBiome->BiomeName.ToString()),
                TextColor, X, Y, nullptr, TextScale);
            Y += LineHeight;

            FString BiomeTypeName;
            switch (Query.IslandBiome->BiomeType)
            {
            case EIslandBiome::Empty: BiomeTypeName = TEXT("Empty"); break;
            case EIslandBiome::Tropical: BiomeTypeName = TEXT("Tropical"); break;
            case EIslandBiome::Volcanic: BiomeTypeName = TEXT("Volcanic"); break;
            case EIslandBiome::Jungle: BiomeTypeName = TEXT("Jungle"); break;
            case EIslandBiome::Rocky: BiomeTypeName = TEXT("Rocky"); break;
            case EIslandBiome::Desert: BiomeTypeName = TEXT("Desert"); break;
            case EIslandBiome::Mangrove: BiomeTypeName = TEXT("Mangrove"); break;
            default: BiomeTypeName = TEXT("Unknown"); break;
            }
            DrawText(FString::Printf(TEXT("  Type: %s"), *BiomeTypeName), TextColor, X, Y, nullptr, TextScale);
            Y += LineHeight;
        }
        else
        {
            DrawText(TEXT("Island Biome: Empty (no asset)"), FColor(128, 128, 128), X, Y, nullptr, TextScale);
            Y += LineHeight;
        }

        if (Query.ElevationZone)
        {
            DrawText(FString::Printf(TEXT("Elevation Zone: %s"), *Query.ElevationZone->BiomeName.ToString()),
                TextColor, X, Y, nullptr, TextScale);
            Y += LineHeight;
        }
        else
        {
            DrawText(TEXT("Elevation Zone: Empty (no asset)"), FColor(128, 128, 128), X, Y, nullptr, TextScale);
            Y += LineHeight;
        }

        if (Query.OceanBiome)
        {
            DrawText(FString::Printf(TEXT("Ocean Biome: %s"), *Query.OceanBiome->BiomeName.ToString()),
                TextColor, X, Y, nullptr, TextScale);
            Y += LineHeight;
        }
        else if (!Query.bIsOnLand)
        {
            DrawText(TEXT("Ocean Biome: Empty (no asset)"), FColor(128, 128, 128), X, Y, nullptr, TextScale);
            Y += LineHeight;
        }

        DrawText(FString::Printf(TEXT("Distance to Shore: %.1f"), Query.DistanceToShore),
            TextColor, X, Y, nullptr, TextScale);
        Y += LineHeight;

        if (!Query.bIsOnLand || Query.bIsUnderwater)
        {
            DrawText(FString::Printf(TEXT("Water Depth: %.1f"), Query.WaterDepth),
                TextColor, X, Y, nullptr, TextScale);
            Y += LineHeight;
        }

        DrawText(FString::Printf(TEXT("Slope Angle: %.1f°"), Query.SlopeAngle),
            TextColor, X, Y, nullptr, TextScale);
        Y += LineHeight;
    }
    else
    {
        DrawText(TEXT("BiomeManager not found!"), FColor::Red, X, Y, nullptr, TextScale);
        Y += LineHeight;
    }

    Y += LineHeight * 0.5f;

    DrawText(TEXT("=== CHUNK INFO ==="), HeaderColor, X, Y, nullptr, TextScale * 1.2f);
    Y += LineHeight * 1.5f;

    if (ChunkManager)
    {
        float ChunkSize = ChunkManager->WorldSettings.ChunkSize;
        int32 ChunkX = FMath::FloorToInt(PlayerPos.X / ChunkSize);
        int32 ChunkY = FMath::FloorToInt(PlayerPos.Y / ChunkSize);

        DrawText(FString::Printf(TEXT("Current Chunk: (%d, %d)"), ChunkX, ChunkY),
            TextColor, X, Y, nullptr, TextScale);
        Y += LineHeight;

        DrawText(FString::Printf(TEXT("Chunk Size: %.0f"), ChunkSize),
            TextColor, X, Y, nullptr, TextScale);
        Y += LineHeight;

        if (TerrainGenerator)
        {
            FIslandData Island;
            if (TerrainGenerator->GetIslandAt(PlayerPos.X, PlayerPos.Y, Island))
            {
                DrawText(TEXT("On Island: Yes"), FColor::Green, X, Y, nullptr, TextScale);
                Y += LineHeight;

                DrawText(FString::Printf(TEXT("  Center: (%.0f, %.0f)"), Island.Center.X, Island.Center.Y),
                    TextColor, X, Y, nullptr, TextScale);
                Y += LineHeight;

                DrawText(FString::Printf(TEXT("  Radius: %.0f"), Island.Radius),
                    TextColor, X, Y, nullptr, TextScale);
                Y += LineHeight;

                DrawText(FString::Printf(TEXT("  Type: %d"), Island.IslandType),
                    TextColor, X, Y, nullptr, TextScale);
                Y += LineHeight;

                if (Island.bIsSpawnIsland)
                {
                    DrawText(TEXT("  (Spawn Island)"), FColor::Cyan, X, Y, nullptr, TextScale);
                    Y += LineHeight;
                }
            }
            else
            {
                DrawText(TEXT("On Island: No (Ocean)"), FColor::Blue, X, Y, nullptr, TextScale);
                Y += LineHeight;
            }
        }
    }
    else
    {
        DrawText(TEXT("ChunkManager not found!"), FColor::Red, X, Y, nullptr, TextScale);
    }

    Y += LineHeight;
    DrawText(TEXT("Press F1 to toggle debug"), FColor(128, 128, 128), X, Y, nullptr, TextScale * 0.8f);
}
