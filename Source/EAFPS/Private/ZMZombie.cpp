#include "ZMZombie.h"
#include "ZMZombieAIController.h"
#include "ZMCharacter.h"
#include "ZMGameMode.h"
#include "ZMSettings.h"
#include "ZMTransientFX.h"
#include "ZMTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FName ZombieMeshProfile(TEXT("ZMZombieMesh"));
	constexpr float ZombieHitDamage = 50.f;
	constexpr float AttackRange = 115.f;
	constexpr float AttackReach = 150.f;
	constexpr float AttackWindupTime = 0.55f;
	constexpr float CorpseLifetime = 6.f;
}

AZMZombie::AZMZombie()
{
	PrimaryActorTick.bCanEverTick = true;

	AIControllerClass = AZMZombieAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	UCapsuleComponent* Capsule = GetCapsuleComponent();
	Capsule->InitCapsuleSize(34.f, 90.f);
	// Weapon traces hit the mesh / greybox parts, never the capsule.
	Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
	Capsule->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	bUseControllerRotationYaw = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = true;
	Move->RotationRate = FRotator(0.f, 420.f, 0.f);
	Move->MaxWalkSpeed = 110.f;
	Move->MaxAcceleration = 1400.f;
	Move->BrakingDecelerationWalking = 1400.f;
	Move->bUseRVOAvoidance = false;
	Move->SetUpdateNavAgentWithOwnersCollisions(true);

	USkeletalMeshComponent* SkelMesh = GetMesh();
	SkelMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));
	SkelMesh->SetCollisionProfileName(ZombieMeshProfile);
	SkelMesh->SetGenerateOverlapEvents(false);
	SkelMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	GreyboxRoot = CreateDefaultSubobject<USceneComponent>(TEXT("GreyboxRoot"));
	GreyboxRoot->SetupAttachment(Capsule);
}

void AZMZombie::BeginPlay()
{
	Super::BeginPlay();
	SetupVisuals();
	GroanTimer = FMath::FRandRange(1.f, 6.f);
	LastProgressLocation = GetActorLocation();
}

void AZMZombie::SetupVisuals()
{
	const UZMSettings* Settings = GetDefault<UZMSettings>();
	USkeletalMesh* SkMesh = Settings->ZombieMesh.IsNull() ? nullptr : Settings->ZombieMesh.LoadSynchronous();
	if (SkMesh)
	{
		bUsingSkeletalMesh = true;
		GetMesh()->SetSkeletalMeshAsset(SkMesh);
		if (UClass* AnimClass = Settings->ZombieAnimClass.IsNull() ? nullptr : Settings->ZombieAnimClass.LoadSynchronous())
		{
			GetMesh()->SetAnimInstanceClass(AnimClass);
		}
		const float Scale = FMath::FRandRange(0.94f, 1.06f);
		GetMesh()->SetRelativeScale3D(FVector(Scale));
		GreyboxRoot->SetVisibility(false, true);
		return;
	}

	// Greybox zombie: hunched torso, arms reaching forward.
	bUsingSkeletalMesh = false;
	GetMesh()->SetVisibility(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	const FLinearColor Skin(0.16f, 0.2f, 0.13f);
	const FLinearColor Cloth(0.07f, 0.065f, 0.06f);
	const FLinearColor Shirt(0.13f, 0.11f, 0.09f);
	auto AddPart = [this](UStaticMesh* PartMesh, const FVector& Loc, const FRotator& Rot, const FVector& Size, const FLinearColor& Color)
	{
		UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
		C->SetStaticMesh(PartMesh);
		C->SetupAttachment(GreyboxRoot);
		C->SetRelativeLocationAndRotation(Loc, Rot);
		C->SetRelativeScale3D(Size / 100.f);
		C->SetCollisionProfileName(ZombieMeshProfile);
		C->SetGenerateOverlapEvents(false);
		C->SetCanEverAffectNavigation(false);
		C->SetMaterial(0, ZM::MakeTintedMaterial(this, Color));
		C->RegisterComponent();
		GreyboxParts.Add(C);
		return C;
	};

	AddPart(ZM::CubeMesh(), FVector(0.f, -11.f, -48.f), FRotator(4.f, 0.f, 0.f), FVector(15.f, 15.f, 84.f), Cloth);
	AddPart(ZM::CubeMesh(), FVector(0.f, 11.f, -48.f), FRotator(-6.f, 0.f, 0.f), FVector(15.f, 15.f, 84.f), Cloth);
	AddPart(ZM::CubeMesh(), FVector(4.f, 0.f, 18.f), FRotator(-14.f, 0.f, 0.f), FVector(26.f, 42.f, 62.f), Shirt);
	AddPart(ZM::CubeMesh(), FVector(26.f, -24.f, 34.f), FRotator(-6.f, 4.f, 0.f), FVector(54.f, 10.f, 10.f), Skin);
	AddPart(ZM::CubeMesh(), FVector(24.f, 24.f, 30.f), FRotator(-12.f, -6.f, 0.f), FVector(52.f, 10.f, 10.f), Skin);
	GreyboxHead = AddPart(ZM::SphereMesh(), FVector(14.f, 0.f, 64.f), FRotator::ZeroRotator, FVector(25.f, 23.f, 28.f), Skin);
}

void AZMZombie::InitForRound(int32 Round, float InHealth)
{
	Health = InHealth;

	const float RunChance = Round < 3 ? 0.f : FMath::Clamp((Round - 2) * 0.3f, 0.f, 0.9f);
	const float SprintChance = Round < 8 ? 0.f : FMath::Clamp((Round - 7) * 0.08f, 0.f, 0.4f);
	const float Roll = FMath::FRand();
	if (Roll < SprintChance) Gait = EZMZombieGait::Sprint;
	else if (Roll < SprintChance + RunChance) Gait = EZMZombieGait::Run;
	else Gait = EZMZombieGait::Walk;

	float Speed = 105.f;
	switch (Gait)
	{
	case EZMZombieGait::Walk:   Speed = FMath::FRandRange(95.f, 120.f); break;
	case EZMZombieGait::Run:    Speed = FMath::FRandRange(225.f, 255.f); break;
	case EZMZombieGait::Sprint: Speed = FMath::FRandRange(320.f, 350.f); break;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed;
}

bool AZMZombie::IsHeadHit(const FHitResult& Hit) const
{
	if (bUsingSkeletalMesh)
	{
		const FString Bone = Hit.BoneName.ToString();
		return Bone.Contains(TEXT("head")) || Bone.Contains(TEXT("neck"));
	}
	return GreyboxHead && Hit.GetComponent() == GreyboxHead;
}

bool AZMZombie::ApplyHit(float Damage, bool bHead, bool bMelee, AZMCharacter* By, const FVector& HitLocation, const FVector& HitDir)
{
	if (bDead) return false;
	AZMGameMode* GM = GetWorld()->GetAuthGameMode<AZMGameMode>();
	if (GM && GM->IsInstaKill()) Damage = Health;

	Health -= Damage;
	if (Health <= 0.f)
	{
		Kill(By, bHead, bMelee, HitDir, true);
		return true;
	}
	if (GM) GM->OnZombieHit(By);
	return false;
}

void AZMZombie::Kill(AZMCharacter* By, bool bHead, bool bMelee, const FVector& HitDir, bool bAwardPoints, bool bRespawn)
{
	if (bDead) return;
	bDead = true;
	DeathTime = 0.f;

	if (AZMGameMode* GM = GetWorld()->GetAuthGameMode<AZMGameMode>())
	{
		GM->OnZombieKilled(this, By, bHead, bMelee, bAwardPoints, bRespawn);
	}

	DetachFromControllerPendingDestroy();
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (bRespawn)
	{
		// Stuck / culled: vanish quietly, the game mode queues a replacement.
		Destroy();
		return;
	}

	ZM::PlayWorldSound(this, "SFX_Zombie_Death", GetActorLocation(), 1.f, FMath::FRandRange(0.85f, 1.15f));

	if (bUsingSkeletalMesh)
	{
		USkeletalMeshComponent* M = GetMesh();
		if (bHead)
		{
			const FVector HeadLoc = M->GetBoneLocation(TEXT("head"));
			AZMTransientFX::SpawnImpact(GetWorld(), HeadLoc, FVector::UpVector, true);
			AZMTransientFX::SpawnImpact(GetWorld(), HeadLoc, HitDir, true);
			M->HideBoneByName(TEXT("head"), EPhysBodyOp::PBO_Term);
		}
		if (M->GetPhysicsAsset())
		{
			M->SetCollisionProfileName(TEXT("Ragdoll"));
			M->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
			M->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
			M->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
			M->SetSimulatePhysics(true);
			M->SetAllPhysicsLinearVelocity(HitDir.GetSafeNormal() * (bMelee ? 350.f : 220.f) + FVector(0.f, 0.f, 80.f), true);
		}
		else
		{
			M->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		for (UStaticMeshComponent* P : GreyboxParts)
		{
			if (P) P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		if (bHead && GreyboxHead)
		{
			AZMTransientFX::SpawnImpact(GetWorld(), GreyboxHead->GetComponentLocation(), FVector::UpVector, true);
			GreyboxHead->SetVisibility(false);
		}
		// Fall away from the shot.
		const FVector LocalDir = GetActorTransform().InverseTransformVectorNoScale(HitDir);
		LungeAlpha = LocalDir.X >= 0.f ? 1.f : -1.f;
	}
	SetLifeSpan(CorpseLifetime);
}

void AZMZombie::UpdateAttack(float DeltaSeconds, AZMCharacter* Target)
{
	AttackCooldown = FMath::Max(AttackCooldown - DeltaSeconds, 0.f);
	if (!Target) return;

	const FVector ToTarget = Target->GetActorLocation() - GetActorLocation();
	const float Dist2D = ToTarget.Size2D();
	const bool bInReach = FMath::Abs(ToTarget.Z) < 130.f;

	if (AttackWindup < 0.f)
	{
		LungeAlpha = FMath::FInterpTo(LungeAlpha, 0.f, DeltaSeconds, 6.f);
		if (AttackCooldown <= 0.f && bInReach && Dist2D < AttackRange && !Target->IsDowned())
		{
			AttackWindup = 0.f;
			ZM::PlayWorldSound(this, "SFX_Zombie_Attack", GetActorLocation(), 1.f, FMath::FRandRange(0.9f, 1.1f));
		}
		return;
	}

	// Wind up: face the player and swing.
	AttackWindup += DeltaSeconds;
	const FRotator Face(0.f, ToTarget.Rotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Face, DeltaSeconds, 10.f));
	LungeAlpha = FMath::Clamp(AttackWindup / AttackWindupTime, 0.f, 1.f);

	if (AttackWindup >= AttackWindupTime)
	{
		if (bInReach && Dist2D < AttackReach && !Target->IsDowned())
		{
			Target->ReceiveZombieHit(ZombieHitDamage, GetActorLocation());
		}
		AttackWindup = -1.f;
		AttackCooldown = 1.0f;
	}
}

void AZMZombie::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bDead)
	{
		DeathTime += DeltaSeconds;
		if (!bUsingSkeletalMesh)
		{
			const float A = FMath::Clamp(DeathTime / 0.45f, 0.f, 1.f);
			const float Ease = A * A;
			GreyboxRoot->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -60.f * Ease), FRotator(-85.f * Ease * LungeAlpha, 0.f, 0.f));
		}
		// Corpses sink out of sight before they are removed.
		if (DeathTime > CorpseLifetime - 1.5f)
		{
			AddActorWorldOffset(FVector(0.f, 0.f, -40.f * DeltaSeconds));
		}
		return;
	}

	AZMCharacter* Target = Cast<AZMCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
	UpdateAttack(DeltaSeconds, Target);

	if (!bUsingSkeletalMesh)
	{
		// Shamble sway and attack lunge on the greybox body.
		const float T = GetWorld()->GetTimeSeconds() + GetUniqueID() * 0.37f;
		const float Speed = GetVelocity().Size2D();
		const float Sway = FMath::Sin(T * (Speed > 180.f ? 9.f : 4.5f)) * FMath::Clamp(Speed / 120.f, 0.f, 1.f);
		GreyboxRoot->SetRelativeLocationAndRotation(FVector(LungeAlpha * 12.f, 0.f, 0.f), FRotator(-LungeAlpha * 18.f, 0.f, Sway * 6.f));
	}

	GroanTimer -= DeltaSeconds;
	if (GroanTimer <= 0.f)
	{
		GroanTimer = FMath::FRandRange(3.5f, 9.f);
		ZM::PlayWorldSound(this, "SFX_Zombie_Groan", GetActorLocation(), 0.8f, FMath::FRandRange(0.8f, 1.15f));
	}

	// Stuck check: if we made no progress for a while and the player is not close, respawn elsewhere.
	StuckTimer += DeltaSeconds;
	if (StuckTimer >= 6.f)
	{
		StuckTimer = 0.f;
		const bool bNoProgress = FVector::Dist2D(GetActorLocation(), LastProgressLocation) < 80.f;
		const bool bFarFromPlayer = !Target || FVector::Dist(Target->GetActorLocation(), GetActorLocation()) > 600.f;
		LastProgressLocation = GetActorLocation();
		if (bNoProgress && bFarFromPlayer)
		{
			Kill(nullptr, false, false, FVector::ZeroVector, false, true);
			return;
		}
	}
	// Fell out of the world.
	if (GetActorLocation().Z < -2000.f)
	{
		Kill(nullptr, false, false, FVector::ZeroVector, false, true);
	}
}
