#include "ZMArenaBuilder.h"
#include "ZMTypes.h"
#include "ZMSpawnPoint.h"
#include "ZMBuyables.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PostProcessComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FLinearColor FloorColor(0.075f, 0.07f, 0.065f);
	const FLinearColor WallColor(0.17f, 0.15f, 0.13f);
	const FLinearColor TrimColor(0.1f, 0.09f, 0.08f);
	const FLinearColor CrateColor(0.16f, 0.1f, 0.05f);
	const FLinearColor MetalColor(0.08f, 0.09f, 0.1f);
	const FLinearColor DirtColor(0.06f, 0.04f, 0.025f);
	const FLinearColor LampColor(1.f, 0.62f, 0.32f);

	constexpr float WallHeight = 420.f;
	constexpr float WallThick = 40.f;
	constexpr float DoorWidth = 300.f;
	constexpr float DoorHeight = 300.f;
}

AZMArenaBuilder::AZMArenaBuilder()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);
}

template <typename T>
T* AZMArenaBuilder::Place(const FVector& Location, const FRotator& Rotation, TFunctionRef<void(T*)> Setup)
{
	const FTransform Xf(Rotation, Location);
	T* Actor = GetWorld()->SpawnActorDeferred<T>(T::StaticClass(), Xf, this, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (!Actor) return nullptr;
	Setup(Actor);
	UGameplayStatics::FinishSpawningActor(Actor, Xf);
	return Actor;
}

UStaticMeshComponent* AZMArenaBuilder::Part(UStaticMesh* Mesh, const FVector& Center, const FVector& Size, const FRotator& Rotation, const FLinearColor& Color, bool bCollide)
{
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Static);
	C->SetStaticMesh(Mesh);
	C->SetupAttachment(RootComponent);
	C->SetRelativeLocationAndRotation(Center, Rotation);
	C->SetRelativeScale3D(Size / 100.f);
	C->SetCollisionProfileName(bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	C->SetCanEverAffectNavigation(bCollide);
	C->SetGenerateOverlapEvents(false);
	C->SetMaterial(0, ZM::MakeTintedMaterial(this, Color));
	C->RegisterComponent();
	AddInstanceComponent(C);
	Parts.Add(C);
	return C;
}

void AZMArenaBuilder::Box(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollide)
{
	Part(ZM::CubeMesh(), Center, Size, FRotator::ZeroRotator, Color, bCollide);
}

void AZMArenaBuilder::Wall(const FVector& From, const FVector& To, float Height, float Thickness, const FLinearColor& Color)
{
	const FVector Delta = (To - From) * FVector(1.f, 1.f, 0.f);
	const float Len = Delta.Size() + Thickness; // overlap corners
	const FVector Center = (From + To) * 0.5f + FVector(0.f, 0.f, Height * 0.5f);
	const FRotator Rot(0.f, Delta.Rotation().Yaw, 0.f);
	Part(ZM::CubeMesh(), Center, FVector(Len, Thickness, Height), Rot, Color, true);
	// Dark baseboard trim so walls read against the floor.
	Part(ZM::CubeMesh(), (From + To) * 0.5f + FVector(0.f, 0.f, 15.f), FVector(Len, Thickness + 8.f, 30.f), Rot, TrimColor, false);
}

void AZMArenaBuilder::Light(const FVector& Location, const FLinearColor& Color, float Intensity, float Radius, bool bFlicker)
{
	UPointLightComponent* L = NewObject<UPointLightComponent>(this);
	L->SetupAttachment(RootComponent);
	L->SetRelativeLocation(Location);
	L->SetIntensityUnits(ELightUnits::Candelas);
	L->SetIntensity(Intensity);
	L->SetLightColor(Color);
	L->SetAttenuationRadius(Radius);
	L->SetSourceRadius(6.f);
	L->SetCastShadows(true);
	L->RegisterComponent();
	AddInstanceComponent(L);

	// Bulb + shade so the light has a visible source.
	Part(ZM::SphereMesh(), Location, FVector(14.f), FRotator::ZeroRotator, Color * 4.f, false);
	Part(ZM::CubeMesh(), Location + FVector(0.f, 0.f, 12.f), FVector(34.f, 34.f, 6.f), FRotator::ZeroRotator, MetalColor, false);

	if (bFlicker)
	{
		FlickerLights.Add(L);
		FlickerBase.Add(Intensity);
		FlickerTimers.Add(FMath::FRandRange(0.f, 1.f));
	}
}

void AZMArenaBuilder::Spawner(const FVector& Location, int32 Zone)
{
	// Disturbed earth where zombies claw their way up.
	Box(FVector(Location.X, Location.Y, 1.f), FVector(150.f, 130.f, 4.f), DirtColor, false);
	Box(FVector(Location.X - 40.f, Location.Y + 30.f, 6.f), FVector(50.f, 40.f, 10.f), DirtColor * 0.8f, false);
	Box(FVector(Location.X + 45.f, Location.Y - 35.f, 5.f), FVector(40.f, 50.f, 8.f), DirtColor * 1.2f, false);

	Place<AZMSpawnPoint>(FVector(Location.X, Location.Y, 100.f), FRotator::ZeroRotator, [Zone](AZMSpawnPoint* P) { P->Zone = Zone; });
}

void AZMArenaBuilder::Atmosphere()
{
	// Cold moonlight.
	UDirectionalLightComponent* Moon = NewObject<UDirectionalLightComponent>(this);
	Moon->SetupAttachment(RootComponent);
	Moon->SetRelativeRotation(FRotator(-38.f, 35.f, 0.f));
	Moon->SetIntensity(0.6f);
	Moon->SetLightColor(FLinearColor(0.55f, 0.66f, 1.f));
	Moon->SetAtmosphereSunLight(true);
	Moon->SetCastShadows(true);
	Moon->RegisterComponent();
	AddInstanceComponent(Moon);

	USkyAtmosphereComponent* Sky = NewObject<USkyAtmosphereComponent>(this);
	Sky->SetupAttachment(RootComponent);
	Sky->RegisterComponent();
	AddInstanceComponent(Sky);

	USkyLightComponent* SkyLight = NewObject<USkyLightComponent>(this);
	SkyLight->SetupAttachment(RootComponent);
	SkyLight->bRealTimeCapture = true;
	SkyLight->SetIntensity(1.5f);
	SkyLight->SetLowerHemisphereColor(FLinearColor(0.01f, 0.012f, 0.015f));
	SkyLight->RegisterComponent();
	AddInstanceComponent(SkyLight);

	UExponentialHeightFogComponent* Fog = NewObject<UExponentialHeightFogComponent>(this);
	Fog->SetupAttachment(RootComponent);
	Fog->SetFogDensity(0.035f);
	Fog->SetFogHeightFalloff(0.35f);
	Fog->SetFogInscatteringColor(FLinearColor(0.025f, 0.035f, 0.045f));
	Fog->SetVolumetricFog(true);
	Fog->SetVolumetricFogScatteringDistribution(0.6f);
	Fog->SetVolumetricFogExtinctionScale(1.5f);
	Fog->RegisterComponent();
	AddInstanceComponent(Fog);

	UPostProcessComponent* PP = NewObject<UPostProcessComponent>(this);
	PP->SetupAttachment(RootComponent);
	PP->bUnbound = true;
	FPostProcessSettings& S = PP->Settings;
	S.bOverride_AutoExposureMethod = true;
	S.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	S.bOverride_AutoExposureBias = true;
	S.AutoExposureBias = 0.f;
	S.bOverride_VignetteIntensity = true;
	S.VignetteIntensity = 0.6f;
	S.bOverride_FilmGrainIntensity = true;
	S.FilmGrainIntensity = 0.12f;
	S.bOverride_BloomIntensity = true;
	S.BloomIntensity = 0.8f;
	S.bOverride_ColorSaturation = true;
	S.ColorSaturation = FVector4(0.85f, 0.85f, 0.85f, 1.f);
	S.bOverride_ColorContrast = true;
	S.ColorContrast = FVector4(1.1f, 1.1f, 1.1f, 1.f);
	S.bOverride_ColorGain = true;
	S.ColorGain = FVector4(0.95f, 1.f, 1.03f, 1.f);
	S.bOverride_SceneFringeIntensity = true;
	S.SceneFringeIntensity = 0.4f;
	PP->RegisterComponent();
	AddInstanceComponent(PP);
}

void AZMArenaBuilder::BeginPlay()
{
	Super::BeginPlay();

	// --- Start room (zone 0): courtyard X[-1000,1000] Y[-1000,1000] ---
	Box(FVector(0.f, 0.f, -10.f), FVector(2000.f, 2000.f, 20.f), FloorColor);
	Wall(FVector(-1000.f, -1000.f, 0.f), FVector(-1000.f, 1000.f, 0.f), WallHeight, WallThick, WallColor);
	Wall(FVector(-1000.f, -1000.f, 0.f), FVector(1000.f, -1000.f, 0.f), WallHeight, WallThick, WallColor);
	// North wall with doorway to zone 1.
	Wall(FVector(1000.f, -1000.f, 0.f), FVector(1000.f, -DoorWidth * 0.5f, 0.f), WallHeight, WallThick, WallColor);
	Wall(FVector(1000.f, DoorWidth * 0.5f, 0.f), FVector(1000.f, 1000.f, 0.f), WallHeight, WallThick, WallColor);
	Box(FVector(1000.f, 0.f, DoorHeight + (WallHeight - DoorHeight) * 0.5f), FVector(WallThick, DoorWidth + 20.f, WallHeight - DoorHeight), WallColor);
	// East wall with doorway to zone 2.
	Wall(FVector(-1000.f, 1000.f, 0.f), FVector(-DoorWidth * 0.5f, 1000.f, 0.f), WallHeight, WallThick, WallColor);
	Wall(FVector(DoorWidth * 0.5f, 1000.f, 0.f), FVector(1000.f, 1000.f, 0.f), WallHeight, WallThick, WallColor);
	Box(FVector(0.f, 1000.f, DoorHeight + (WallHeight - DoorHeight) * 0.5f), FVector(DoorWidth + 20.f, WallThick, WallHeight - DoorHeight), WallColor);

	// Cover and clutter.
	Box(FVector(-320.f, 620.f, 45.f), FVector(90.f, 90.f, 90.f), CrateColor);
	Box(FVector(-410.f, 660.f, 45.f), FVector(80.f, 80.f, 90.f), CrateColor * 0.8f);
	Box(FVector(-360.f, 640.f, 125.f), FVector(70.f, 70.f, 70.f), CrateColor * 1.2f);
	Box(FVector(480.f, -260.f, 50.f), FVector(60.f, 320.f, 100.f), FLinearColor(0.12f, 0.11f, 0.08f)); // sandbags
	Box(FVector(-560.f, -420.f, 40.f), FVector(220.f, 110.f, 80.f), MetalColor);                     // dumpster
	Box(FVector(-560.f, -420.f, 84.f), FVector(230.f, 118.f, 8.f), MetalColor * 1.4f);
	for (const FVector2D P : { FVector2D(-500.f, 500.f), FVector2D(500.f, 500.f), FVector2D(-500.f, -500.f), FVector2D(500.f, -500.f) })
	{
		// Lamp posts.
		Box(FVector(P.X, P.Y, 170.f), FVector(16.f, 16.f, 340.f), MetalColor);
	}
	Light(FVector(-500.f, 500.f, 352.f), LampColor, 40.f, 1500.f);
	Light(FVector(500.f, 500.f, 352.f), LampColor, 40.f, 1500.f, true);
	Light(FVector(-500.f, -500.f, 352.f), LampColor, 40.f, 1500.f);
	Light(FVector(500.f, -500.f, 352.f), LampColor, 40.f, 1500.f);

	// --- Zone 1 (north): X[1000,2800] Y[-900,900] ---
	Box(FVector(1900.f, 0.f, -10.f), FVector(1800.f, 1800.f, 20.f), FloorColor * 1.1f);
	Wall(FVector(2800.f, -900.f, 0.f), FVector(2800.f, 900.f, 0.f), WallHeight, WallThick, WallColor * 0.9f);
	Wall(FVector(1000.f, -900.f, 0.f), FVector(2800.f, -900.f, 0.f), WallHeight, WallThick, WallColor * 0.9f);
	Wall(FVector(1000.f, 900.f, 0.f), FVector(2800.f, 900.f, 0.f), WallHeight, WallThick, WallColor * 0.9f);
	Box(FVector(1700.f, 300.f, 40.f), FVector(200.f, 90.f, 80.f), CrateColor * 0.7f); // workbench
	Box(FVector(2300.f, -350.f, 60.f), FVector(80.f, 260.f, 120.f), MetalColor);       // shelving
	Light(FVector(1500.f, 0.f, 380.f), FLinearColor(0.75f, 0.85f, 1.f), 30.f, 1400.f, true);
	Light(FVector(2400.f, 0.f, 380.f), FLinearColor(0.75f, 0.85f, 1.f), 30.f, 1400.f);

	// --- Zone 2 (east): X[-800,800] Y[1000,2800] ---
	Box(FVector(0.f, 1900.f, -10.f), FVector(1600.f, 1800.f, 20.f), FloorColor * 0.9f);
	Wall(FVector(-800.f, 2800.f, 0.f), FVector(800.f, 2800.f, 0.f), WallHeight, WallThick, WallColor * 1.1f);
	Wall(FVector(-800.f, 1000.f, 0.f), FVector(-800.f, 2800.f, 0.f), WallHeight, WallThick, WallColor * 1.1f);
	Wall(FVector(800.f, 1000.f, 0.f), FVector(800.f, 2800.f, 0.f), WallHeight, WallThick, WallColor * 1.1f);
	Box(FVector(-250.f, 1900.f, 45.f), FVector(90.f, 90.f, 90.f), CrateColor);
	Box(FVector(300.f, 2100.f, 35.f), FVector(160.f, 70.f, 70.f), CrateColor * 0.9f);
	Light(FVector(0.f, 1600.f, 380.f), FLinearColor(1.f, 0.45f, 0.3f), 30.f, 1300.f);
	Light(FVector(0.f, 2400.f, 380.f), FLinearColor(1.f, 0.45f, 0.3f), 30.f, 1300.f, true);

	// --- Doors ---
	const FVector DoorSize(WallThick, DoorWidth, DoorHeight);
	Place<AZMDoor>(FVector(1000.f, 0.f, DoorHeight * 0.5f), FRotator(0.f, 180.f, 0.f), [&](AZMDoor* D) { D->Cost = 750; D->UnlocksZone = 1; D->Size = DoorSize; });
	Place<AZMDoor>(FVector(0.f, 1000.f, DoorHeight * 0.5f), FRotator(0.f, -90.f, 0.f), [&](AZMDoor* D) { D->Cost = 1000; D->UnlocksZone = 2; D->Size = DoorSize; });

	// --- Wall buys (actor X points out of the wall) ---
	Place<AZMWallBuy>(FVector(-978.f, -350.f, 165.f), FRotator(0.f, 0.f, 0.f), [](AZMWallBuy* W) { W->WeaponId = "VANDAL"; });
	Place<AZMWallBuy>(FVector(2778.f, -400.f, 165.f), FRotator(0.f, 180.f, 0.f), [](AZMWallBuy* W) { W->WeaponId = "RANGER"; });
	Place<AZMWallBuy>(FVector(300.f, 2778.f, 165.f), FRotator(0.f, -90.f, 0.f), [](AZMWallBuy* W) { W->WeaponId = "BREACHER"; });

	// --- Perks ---
	Place<AZMPerkMachine>(FVector(350.f, -940.f, 0.f), FRotator(0.f, 90.f, 0.f), [](AZMPerkMachine* P) { P->Perk = EZMPerk::SecondWind; });
	Place<AZMPerkMachine>(FVector(2000.f, -840.f, 0.f), FRotator(0.f, 90.f, 0.f), [](AZMPerkMachine* P) { P->Perk = EZMPerk::IronGut; });
	Place<AZMPerkMachine>(FVector(2000.f, 840.f, 0.f), FRotator(0.f, -90.f, 0.f), [](AZMPerkMachine* P) { P->Perk = EZMPerk::DoubleTap; });
	Place<AZMPerkMachine>(FVector(-740.f, 2200.f, 0.f), FRotator(0.f, 0.f, 0.f), [](AZMPerkMachine* P) { P->Perk = EZMPerk::QuickHands; });

	// --- Mystery box ---
	Place<AZMMysteryBox>(FVector(2745.f, 350.f, 0.f), FRotator(0.f, 180.f, 0.f), [](AZMMysteryBox*) {});

	// --- Zombie spawners ---
	Spawner(FVector(-820.f, -820.f, 0.f), 0);
	Spawner(FVector(-820.f, 820.f, 0.f), 0);
	Spawner(FVector(820.f, -700.f, 0.f), 0);
	Spawner(FVector(820.f, 780.f, 0.f), 0);
	Spawner(FVector(2620.f, -720.f, 0.f), 1);
	Spawner(FVector(2620.f, 720.f, 0.f), 1);
	Spawner(FVector(1600.f, -720.f, 0.f), 1);
	Spawner(FVector(-620.f, 2620.f, 0.f), 2);
	Spawner(FVector(620.f, 2620.f, 0.f), 2);
	Spawner(FVector(600.f, 1500.f, 0.f), 2);

	Atmosphere();
}

void AZMArenaBuilder::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	for (int32 i = 0; i < FlickerLights.Num(); ++i)
	{
		FlickerTimers[i] -= DeltaSeconds;
		if (FlickerTimers[i] > 0.f) continue;
		// Mostly on, with bursts of failing-bulb stutter.
		const bool bOut = FMath::FRand() < 0.12f;
		FlickerLights[i]->SetIntensity(bOut ? FlickerBase[i] * 0.08f : FlickerBase[i] * FMath::FRandRange(0.8f, 1.05f));
		FlickerTimers[i] = bOut ? FMath::FRandRange(0.03f, 0.12f) : FMath::FRandRange(0.05f, 0.6f);
	}
}
