#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZMTypes.h"
#include "ZMWeapon.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class AZMCharacter;

/** A first-person weapon. Hitscan, owned and driven by AZMCharacter. */
UCLASS()
class EAFPS_API AZMWeapon : public AActor
{
	GENERATED_BODY()

public:
	AZMWeapon();

	void Init(const FZMWeaponStats& InStats, AZMCharacter* InOwner);
	virtual void Tick(float DeltaSeconds) override;

	const FZMWeaponStats& GetStats() const { return Stats; }
	int32 GetMag() const { return Mag; }
	int32 GetReserve() const { return Reserve; }
	bool IsReloading() const { return ReloadRemaining > 0.f; }
	float GetReloadAlpha() const;
	float GetCurrentSpread() const { return CurrentSpread; }

	void SetTriggerHeld(bool bHeld);
	void StartReload();
	void CancelReload();
	void RefillAmmo();
	bool IsAmmoFull() const { return Mag == Stats.MagSize && Reserve == Stats.MaxReserve; }

	/** Visual kick the character applies to the viewmodel (decays over time). */
	FVector GetKickOffset() const { return KickOffset; }
	FRotator GetKickRotation() const { return KickRotation; }

	/** Creates the greybox model (body, barrel, magazine, sights) for a weapon. */
	static void BuildModel(AActor* Owner, USceneComponent* Parent, const FZMWeaponStats& Stats, bool bOwnerOnly);

protected:
	void TryFire();
	void FireShot();
	void FinishReload();

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> ModelRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> MuzzleLight;

	UPROPERTY() TObjectPtr<AZMCharacter> OwnerCharacter;

	FZMWeaponStats Stats;
	int32 Mag = 0;
	int32 Reserve = 0;
	bool bTriggerHeld = false;
	bool bShotThisPress = false;
	float FireCooldown = 0.f;
	float ReloadRemaining = 0.f;
	float ReloadDuration = 0.f;
	float CurrentSpread = 0.f;
	float MuzzleFlashTime = 0.f;
	FVector KickOffset = FVector::ZeroVector;
	FRotator KickRotation = FRotator::ZeroRotator;
};
