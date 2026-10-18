#include "ZMWeapon.h"
#include "ZMCharacter.h"
#include "ZMZombie.h"
#include "ZMTransientFX.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AZMWeapon::AZMWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	ModelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ModelRoot"));
	ModelRoot->SetupAttachment(Root);

	MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleLight"));
	MuzzleLight->SetupAttachment(Root);
	MuzzleLight->SetIntensity(0.f);
	MuzzleLight->SetLightColor(FLinearColor(1.f, 0.72f, 0.38f));
	MuzzleLight->SetAttenuationRadius(600.f);
	MuzzleLight->SetCastShadows(false);
}

void AZMWeapon::Init(const FZMWeaponStats& InStats, AZMCharacter* InOwner)
{
	Stats = InStats;
	OwnerCharacter = InOwner;
	SetOwner(InOwner);
	Mag = Stats.MagSize;
	Reserve = Stats.MaxReserve;
	MuzzleLight->SetRelativeLocation(FVector(Stats.BodySize.X * 0.7f + 14.f, 0.f, 0.f));
	BuildModel(this, ModelRoot, Stats, true);
}

void AZMWeapon::BuildModel(AActor* Owner, USceneComponent* Parent, const FZMWeaponStats& S, bool bOwnerOnly)
{
	// Greybox model, origin at the sight line's rear. Cube = 100uu, cylinder = 100uu tall along Z.
	const float L = S.BodySize.X, W = S.BodySize.Y, H = S.BodySize.Z;
	const FLinearColor Dark = S.Tint;
	const FLinearColor Mid = S.Tint * 2.2f + FLinearColor(0.02f, 0.02f, 0.02f);

	auto AddPart = [&](UStaticMesh* Mesh, const FVector& Loc, const FRotator& Rot, const FVector& Size, const FLinearColor& Color)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(Owner);
		C->SetStaticMesh(Mesh);
		C->SetupAttachment(Parent);
		C->SetRelativeLocation(Loc);
		C->SetRelativeRotation(Rot);
		C->SetRelativeScale3D(Size / 100.f);
		C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		C->SetGenerateOverlapEvents(false);
		C->SetCastShadow(!bOwnerOnly);
		C->SetOnlyOwnerSee(bOwnerOnly);
		C->SetMaterial(0, ZM::MakeTintedMaterial(Owner, Color));
		C->RegisterComponent();
	};

	const float BodyCenter = L * 0.2f;
	AddPart(ZM::CubeMesh(), FVector(BodyCenter, 0, 0), FRotator::ZeroRotator, FVector(L, W, H), Dark);
	// Barrel
	const float BarrelLen = L * 0.45f;
	const float BarrelDiam = FMath::Max(W * 0.45f, 1.6f);
	AddPart(ZM::CylinderMesh(), FVector(BodyCenter + L * 0.5f + BarrelLen * 0.5f, 0, H * 0.15f), FRotator(90, 0, 0),
		FVector(BarrelDiam, BarrelDiam, BarrelLen), Mid);
	// Magazine / grip
	const bool bPistol = S.Class == EZMWeaponClass::Pistol;
	AddPart(ZM::CubeMesh(), FVector(bPistol ? -L * 0.15f : BodyCenter + L * 0.05f, 0, -H * 0.5f - H * 0.6f),
		FRotator(bPistol ? -12.f : 8.f, 0, 0), FVector(bPistol ? L * 0.28f : L * 0.14f, W * 0.8f, H * 1.3f), Mid);
	if (!bPistol)
	{
		// Pistol grip and stock
		AddPart(ZM::CubeMesh(), FVector(BodyCenter - L * 0.25f, 0, -H * 0.9f), FRotator(-15.f, 0, 0), FVector(L * 0.1f, W * 0.8f, H * 1.1f), Dark);
		AddPart(ZM::CubeMesh(), FVector(BodyCenter - L * 0.62f, 0, -H * 0.2f), FRotator::ZeroRotator, FVector(L * 0.3f, W * 0.9f, H * 0.9f), Mid);
	}
	// Sights: rear notch (two posts) and front post, top at GetSightHeight().
	const float PostH = 1.6f;
	const float TopZ = H * 0.5f + PostH * 0.5f;
	AddPart(ZM::CubeMesh(), FVector(0.f, -0.55f, TopZ), FRotator::ZeroRotator, FVector(0.6f, 0.5f, PostH), Mid);
	AddPart(ZM::CubeMesh(), FVector(0.f, 0.55f, TopZ), FRotator::ZeroRotator, FVector(0.6f, 0.5f, PostH), Mid);
	AddPart(ZM::CubeMesh(), FVector(BodyCenter + L * 0.45f, 0.f, TopZ - 0.2f), FRotator::ZeroRotator, FVector(0.6f, 0.35f, PostH), FLinearColor(0.9f, 0.9f, 0.2f));
}

float AZMWeapon::GetReloadAlpha() const
{
	return ReloadDuration > 0.f ? 1.f - ReloadRemaining / ReloadDuration : 0.f;
}

void AZMWeapon::SetTriggerHeld(bool bHeld)
{
	bTriggerHeld = bHeld;
	if (!bHeld)
	{
		bShotThisPress = false;
	}
}

void AZMWeapon::StartReload()
{
	if (IsReloading() || Mag >= Stats.MagSize || Reserve <= 0) return;
	const float Scale = (OwnerCharacter && OwnerCharacter->HasPerk(EZMPerk::QuickHands)) ? 0.5f : 1.f;
	ReloadDuration = (Mag > 0 ? Stats.ReloadTime : Stats.EmptyReloadTime) * Scale;
	ReloadRemaining = ReloadDuration;
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Reload_Out"));
}

void AZMWeapon::CancelReload()
{
	ReloadRemaining = 0.f;
	ReloadDuration = 0.f;
}

void AZMWeapon::FinishReload()
{
	const int32 Take = FMath::Min(Stats.MagSize - Mag, Reserve);
	Mag += Take;
	Reserve -= Take;
	ReloadDuration = 0.f;
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Reload_In"));
}

void AZMWeapon::RefillAmmo()
{
	Reserve = Stats.MaxReserve;
	if (!IsReloading() && Mag == 0)
	{
		StartReload();
	}
}

void AZMWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (ReloadRemaining > 0.f)
	{
		ReloadRemaining -= DeltaSeconds;
		if (ReloadRemaining <= 0.f)
		{
			ReloadRemaining = 0.f;
			FinishReload();
		}
	}

	const float AdsAlpha = OwnerCharacter ? OwnerCharacter->GetAdsAlpha() : 0.f;
	const float MoveFactor = OwnerCharacter ? OwnerCharacter->GetMoveSpreadFactor() : 0.f;
	const float Base = FMath::Lerp(Stats.HipSpread, Stats.AdsSpread, AdsAlpha) + Stats.MoveSpreadAdd * MoveFactor * (1.f - AdsAlpha);
	// Bloom decays quickly; CurrentSpread never goes below the stance base.
	const float Bloom = FMath::Max(CurrentSpread - Base, 0.f);
	CurrentSpread = Base + FMath::FInterpTo(Bloom, 0.f, DeltaSeconds, 7.f);

	if (bTriggerHeld)
	{
		FireCooldown -= DeltaSeconds;
		TryFire();
	}
	else
	{
		FireCooldown = FMath::Max(FireCooldown - DeltaSeconds, 0.f);
	}

	KickOffset = FMath::VInterpTo(KickOffset, FVector::ZeroVector, DeltaSeconds, 16.f);
	KickRotation = FMath::RInterpTo(KickRotation, FRotator::ZeroRotator, DeltaSeconds, 14.f);

	if (MuzzleFlashTime > 0.f)
	{
		MuzzleFlashTime -= DeltaSeconds;
		MuzzleLight->SetIntensity(MuzzleFlashTime > 0.f ? 25000.f : 0.f);
	}
}

void AZMWeapon::TryFire()
{
	if (FireCooldown > 0.f || IsReloading()) return;
	if (!Stats.bAutomatic && bShotThisPress) return;
	if (!OwnerCharacter || !OwnerCharacter->CanFire()) return;

	if (Mag <= 0)
	{
		if (!bShotThisPress)
		{
			UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Dry_Fire"));
			bShotThisPress = true;
		}
		StartReload();
		return;
	}

	FireShot();
	bShotThisPress = true;
	const float RateScale = OwnerCharacter->HasPerk(EZMPerk::DoubleTap) ? 1.33f : 1.f;
	FireCooldown += Stats.GetFireInterval() / RateScale;
}

void AZMWeapon::FireShot()
{
	--Mag;
	UWorld* World = GetWorld();

	FVector EyeLoc;
	FRotator EyeRot;
	OwnerCharacter->GetActorEyesViewPoint(EyeLoc, EyeRot);
	const FVector Aim = EyeRot.Vector();

	FCollisionQueryParams Query(SCENE_QUERY_STAT(ZMWeaponTrace), false);
	Query.AddIgnoredActor(this);
	Query.AddIgnoredActor(OwnerCharacter);

	const FVector Muzzle = GetActorLocation() + GetActorForwardVector() * (Stats.BodySize.X * 0.7f + 14.f);
	const float SpreadRad = FMath::DegreesToRadians(FMath::Max(CurrentSpread, 0.01f));

	bool bAnyHit = false, bAnyKill = false, bAnyHead = false;
	for (int32 Pellet = 0; Pellet < Stats.Pellets; ++Pellet)
	{
		const FVector Dir = FMath::VRandCone(Aim, SpreadRad);
		const FVector End = EyeLoc + Dir * 20000.f;
		FHitResult Hit;
		const bool bHit = World->LineTraceSingleByChannel(Hit, EyeLoc, End, ECC_Visibility, Query);
		const FVector ImpactPoint = bHit ? Hit.ImpactPoint : End;

		if (Pellet == 0 && (Stats.Pellets == 1) && (Mag % 2 == 0 || !Stats.bAutomatic))
		{
			AZMTransientFX::SpawnTracer(World, Muzzle, ImpactPoint);
		}
		if (!bHit) continue;

		if (AZMZombie* Zombie = Cast<AZMZombie>(Hit.GetActor()))
		{
			const float Dist = Hit.Distance;
			const float Falloff = FMath::GetMappedRangeValueClamped(FVector2D(Stats.FalloffStart, Stats.FalloffEnd), FVector2D(1.f, Stats.MinDamageScale), Dist);
			const bool bHead = Zombie->IsHeadHit(Hit);
			float Damage = Stats.Damage * Falloff * (bHead ? Stats.HeadshotMultiplier : 1.f);
			const bool bKilled = Zombie->ApplyHit(Damage, bHead, false, OwnerCharacter, Hit.ImpactPoint, Dir);
			bAnyHit = true;
			bAnyKill |= bKilled;
			bAnyHead |= bHead;
			AZMTransientFX::SpawnImpact(World, Hit.ImpactPoint, -Dir, true);
		}
		else
		{
			AZMTransientFX::SpawnImpact(World, Hit.ImpactPoint, Hit.ImpactNormal, false);
			if (Pellet == 0)
			{
				ZM::PlayWorldSound(this, "SFX_Impact", Hit.ImpactPoint, 0.6f, FMath::FRandRange(0.9f, 1.1f));
			}
		}
	}

	if (bAnyHit)
	{
		OwnerCharacter->NotifyHitMarker(bAnyKill, bAnyHead);
	}

	// Recoil: camera climb (applied over a few frames by the character) plus viewmodel kick.
	const float RecoilScale = FMath::Lerp(1.f, Stats.AdsRecoilScale, OwnerCharacter->GetAdsAlpha());
	const float Yaw = FMath::FRandRange(-Stats.RecoilYawRange, Stats.RecoilYawRange) + Stats.RecoilYawBias;
	OwnerCharacter->AddRecoil(Stats.RecoilPitch * RecoilScale * FMath::FRandRange(0.85f, 1.1f), Yaw * RecoilScale);
	CurrentSpread += Stats.SpreadPerShot;

	const float KickScale = FMath::Lerp(1.f, 0.45f, OwnerCharacter->GetAdsAlpha());
	KickOffset += FVector(-1.4f, 0.f, 0.25f) * Stats.VisualKick * KickScale;
	KickRotation += FRotator(Stats.VisualKick * 1.6f * KickScale, FMath::FRandRange(-0.4f, 0.4f) * Stats.VisualKick, FMath::FRandRange(-1.f, 1.f) * Stats.VisualKick);

	MuzzleFlashTime = 0.04f;
	MuzzleLight->SetIntensity(25000.f);

	UGameplayStatics::PlaySound2D(this, ZM::LoadSound(Stats.FireSound), 1.f, FMath::FRandRange(0.96f, 1.04f));
	if (Mag > 0 && Mag <= FMath::Max(Stats.MagSize / 5, 2))
	{
		UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Low_Ammo"), 0.5f);
	}
	OwnerCharacter->OnWeaponFired();
}
