// Buoyancy for physics-simulated ships. Samples a Gerstner wave surface at each pontoon,
// applies an upward force scaled to the ship's own mass so tuning is mass-independent, and
// adds water drag plus submersion-blended damping.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ShipBuoyancyComponent.generated.h"

USTRUCT(BlueprintType)
struct FShipPontoon
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pontoon")
    FVector RelativeLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pontoon", meta = (ClampMin = "0.1"))
    float ForceMultiplier = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pontoon|Debug")
    float SubmersionDepth = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pontoon|Debug")
    bool bIsSubmerged = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pontoon|Debug")
    FVector WorldLocation = FVector::ZeroVector;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pontoon|Debug")
    float WaterHeight = 0.0f;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SHADOWEDSPHERE_API UShipBuoyancyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UShipBuoyancyComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    // Drag your ocean actor here (BP_Fantasy_Ocean etc). Uses its Z as sea level.
    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Buoyancy|Ocean")
    AActor* OceanActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Ocean")
    float FallbackSeaLevel = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    bool bEnableWaves = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    float Wave1Amplitude = 40.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    float Wave1Wavelength = 3000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    float Wave1Speed = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    FVector2D Wave1Direction = FVector2D(1.0f, 0.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    float Wave2Amplitude = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    float Wave2Wavelength = 1500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    float Wave2Speed = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    FVector2D Wave2Direction = FVector2D(0.7f, 0.7f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    float Wave3Amplitude = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    float Wave3Wavelength = 800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    float Wave3Speed = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Waves")
    FVector2D Wave3Direction = FVector2D(-0.5f, 0.8f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Pontoons")
    TArray<FShipPontoon> Pontoons;

    // Buoyancy strength as a multiplier of gravity.
    // 1.0 = when fully submerged, exactly counteracts gravity (neutral buoyancy)
    // 1.5 = 50% stronger than gravity (ship floats up)
    // 2.0 = double gravity (very buoyant, pops up fast)
    // Works automatically regardless of ship mass.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Forces", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float BuoyancyForce = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Forces", meta = (ClampMin = "0.5", ClampMax = "3.0"))
    float SubmersionForceRamp = 1.2f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Forces", meta = (ClampMin = "10"))
    float MaxSubmersionDepth = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Damping", meta = (ClampMin = "0"))
    float WaterLinearDamping = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Damping", meta = (ClampMin = "0"))
    float WaterAngularDamping = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Damping", meta = (ClampMin = "0"))
    float AirLinearDamping = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Damping", meta = (ClampMin = "0"))
    float AirAngularDamping = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Drag", meta = (ClampMin = "0"))
    float LateralDragCoefficient = 200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Drag", meta = (ClampMin = "0"))
    float ForwardDragCoefficient = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Drag", meta = (ClampMin = "0"))
    float VerticalDragCoefficient = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Buoyancy|Debug")
    bool bDrawDebug = true;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buoyancy|State")
    float SubmersionPercent = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Buoyancy|State")
    bool bIsInWater = false;

    UFUNCTION(BlueprintCallable, Category = "Buoyancy")
    float GetSeaLevel() const;

    UFUNCTION(BlueprintCallable, Category = "Buoyancy")
    float GetWaterHeightAt(FVector WorldPos) const;

    UFUNCTION(BlueprintCallable, Category = "Buoyancy")
    bool IsUnderwater(FVector WorldPos) const;

protected:
    UPROPERTY()
    UPrimitiveComponent* PhysicsBody;

    float GameTime = 0.0f;

    void SetupDefaultPontoons();
    void ApplyBuoyancyForces(float DeltaTime);
    void ApplyWaterDrag(float DeltaTime);
    void UpdateDamping();
    void DrawDebugInfo();

    float CalcGerstnerWaveHeight(float X, float Y, float Time,
        float Amplitude, float Wavelength, float Speed, FVector2D Direction) const;
};
