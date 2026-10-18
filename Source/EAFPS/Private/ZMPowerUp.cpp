#include "ZMPowerUp.h"
#include "ZMCharacter.h"
#include "ZMGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	constexpr float PowerUpLifetime = 30.f;
	constexpr float PowerUpBlinkStart = 22.f;
	constexpr float PickupRadius = 90.f;
}

AZMPowerUp::AZMPowerUp()
{
	PrimaryActorTick.bCanEverTick = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetGenerateOverlapEvents(false);
	Mesh->SetCanEverAffectNavigation(false);
	Mesh->SetRelativeScale3D(FVector(0.32f));
	RootComponent = Mesh;

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Mesh);
	Glow->SetUsingAbsoluteScale(true);
	Glow->SetIntensity(6000.f);
	Glow->SetAttenuationRadius(350.f);
	Glow->SetCastShadows(false);

	Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	Label->SetupAttachment(Mesh);
	Label->SetUsingAbsoluteScale(true);
	Label->SetUsingAbsoluteRotation(true);
	Label->SetHorizontalAlignment(EHTA_Center);
	Label->SetVerticalAlignment(EVRTA_TextCenter);
	Label->SetWorldSize(12.f);
	Label->SetRelativeLocation(FVector(0.f, 0.f, 45.f));
	Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AZMPowerUp::Init(EZMPowerUp InType)
{
	Type = InType;
	BaseLocation = GetActorLocation();
	const FLinearColor C = ZM::PowerUpColor(Type);
	Mesh->SetStaticMesh(Type == EZMPowerUp::Nuke ? ZM::SphereMesh() : ZM::CubeMesh());
	Mesh->SetMaterial(0, ZM::MakeTintedMaterial(this, C));
	Glow->SetLightColor(C);
	Label->SetTextRenderColor(C.ToFColor(true));
	Label->SetText(ZM::PowerUpName(Type));
	ZM::PlayWorldSound(this, "SFX_PowerUp_Spawn", BaseLocation);
}

void AZMPowerUp::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;

	SetActorLocation(BaseLocation + FVector(0.f, 0.f, FMath::Sin(Age * 2.5f) * 8.f));
	AddActorWorldRotation(FRotator(0.f, 90.f * DeltaSeconds, 0.f));

	// Label always faces the player.
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (Pawn)
	{
		const FVector To = Pawn->GetActorLocation() - Label->GetComponentLocation();
		Label->SetWorldRotation(FRotator(0.f, To.Rotation().Yaw, 0.f));
	}

	if (Age > PowerUpBlinkStart)
	{
		const float Rate = Age > PowerUpLifetime - 3.f ? 12.f : 5.f;
		const bool bVisible = FMath::Sin(Age * Rate) > -0.3f;
		Mesh->SetVisibility(bVisible, true);
	}
	if (Age >= PowerUpLifetime)
	{
		Destroy();
		return;
	}

	AZMCharacter* Player = Cast<AZMCharacter>(Pawn);
	if (Player && !Player->IsDowned() && FVector::Dist2D(Player->GetActorLocation(), BaseLocation) < PickupRadius
		&& FMath::Abs(Player->GetActorLocation().Z - BaseLocation.Z) < 200.f)
	{
		if (AZMGameMode* GM = GetWorld()->GetAuthGameMode<AZMGameMode>())
		{
			GM->ApplyPowerUp(Type, Player);
		}
		Destroy();
	}
}
