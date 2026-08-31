// Implementation of ATerrainChunk: samples the heightfield into a vertex grid, assigns each
// vertex a biome material, and splits triangles into base sections by majority vote.
// Triangles near a biome border are duplicated into overlay sections offset +1 in Z, with
// vertex alpha fading out over FadeRadius cells, so the seam blends instead of stepping.
// Each biome material must use Blend Mode = Masked with VertexColor.A -> DitherTemporalAA
// -> Opacity Mask for the overlay to read as a smooth blend.

#include "TerrainChunk.h"
#include "TerrainGenerator.h"
#include "BiomeManager.h"

ATerrainChunk::ATerrainChunk()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
    RootComponent = MeshComponent;

    MeshComponent->bUseComplexAsSimpleCollision = true;
    MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    MeshComponent->SetCollisionObjectType(ECC_WorldStatic);
    MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
}

void ATerrainChunk::Initialize(FChunkCoord InCoord, ATerrainGenerator* InGenerator, UMaterialInterface* InMaterial, ABiomeManager* InBiomeManager)
{
    Coord = InCoord;
    Generator = InGenerator;
    TerrainMaterial = InMaterial;
    BiomeManager = InBiomeManager;

    if (Generator)
    {
        float ChunkSize = Generator->WorldSettings.ChunkSize;
        SetActorLocation(FVector(Coord.X * ChunkSize, Coord.Y * ChunkSize, 0.0f));
    }
}

void ATerrainChunk::GenerateMesh()
{
    if (!Generator || bIsGenerated)
        return;

    const FTerrainWorldSettings& Settings = Generator->WorldSettings;
    const int32 Res = Settings.ChunkResolution;
    const float ChunkSize = Settings.ChunkSize;
    const float Step = ChunkSize / (float)(Res - 1);
    const float ChunkWorldX = Coord.X * ChunkSize;
    const float ChunkWorldY = Coord.Y * ChunkSize;
    const int32 VertCount = Res * Res;

    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FProcMeshTangent> Tangents;
    TArray<UMaterialInterface*> VertexMaterial;

    Vertices.SetNum(VertCount);
    Normals.SetNum(VertCount);
    UVs.SetNum(VertCount);
    Tangents.SetNum(VertCount);
    VertexMaterial.SetNum(VertCount);

    for (int32 Y = 0; Y < Res; Y++)
    {
        for (int32 X = 0; X < Res; X++)
        {
            int32 Idx = Y * Res + X;
            float LocalX = X * Step;
            float LocalY = Y * Step;
            float WorldX = ChunkWorldX + LocalX;
            float WorldY = ChunkWorldY + LocalY;

            float Height = Generator->GetHeightAt(WorldX, WorldY);
            Vertices[Idx] = FVector(LocalX, LocalY, Height);
            UVs[Idx] = FVector2D(WorldX / 100.0f, WorldY / 100.0f);

            float Delta = Step * 0.5f;
            float HL = Generator->GetHeightAt(WorldX - Delta, WorldY);
            float HR = Generator->GetHeightAt(WorldX + Delta, WorldY);
            float HD = Generator->GetHeightAt(WorldX, WorldY - Delta);
            float HU = Generator->GetHeightAt(WorldX, WorldY + Delta);

            FVector Normal(HL - HR, HD - HU, 2.0f * Delta);
            Normal.Normalize();
            if (Normal.Z < 0) Normal = -Normal;
            Normals[Idx] = Normal;

            Tangents[Idx] = FProcMeshTangent(FVector(1, 0, 0), false);

            VertexMaterial[Idx] = TerrainMaterial;
            if (BiomeManager)
            {
                FBiomeQueryResult Q = BiomeManager->QueryBiomeAt(WorldX, WorldY);
                if (Q.IslandBiome && Q.IslandBiome->TerrainMaterial)
                    VertexMaterial[Idx] = Q.IslandBiome->TerrainMaterial;
                else if (Q.OceanBiome && Q.OceanBiome->TerrainMaterial)
                    VertexMaterial[Idx] = Q.OceanBiome->TerrainMaterial;
            }
        }
    }

    TArray<bool> IsBorderVertex;
    IsBorderVertex.SetNumZeroed(VertCount);

    const int32 FadeRadius = 2;

    for (int32 Y = 0; Y < Res; Y++)
    {
        for (int32 X = 0; X < Res; X++)
        {
            int32 Idx = Y * Res + X;
            UMaterialInterface* MyMat = VertexMaterial[Idx];

            for (int32 D = 0; D < 4; D++)
            {
                int32 NX = X + (D == 0 ? 1 : (D == 1 ? -1 : 0));
                int32 NY = Y + (D == 2 ? 1 : (D == 3 ? -1 : 0));
                if (NX < 0 || NX >= Res || NY < 0 || NY >= Res) continue;

                int32 NIdx = NY * Res + NX;
                if (VertexMaterial[NIdx] != MyMat)
                {
                    IsBorderVertex[Idx] = true;
                    break;
                }
            }
        }
    }

    TArray<int32> DistToBorder;
    DistToBorder.SetNum(VertCount);
    for (int32 i = 0; i < VertCount; i++)
    {
        DistToBorder[i] = IsBorderVertex[i] ? 0 : FadeRadius + 1;
    }

    for (int32 Pass = 0; Pass < FadeRadius; Pass++)
    {
        for (int32 Y = 0; Y < Res; Y++)
        {
            for (int32 X = 0; X < Res; X++)
            {
                int32 Idx = Y * Res + X;
                if (DistToBorder[Idx] > Pass) continue;

                for (int32 D = 0; D < 4; D++)
                {
                    int32 NX = X + (D == 0 ? 1 : (D == 1 ? -1 : 0));
                    int32 NY = Y + (D == 2 ? 1 : (D == 3 ? -1 : 0));
                    if (NX < 0 || NX >= Res || NY < 0 || NY >= Res) continue;
                    int32 NIdx = NY * Res + NX;
                    DistToBorder[NIdx] = FMath::Min(DistToBorder[NIdx], Pass + 1);
                }
            }
        }
    }

    TArray<FColor> BaseColors;
    BaseColors.SetNum(VertCount);
    for (int32 i = 0; i < VertCount; i++)
    {
        BaseColors[i] = FColor::White;
    }

    TMap<UMaterialInterface*, TArray<int32>> BaseMaterialTriangles;

    struct FOverlayData
    {
        TArray<int32> Triangles;
    };
    TMap<UMaterialInterface*, FOverlayData> OverlayMaterialTriangles;

    for (int32 Y = 0; Y < Res - 1; Y++)
    {
        for (int32 X = 0; X < Res - 1; X++)
        {
            int32 BL = Y * Res + X;
            int32 BR = BL + 1;
            int32 TL = (Y + 1) * Res + X;
            int32 TR = TL + 1;

            int32 QuadTris[2][3] = {
                { BL, TL, BR },
                { BR, TL, TR }
            };

            for (int32 T = 0; T < 2; T++)
            {
                int32 I0 = QuadTris[T][0];
                int32 I1 = QuadTris[T][1];
                int32 I2 = QuadTris[T][2];

                UMaterialInterface* M0 = VertexMaterial[I0];
                UMaterialInterface* M1 = VertexMaterial[I1];
                UMaterialInterface* M2 = VertexMaterial[I2];

                UMaterialInterface* Winner = M0;
                if (M1 == M2) Winner = M1;
                else if (M0 == M2) Winner = M0;
                else if (M0 == M1) Winner = M0;
                if (!Winner) Winner = TerrainMaterial;
                if (!Winner) continue;

                BaseMaterialTriangles.FindOrAdd(Winner).Append({ I0, I1, I2 });

                bool bIsBorder = (M0 != M1 || M1 != M2);

                if (bIsBorder)
                {
                    UMaterialInterface* Loser = nullptr;
                    if (M0 != Winner) Loser = M0;
                    else if (M1 != Winner) Loser = M1;
                    else if (M2 != Winner) Loser = M2;

                    if (Loser)
                    {
                        OverlayMaterialTriangles.FindOrAdd(Loser).Triangles.Append({ I0, I1, I2 });
                    }

                    OverlayMaterialTriangles.FindOrAdd(Winner).Triangles.Append({ I0, I1, I2 });
                }
            }
        }
    }

    for (int32 Y = 0; Y < Res - 1; Y++)
    {
        for (int32 X = 0; X < Res - 1; X++)
        {
            int32 BL = Y * Res + X;
            int32 BR = BL + 1;
            int32 TL = (Y + 1) * Res + X;
            int32 TR = TL + 1;

            int32 QuadTris[2][3] = {
                { BL, TL, BR },
                { BR, TL, TR }
            };

            for (int32 T = 0; T < 2; T++)
            {
                int32 I0 = QuadTris[T][0];
                int32 I1 = QuadTris[T][1];
                int32 I2 = QuadTris[T][2];

                UMaterialInterface* M0 = VertexMaterial[I0];
                UMaterialInterface* M1 = VertexMaterial[I1];
                UMaterialInterface* M2 = VertexMaterial[I2];

                if (M0 != M1 || M1 != M2) continue;

                bool bNearBorder = (DistToBorder[I0] <= FadeRadius ||
                    DistToBorder[I1] <= FadeRadius ||
                    DistToBorder[I2] <= FadeRadius);

                if (bNearBorder)
                {
                    for (auto& Pair : OverlayMaterialTriangles)
                    {
                        if (Pair.Key != M0)
                        {
                            Pair.Value.Triangles.Append({ I0, I1, I2 });
                        }
                    }
                }
            }
        }
    }

    TArray<FVector> OverlayVertices;
    TArray<FColor> OverlayColors;
    OverlayVertices.SetNum(VertCount);
    OverlayColors.SetNum(VertCount);

    for (int32 i = 0; i < VertCount; i++)
    {
        OverlayVertices[i] = Vertices[i] + FVector(0, 0, 1.0f);

        float Dist = (float)DistToBorder[i];
        float Alpha;
        if (Dist > FadeRadius)
        {
            Alpha = 0.0f;
        }
        else
        {
            Alpha = 1.0f - (Dist / (float)(FadeRadius + 1));
        }

        OverlayColors[i] = FColor(255, 255, 255, (uint8)FMath::Clamp(FMath::RoundToInt(Alpha * 255.0f), 0, 255));
    }

    int32 SectionIdx = 0;

    for (auto& Pair : BaseMaterialTriangles)
    {
        UMaterialInterface* Mat = Pair.Key;
        TArray<int32>& Tris = Pair.Value;
        if (Tris.Num() == 0 || !Mat) continue;

        MeshComponent->CreateMeshSection(SectionIdx, Vertices, Tris, Normals, UVs, BaseColors, Tangents, true);
        MeshComponent->SetMaterial(SectionIdx, Mat);
        SectionIdx++;
    }

    for (auto& Pair : OverlayMaterialTriangles)
    {
        UMaterialInterface* Mat = Pair.Key;
        TArray<int32>& Tris = Pair.Value.Triangles;
        if (Tris.Num() == 0 || !Mat) continue;

        TSet<int64> Seen;
        TArray<int32> UniqueTris;
        for (int32 i = 0; i < Tris.Num(); i += 3)
        {
            int64 Key = (int64)Tris[i] * 1000000LL + (int64)Tris[i + 1] * 1000LL + (int64)Tris[i + 2];
            if (!Seen.Contains(Key))
            {
                Seen.Add(Key);
                UniqueTris.Add(Tris[i]);
                UniqueTris.Add(Tris[i + 1]);
                UniqueTris.Add(Tris[i + 2]);
            }
        }

        if (UniqueTris.Num() == 0) continue;

        MeshComponent->CreateMeshSection(SectionIdx, OverlayVertices, UniqueTris, Normals, UVs, OverlayColors, Tangents, false);
        MeshComponent->SetMaterial(SectionIdx, Mat);
        SectionIdx++;
    }

    if (SectionIdx == 0)
    {
        TArray<int32> AllTris;
        AllTris.Reserve((Res - 1) * (Res - 1) * 6);
        for (int32 Y = 0; Y < Res - 1; Y++)
        {
            for (int32 X = 0; X < Res - 1; X++)
            {
                int32 BL = Y * Res + X;
                int32 BR = BL + 1;
                int32 TL = (Y + 1) * Res + X;
                int32 TR = TL + 1;
                AllTris.Add(BL); AllTris.Add(TL); AllTris.Add(BR);
                AllTris.Add(BR); AllTris.Add(TL); AllTris.Add(TR);
            }
        }
        MeshComponent->CreateMeshSection(0, Vertices, AllTris, Normals, UVs, BaseColors, Tangents, true);
        if (TerrainMaterial) MeshComponent->SetMaterial(0, TerrainMaterial);
    }

    bIsGenerated = true;
}
