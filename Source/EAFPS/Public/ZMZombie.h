#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZMZombie.generated.h"

class AZMCharacter;
class UStaticMeshComponent;

UENUM()
enum class EZMZombieGait : uint8 { Walk, Run, Sprint };

UCLASS()
class EAFPS_API AZMZombie : public ACharacter
{
	GENERATED_BODY()

public:
	AZMZombie();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	void InitForRound(int32 Round, float InHealth);
	bool IsHeadHit(const FHitResult& Hit) const;
	/** Returns true if this hit killed the zombie. Awards points through the game mode. */
	bool ApplyHit(float Damage, bool bHead, bool bMelee, AZMCharacter* By, const FVector& HitLocation, const FVector& HitDir);
	void Kill(AZMCharacter* By, bool bHead, bool bMelee, const FVector& HitDir, bool bAwardPoints, bool bRespawn = false);
	bool IsDead() const { return bDead; }
	EZMZombieGait GetGait() const { return Gait; }

protected:
	void SetupVisuals();
	void UpdateAttack(float DeltaSeconds, AZMCharacter* Target);

	/** Greybox body used when no skeletal mesh is configured. */
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> GreyboxRoot;
	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> GreyboxParts;
	UPROPERTY() TObjectPtr<UStaticMeshComponent> GreyboxHead;

	float Health = 150.f;
	bool bDead = false;
	bool bUsingSkeletalMesh = false;
	EZMZombieGait Gait = EZMZombieGait::Walk;
	float AttackWindup = -1.f;
	float AttackCooldown = 0.f;
	float GroanTimer = 0.f;
	float DeathTime = 0.f;
	float StuckTimer = 0.f;
	FVector LastProgressLocation = FVector::ZeroVector;
	float LungeAlpha = 0.f;
};
