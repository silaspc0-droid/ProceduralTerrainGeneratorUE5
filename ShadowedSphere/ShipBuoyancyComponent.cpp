// Implementation of UShipBuoyancyComponent: evaluates water height per pontoon, applies
// depth-ramped buoyancy and directional drag to the owner's physics body, blends damping
// between air and water, and draws the debug water grid and pontoon state.

#include "ShipBuoyancyComponent.h"
#include "DrawDebugHelpers.h"

UShipBuoyancyComponent::UShipBuoyancyComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UShipBuoyancyComponent::BeginPlay()
{
    Super::BeginPlay();

    AActor* Owner = GetOwner();
    if (Owner)
    {
        PhysicsBody = Cast<UPrimitiveComponent>(Owner->GetRootComponent());

        if (!PhysicsBody)
        {
            UE_LOG(LogTemp, Error, TEXT("ShipBuoyancy on %s: Root component is NOT a PrimitiveComponent!"), *Owner->GetName());
        }
        else if (!PhysicsBody->IsSimulatingPhysics())
        {
            UE_LOG(LogTemp, Error, TEXT("ShipBuoyancy on %s: Root component is NOT simulating physics!"), *Owner->GetName());
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("ShipBuoyancy on %s: Physics body found. Mass=%.1f kg"),
                *Owner->GetName(), PhysicsBody->GetMass());
        }
    }

    if (Pontoons.Num() == 0)
    {
        SetupDefaultPontoons();
    }

    UE_LOG(LogTemp, Warning, TEXT("ShipBuoyancy: %d pontoons, OceanActor=%s, SeaLevel=%.1f, BuoyancyForce=%.0f"),
        Pontoons.Num(),
        OceanActor ? *OceanActor->GetName() : TEXT("NONE (using fallback)"),
        GetSeaLevel(),
        BuoyancyForce);
}

void UShipBuoyancyComponent::SetupDefaultPontoons()
{
    FShipPontoon Bow;
    Bow.RelativeLocation = FVector(300.0f, 0.0f, -50.0f);
    Bow.ForceMultiplier = 1.0f;

    FShipPontoon Stern;
    Stern.RelativeLocation = FVector(-300.0f, 0.0f, -50.0f);
    Stern.ForceMultiplier = 1.0f;

    FShipPontoon Port;
    Port.RelativeLocation = FVector(0.0f, -150.0f, -50.0f);
    Port.ForceMultiplier = 0.8f;

    FShipPontoon Starboard;
    Starboard.RelativeLocation = FVector(0.0f, 150.0f, -50.0f);
    Starboard.ForceMultiplier = 0.8f;

    FShipPontoon Center;
    Center.RelativeLocation = FVector(0.0f, 0.0f, -80.0f);
    Center.ForceMultiplier = 1.2f;

    Pontoons.Add(Bow);
    Pontoons.Add(Stern);
    Pontoons.Add(Port);
    Pontoons.Add(Starboard);
    Pontoons.Add(Center);
}

float UShipBuoyancyComponent::GetSeaLevel() const
{
    if (OceanActor)
    {
        return OceanActor->GetActorLocation().Z;
    }
    return FallbackSeaLevel;
}

float UShipBuoyancyComponent::CalcGerstnerWaveHeight(float X, float Y, float Time,
    float Amplitude, float Wavelength, float Speed, FVector2D Direction) const
{
    if (Wavelength <= 0.0f || Amplitude <= 0.0f)
        return 0.0f;

    Direction.Normalize();
    float K = 2.0f * PI / Wavelength;
    float W = Speed * K;
    float Phase = K * (Direction.X * X + Direction.Y * Y) - W * Time;
    return Amplitude * FMath::Sin(Phase);
}

float UShipBuoyancyComponent::GetWaterHeightAt(FVector WorldPos) const
{
    float BaseHeight = GetSeaLevel();

    if (!bEnableWaves)
        return BaseHeight;

    float X = WorldPos.X;
    float Y = WorldPos.Y;

    float WaveHeight = 0.0f;
    WaveHeight += CalcGerstnerWaveHeight(X, Y, GameTime, Wave1Amplitude, Wave1Wavelength, Wave1Speed, Wave1Direction);
    WaveHeight += CalcGerstnerWaveHeight(X, Y, GameTime, Wave2Amplitude, Wave2Wavelength, Wave2Speed, Wave2Direction);
    WaveHeight += CalcGerstnerWaveHeight(X, Y, GameTime, Wave3Amplitude, Wave3Wavelength, Wave3Speed, Wave3Direction);

    return BaseHeight + WaveHeight;
}

bool UShipBuoyancyComponent::IsUnderwater(FVector WorldPos) const
{
    return WorldPos.Z < GetWaterHeightAt(WorldPos);
}

void UShipBuoyancyComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!PhysicsBody || !PhysicsBody->IsSimulatingPhysics())
        return;

    GameTime += DeltaTime;

    ApplyBuoyancyForces(DeltaTime);
    ApplyWaterDrag(DeltaTime);
    UpdateDamping();

    if (bDrawDebug)
    {
        DrawDebugInfo();
    }
}

void UShipBuoyancyComponent::ApplyBuoyancyForces(float DeltaTime)
{
    AActor* Owner = GetOwner();
    if (!Owner || !PhysicsBody) return;

    FTransform ActorTransform = Owner->GetActorTransform();
    int32 SubmergedCount = 0;

    float Mass = PhysicsBody->GetMass();
    float GravityZ = FMath::Abs(GetWorld()->GetGravityZ());
    float WeightForce = Mass * GravityZ;

    for (FShipPontoon& Pontoon : Pontoons)
    {
        Pontoon.WorldLocation = ActorTransform.TransformPosition(Pontoon.RelativeLocation);
        Pontoon.WaterHeight = GetWaterHeightAt(Pontoon.WorldLocation);
        Pontoon.SubmersionDepth = Pontoon.WaterHeight - Pontoon.WorldLocation.Z;
        Pontoon.bIsSubmerged = Pontoon.SubmersionDepth > 0.0f;

        if (Pontoon.bIsSubmerged)
        {
            SubmergedCount++;

            float ClampedDepth = FMath::Min(Pontoon.SubmersionDepth, MaxSubmersionDepth);
            float NormDepth = ClampedDepth / MaxSubmersionDepth;
            float ForceFactor = FMath::Pow(NormDepth, 1.0f / SubmersionForceRamp);

            float PerPontoonWeight = WeightForce / FMath::Max(Pontoons.Num(), 1);
            float UpForce = PerPontoonWeight * BuoyancyForce * ForceFactor * Pontoon.ForceMultiplier;

            PhysicsBody->AddForceAtLocation(FVector(0.0f, 0.0f, UpForce), Pontoon.WorldLocation);
        }
    }

    SubmersionPercent = (Pontoons.Num() > 0) ? (float)SubmergedCount / (float)Pontoons.Num() : 0.0f;
    bIsInWater = SubmergedCount > 0;
}

void UShipBuoyancyComponent::ApplyWaterDrag(float DeltaTime)
{
    if (!bIsInWater || !PhysicsBody) return;

    FVector WorldVelocity = PhysicsBody->GetPhysicsLinearVelocity();
    FTransform Transform = GetOwner()->GetActorTransform();
    FVector LocalVelocity = Transform.InverseTransformVector(WorldVelocity);

    FVector DragForceLocal;
    DragForceLocal.X = -LocalVelocity.X * ForwardDragCoefficient;
    DragForceLocal.Y = -LocalVelocity.Y * LateralDragCoefficient;
    DragForceLocal.Z = -LocalVelocity.Z * VerticalDragCoefficient;
    DragForceLocal *= SubmersionPercent;

    FVector DragForceWorld = Transform.TransformVector(DragForceLocal);
    PhysicsBody->AddForce(DragForceWorld);
}

void UShipBuoyancyComponent::UpdateDamping()
{
    if (!PhysicsBody) return;

    float LinearDamp = FMath::Lerp(AirLinearDamping, WaterLinearDamping, SubmersionPercent);
    float AngularDamp = FMath::Lerp(AirAngularDamping, WaterAngularDamping, SubmersionPercent);

    PhysicsBody->SetLinearDamping(LinearDamp);
    PhysicsBody->SetAngularDamping(AngularDamp);
}

void UShipBuoyancyComponent::DrawDebugInfo()
{
    UWorld* World = GetWorld();
    if (!World) return;

    AActor* Owner = GetOwner();
    if (Owner)
    {
        FVector ShipPos = Owner->GetActorLocation();
        float WH = GetWaterHeightAt(ShipPos);

        float GridSize = 500.0f;
        for (float DX = -GridSize; DX <= GridSize; DX += GridSize)
        {
            FVector A(ShipPos.X + DX, ShipPos.Y - GridSize, WH);
            FVector B(ShipPos.X + DX, ShipPos.Y + GridSize, WH);
            DrawDebugLine(World, A, B, FColor::Blue, false, -1.0f, 0, 1.0f);
        }
        for (float DY = -GridSize; DY <= GridSize; DY += GridSize)
        {
            FVector A(ShipPos.X - GridSize, ShipPos.Y + DY, WH);
            FVector B(ShipPos.X + GridSize, ShipPos.Y + DY, WH);
            DrawDebugLine(World, A, B, FColor::Blue, false, -1.0f, 0, 1.0f);
        }
    }

    for (const FShipPontoon& Pontoon : Pontoons)
    {
        FColor Color = Pontoon.bIsSubmerged ? FColor::Cyan : FColor::Red;
        DrawDebugSphere(World, Pontoon.WorldLocation, 20.0f, 8, Color, false, -1.0f, 0, 2.0f);

        FVector WaterPos(Pontoon.WorldLocation.X, Pontoon.WorldLocation.Y, Pontoon.WaterHeight);
        DrawDebugSphere(World, WaterPos, 10.0f, 6, FColor::Blue, false, -1.0f, 0, 1.0f);
        DrawDebugLine(World, Pontoon.WorldLocation, WaterPos, FColor::White, false, -1.0f, 0, 1.0f);

        FString DepthText = FString::Printf(TEXT("%.0f"), Pontoon.SubmersionDepth);
        FColor TextColor = Pontoon.bIsSubmerged ? FColor::Cyan : FColor::Red;
        DrawDebugString(World, Pontoon.WorldLocation + FVector(0, 0, 40), DepthText, nullptr, TextColor, 0.0f);
    }

    if (Owner)
    {
        FVector ActorLoc = Owner->GetActorLocation();
        FString StateText = FString::Printf(TEXT("%.0f%% submerged | Sea=%.0f | ShipZ=%.0f | Mass=%.0f"),
            SubmersionPercent * 100.0f,
            GetSeaLevel(),
            ActorLoc.Z,
            PhysicsBody ? PhysicsBody->GetMass() : 0.0f);
        DrawDebugString(World, ActorLoc + FVector(0, 0, 250), StateText, nullptr, FColor::Yellow, 0.0f);
    }
}
