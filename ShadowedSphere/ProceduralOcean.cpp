// Implementation of AProceduralOcean: builds the water grid, drives the material's Time
// parameter, keeps the mesh centred on the player, evaluates Gerstner wave displacement,
// and enables the underwater post-process when the camera drops below the surface.

#include "ProceduralOcean.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

AProceduralOcean::AProceduralOcean()
{
    PrimaryActorTick.bCanEverTick = true;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    OceanMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("OceanMesh"));
    OceanMesh->SetupAttachment(RootComponent);
    OceanMesh->SetCastShadow(false);
    OceanMesh->bUseAsyncCooking = true;

    OceanMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    OceanMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    OceanMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    UnderwaterPostProcess = CreateDefaultSubobject<UPostProcessComponent>(TEXT("UnderwaterPostProcess"));
    UnderwaterPostProcess->SetupAttachment(RootComponent);
    UnderwaterPostProcess->bEnabled = false;
    UnderwaterPostProcess->bUnbound = true;
}

void AProceduralOcean::BeginPlay()
{
    Super::BeginPlay();

    GenerateOceanMesh();
    SetupUnderwaterPostProcess();

    UE_LOG(LogTemp, Log, TEXT("ProceduralOcean initialized - Size: %.0f, Resolution: %d, SeaLevel: %.1f"),
        OceanSize, MeshResolution, SeaLevel);
}

void AProceduralOcean::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    GameTime += DeltaTime;

    if (OceanMaterialDynamic)
    {
        OceanMaterialDynamic->SetScalarParameterValue(TEXT("Time"), GameTime);
    }

    if (bFollowPlayer)
    {
        UpdateOceanPosition();
    }

    if (bEnableUnderwaterEffects)
    {
        UpdateUnderwaterEffects();
    }
}

void AProceduralOcean::GenerateOceanMesh()
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;
    TArray<FProcMeshTangent> Tangents;

    int32 NumVerts = MeshResolution * MeshResolution;
    Vertices.Reserve(NumVerts);
    Normals.Reserve(NumVerts);
    UVs.Reserve(NumVerts);
    Colors.Reserve(NumVerts);
    Tangents.Reserve(NumVerts);

    float HalfSize = OceanSize * 0.5f;
    float Step = OceanSize / (float)(MeshResolution - 1);

    for (int32 Y = 0; Y < MeshResolution; Y++)
    {
        for (int32 X = 0; X < MeshResolution; X++)
        {
            float LocalX = -HalfSize + X * Step;
            float LocalY = -HalfSize + Y * Step;

            Vertices.Add(FVector(LocalX, LocalY, SeaLevel));
            Normals.Add(FVector::UpVector);

            float U = (float)X / (float)(MeshResolution - 1);
            float V = (float)Y / (float)(MeshResolution - 1);
            UVs.Add(FVector2D(U * OceanSize / 1000.0f, V * OceanSize / 1000.0f));

            Colors.Add(FColor::White);
            Tangents.Add(FProcMeshTangent(FVector(1, 0, 0), false));
        }
    }

    Triangles.Reserve((MeshResolution - 1) * (MeshResolution - 1) * 6);

    for (int32 Y = 0; Y < MeshResolution - 1; Y++)
    {
        for (int32 X = 0; X < MeshResolution - 1; X++)
        {
            int32 BL = Y * MeshResolution + X;
            int32 BR = BL + 1;
            int32 TL = (Y + 1) * MeshResolution + X;
            int32 TR = TL + 1;

            Triangles.Add(BL);
            Triangles.Add(TL);
            Triangles.Add(BR);

            Triangles.Add(BR);
            Triangles.Add(TL);
            Triangles.Add(TR);
        }
    }

    OceanMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);

    if (OceanMaterial)
    {
        OceanMaterialDynamic = UMaterialInstanceDynamic::Create(OceanMaterial, this);
        OceanMesh->SetMaterial(0, OceanMaterialDynamic);
        UE_LOG(LogTemp, Log, TEXT("ProceduralOcean: Material applied"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ProceduralOcean: No OceanMaterial assigned! Create a Single Layer Water material."));
    }
}

void AProceduralOcean::RegenerateMesh()
{
    OceanMesh->ClearAllMeshSections();
    GenerateOceanMesh();
}

void AProceduralOcean::UpdateOceanPosition()
{
    FVector PlayerPos = GetPlayerLocation();

    float SnappedX = FMath::FloorToFloat(PlayerPos.X / FollowGridSnap) * FollowGridSnap;
    float SnappedY = FMath::FloorToFloat(PlayerPos.Y / FollowGridSnap) * FollowGridSnap;

    FVector NewPos(SnappedX, SnappedY, 0.0f);

    if (!NewPos.Equals(GetActorLocation(), 1.0f))
    {
        SetActorLocation(NewPos);
    }
}

void AProceduralOcean::SetupUnderwaterPostProcess()
{
    if (!UnderwaterPostProcess)
    {
        return;
    }

    FPostProcessSettings& PP = UnderwaterPostProcess->Settings;

    PP.bOverride_ColorGamma = true;
    PP.ColorGamma = FVector4(0.9f, 0.95f, 1.0f, 1.0f);

    PP.bOverride_ColorSaturation = true;
    PP.ColorSaturation = FVector4(0.8f, 0.8f, 0.8f, 1.0f);

    PP.bOverride_SceneColorTint = true;
    PP.SceneColorTint = FLinearColor(0.7f, 0.85f, 1.0f, 1.0f);

    PP.bOverride_VignetteIntensity = true;
    PP.VignetteIntensity = 0.5f;

    PP.bOverride_BloomIntensity = true;
    PP.BloomIntensity = 1.5f;
}

void AProceduralOcean::UpdateUnderwaterEffects()
{
    FVector PlayerLoc = GetPlayerLocation();
    bool bUnderwater = IsPositionUnderwater(PlayerLoc);

    if (UnderwaterPostProcess->bEnabled != bUnderwater)
    {
        UnderwaterPostProcess->bEnabled = bUnderwater;
    }
}

FVector AProceduralOcean::CalculateGerstnerWave(float X, float Y, float Time,
    float Amplitude, float Wavelength, float Speed, FVector2D Direction, float Steepness) const
{
    if (Wavelength <= 0.0f || Amplitude <= 0.0f)
    {
        return FVector::ZeroVector;
    }
    Direction.Normalize();

    float K = 2.0f * PI / Wavelength;
    float W = Speed * K;
    float Q = Steepness / (K * Amplitude);

    float Phase = K * (Direction.X * X + Direction.Y * Y) - W * Time;

    float CosPhase = FMath::Cos(Phase);
    float SinPhase = FMath::Sin(Phase);

    FVector Displacement;
    Displacement.X = Q * Amplitude * Direction.X * CosPhase;
    Displacement.Y = Q * Amplitude * Direction.Y * CosPhase;
    Displacement.Z = Amplitude * SinPhase;

    return Displacement;
}

float AProceduralOcean::GetWaveHeightAt(float WorldX, float WorldY) const
{
    FVector Displacement = GetWaveDisplacementAt(WorldX, WorldY);
    return SeaLevel + Displacement.Z;
}

FVector AProceduralOcean::GetWaveDisplacementAt(float WorldX, float WorldY) const
{
    if (!bEnableGerstnerWaves)
    {
        return FVector(0, 0, SeaLevel);
    }

    FVector TotalDisplacement = FVector::ZeroVector;

    TotalDisplacement += CalculateGerstnerWave(WorldX, WorldY, GameTime,
        Wave1Amplitude, Wave1Wavelength, Wave1Speed, Wave1Direction, WaveSteepness);

    TotalDisplacement += CalculateGerstnerWave(WorldX, WorldY, GameTime,
        Wave2Amplitude, Wave2Wavelength, Wave2Speed, Wave2Direction, WaveSteepness);

    TotalDisplacement += CalculateGerstnerWave(WorldX, WorldY, GameTime,
        Wave3Amplitude, Wave3Wavelength, Wave3Speed, Wave3Direction, WaveSteepness);

    return TotalDisplacement;
}

bool AProceduralOcean::IsPositionUnderwater(FVector WorldPosition) const
{
    float WaterHeight = GetWaveHeightAt(WorldPosition.X, WorldPosition.Y);
    return WorldPosition.Z < WaterHeight;
}

FVector AProceduralOcean::GetPlayerLocation() const
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC && PC->GetPawn())
    {
        return PC->GetPawn()->GetActorLocation();
    }
    return FVector::ZeroVector;
}
