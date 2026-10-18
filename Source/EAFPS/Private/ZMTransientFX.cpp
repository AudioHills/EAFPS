#include "ZMTransientFX.h"
#include "ZMTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

AZMTransientFX::AZMTransientFX()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetGenerateOverlapEvents(false);
	RootComponent = Mesh;

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Mesh);
	Light->SetIntensity(0.f);
	Light->SetCastShadows(false);
	Light->SetAttenuationRadius(250.f);
	Light->SetUsingAbsoluteScale(true);
}

void AZMTransientFX::SpawnImpact(UWorld* World, const FVector& Location, const FVector& Normal, bool bFlesh)
{
	if (!World) return;
	const int32 Count = bFlesh ? 3 : 2;
	for (int32 i = 0; i < Count; ++i)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AZMTransientFX* FX = World->SpawnActor<AZMTransientFX>(Location + Normal * 2.f, FRotator::ZeroRotator, Params);
		if (!FX) continue;

		FX->Mesh->SetStaticMesh(bFlesh ? ZM::SphereMesh() : ZM::CubeMesh());
		const FLinearColor Color = bFlesh ? FLinearColor(0.25f, 0.01f, 0.01f) : FLinearColor(0.35f, 0.33f, 0.3f);
		FX->Mesh->SetMaterial(0, ZM::MakeTintedMaterial(FX, Color));
		const float Size = FMath::FRandRange(0.03f, bFlesh ? 0.09f : 0.05f);
		FX->StartScale = FVector(Size);
		FX->EndScale = FVector(Size * (bFlesh ? 0.2f : 0.1f));
		FX->Life = FMath::FRandRange(0.18f, 0.35f);
		FX->Velocity = (Normal + FMath::VRand() * 0.6f).GetSafeNormal() * FMath::FRandRange(120.f, 380.f);
		if (i == 0 && !bFlesh)
		{
			FX->Light->SetLightColor(FLinearColor(1.f, 0.7f, 0.35f));
			FX->StartLight = 1500.f;
		}
		FX->Mesh->SetWorldScale3D(FX->StartScale);
	}
}

void AZMTransientFX::SpawnTracer(UWorld* World, const FVector& Start, const FVector& End)
{
	if (!World) return;
	const FVector Delta = End - Start;
	const float Length = Delta.Size();
	if (Length < 200.f) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	// Tracer streak covers a slice of the path near the muzzle; cylinder mesh is 100 units along Z.
	const float StreakLen = FMath::Min(Length * 0.5f, 900.f);
	const FVector Dir = Delta / Length;
	const FVector Mid = Start + Dir * (150.f + StreakLen * 0.5f);
	AZMTransientFX* FX = World->SpawnActor<AZMTransientFX>(Mid, FRotationMatrix::MakeFromZ(Dir).Rotator(), Params);
	if (!FX) return;
	FX->Mesh->SetStaticMesh(ZM::CylinderMesh());
	FX->Mesh->SetMaterial(0, ZM::MakeTintedMaterial(FX, FLinearColor(1.f, 0.8f, 0.45f)));
	FX->StartScale = FVector(0.006f, 0.006f, StreakLen / 100.f);
	FX->EndScale = FVector(0.002f, 0.002f, StreakLen / 100.f);
	FX->Velocity = Dir * 25000.f;
	FX->Life = 0.05f;
	FX->Mesh->SetWorldScale3D(FX->StartScale);
}

void AZMTransientFX::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	const float A = FMath::Clamp(Age / Life, 0.f, 1.f);
	Mesh->SetWorldScale3D(FMath::Lerp(StartScale, EndScale, A));
	Velocity.Z -= 600.f * DeltaSeconds;
	AddActorWorldOffset(Velocity * DeltaSeconds);
	Light->SetIntensity(StartLight * (1.f - A));
	if (Age >= Life)
	{
		Destroy();
	}
}
