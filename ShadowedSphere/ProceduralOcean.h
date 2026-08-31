// Standalone ocean actor for procedural worlds. Builds a flat procedural mesh for a Single
// Layer Water material, optionally follows the player on a snapped grid, sums three Gerstner
// waves for gameplay height queries, and toggles an underwater post-process volume.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralMeshComponent.h"
#include "ProceduralOcean.generated.h"

class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPostProcessComponent;
class UBoxComponent;

UCLASS(Blueprintable)
class SHADOWEDSPHERE_API AProceduralOcean : public AActor
{
    GENERATED_BODY()

public:
    AProceduralOcean();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ocean")
    UProceduralMeshComponent* OceanMesh;

    // Assign a Single Layer Water material here
    // Create one in editor: Material > Shading Model = Single Layer Water
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Material")
    UMaterialInterface* OceanMaterial;

    UPROPERTY(BlueprintReadOnly, Category = "Ocean|Material")
    UMaterialInstanceDynamic* OceanMaterialDynamic;

    // Sea level (Z height of water surface)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Settings")
    float SeaLevel = 0.0f;

    // Total size of ocean mesh
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Settings")
    float OceanSize = 500000.0f;

    // Resolution of ocean mesh (vertices per side)
    // Higher = better waves but more expensive
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Settings", meta = (ClampMin = "16", ClampMax = "512"))
    int32 MeshResolution = 128;

    // Follow the player for infinite ocean
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Settings")
    bool bFollowPlayer = true;

    // Grid snap size when following (prevents swimming artifacts)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Settings")
    float FollowGridSnap = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    bool bEnableGerstnerWaves = true;

    // Wave 1
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    float Wave1Amplitude = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    float Wave1Wavelength = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    float Wave1Speed = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    FVector2D Wave1Direction = FVector2D(1.0f, 0.0f);

    // Wave 2
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    float Wave2Amplitude = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    float Wave2Wavelength = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    float Wave2Speed = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    FVector2D Wave2Direction = FVector2D(0.7f, 0.7f);

    // Wave 3
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    float Wave3Amplitude = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    float Wave3Wavelength = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    float Wave3Speed = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves")
    FVector2D Wave3Direction = FVector2D(-0.5f, 0.8f);

    // Steepness for Gerstner waves (0-1, higher = sharper peaks)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Waves", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WaveSteepness = 0.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Ocean|Underwater")
    UPostProcessComponent* UnderwaterPostProcess;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Underwater")
    bool bEnableUnderwaterEffects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Underwater")
    FLinearColor UnderwaterColor = FLinearColor(0.01f, 0.05f, 0.1f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ocean|Underwater")
    float UnderwaterFogDensity = 0.02f;

    // Get wave height at world position (includes Gerstner displacement)
    UFUNCTION(BlueprintCallable, Category = "Ocean")
    float GetWaveHeightAt(float WorldX, float WorldY) const;

    // Get full wave displacement (X, Y, Z) at world position
    UFUNCTION(BlueprintCallable, Category = "Ocean")
    FVector GetWaveDisplacementAt(float WorldX, float WorldY) const;

    // Check if a position is underwater
    UFUNCTION(BlueprintCallable, Category = "Ocean")
    bool IsPositionUnderwater(FVector WorldPosition) const;

    // Regenerate the ocean mesh (call if you change resolution)
    UFUNCTION(BlueprintCallable, Category = "Ocean")
    void RegenerateMesh();

protected:
    float GameTime = 0.0f;
    FVector LastPlayerPosition = FVector::ZeroVector;

    void GenerateOceanMesh();
    void UpdateOceanPosition();
    void UpdateUnderwaterEffects();
    void SetupUnderwaterPostProcess();
    FVector GetPlayerLocation() const;

    FVector CalculateGerstnerWave(float X, float Y, float Time,
        float Amplitude, float Wavelength, float Speed, FVector2D Direction, float Steepness) const;
};
