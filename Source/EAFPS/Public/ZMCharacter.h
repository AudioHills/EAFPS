#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZMTypes.h"
#include "ZMCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class AZMWeapon;
class AZMInteractable;
struct FInputActionValue;

UCLASS()
class EAFPS_API AZMCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZMCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;
	virtual FVector GetPawnViewLocation() const override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void Landed(const FHitResult& Hit) override;

	// --- Queries used by weapons / HUD ---
	float GetAdsAlpha() const { return AdsAlpha; }
	float GetMoveSpreadFactor() const;
	bool CanFire() const;
	bool IsSprinting() const { return bSprinting; }
	bool IsDowned() const { return bDowned; }
	AZMWeapon* GetCurrentWeapon() const;
	const TArray<TObjectPtr<AZMWeapon>>& GetWeapons() const { return Weapons; }
	float GetHealth() const { return Health; }
	float GetMaxHealth() const { return MaxHealth; }
	float GetHitMarkerTime() const { return HitMarkerTime; }
	bool WasHitMarkerKill() const { return bHitMarkerKill; }
	float GetDamageFlash() const { return DamageFlash; }
	FVector GetLastDamageDirection() const { return LastDamageDir; }
	float GetDownedTime() const { return DownedTime; }
	float GetLookSensitivity() const { return LookSensitivity; }
	float GetSensitivityToastTime() const { return SensToast; }
	AZMInteractable* GetFocusedInteractable() const { return FocusedInteractable; }
	const TArray<EZMPerk>& GetPerks() const { return Perks; }
	bool HasPerk(EZMPerk Perk) const { return Perks.Contains(Perk); }

	// --- Actions ---
	void AddRecoil(float Pitch, float Yaw);
	void NotifyHitMarker(bool bKill, bool bHead);
	void OnWeaponFired();
	void ReceiveZombieHit(float Damage, const FVector& FromLocation);

	/** Gives a weapon (replacing the current one if both slots are full) or refills ammo if owned. */
	void GiveWeapon(FName WeaponId);
	bool HasWeapon(FName WeaponId) const;
	AZMWeapon* FindWeapon(FName WeaponId) const;
	void GivePerk(EZMPerk Perk);
	void RefillAllAmmo();
	int32 GetMaxWeaponSlots() const { return 2; }

protected:
	void BuildInput();

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_GamepadLook(const FInputActionValue& Value);
	void Input_SensUp();
	void Input_SensDown();
	void Input_JumpStart();
	void Input_SprintStart();
	void Input_CrouchStart();
	void Input_AdsStart();
	void Input_AdsStop();
	void Input_FireStart();
	void Input_FireStop();
	void Input_Reload();
	void Input_Interact();
	void Input_Melee();
	void Input_NextWeapon();
	void Input_Slot1();
	void Input_Slot2();

	void SwitchToWeapon(int32 Index);
	void StartSlide();
	void EndSlide();
	void UpdateMovement(float DeltaSeconds);
	void UpdateCamera(float DeltaSeconds);
	void UpdateViewModel(float DeltaSeconds);
	void UpdateInteraction();
	void UpdateHealth(float DeltaSeconds);
	void DoMeleeHit();
	void GoDown();
	void Revive();
	AZMWeapon* SpawnWeapon(FName WeaponId);

	UPROPERTY(VisibleAnywhere, Category = "Camera") TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, Category = "Camera") TObjectPtr<USceneComponent> ViewModelRoot;

	UPROPERTY() TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY() TObjectPtr<UInputAction> MoveAction;
	UPROPERTY() TObjectPtr<UInputAction> LookAction;
	UPROPERTY() TObjectPtr<UInputAction> GamepadLookAction;
	UPROPERTY() TObjectPtr<UInputAction> SensUpAction;
	UPROPERTY() TObjectPtr<UInputAction> SensDownAction;
	UPROPERTY() TObjectPtr<UInputAction> JumpAction;
	UPROPERTY() TObjectPtr<UInputAction> SprintAction;
	UPROPERTY() TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY() TObjectPtr<UInputAction> AdsAction;
	UPROPERTY() TObjectPtr<UInputAction> FireAction;
	UPROPERTY() TObjectPtr<UInputAction> ReloadAction;
	UPROPERTY() TObjectPtr<UInputAction> InteractAction;
	UPROPERTY() TObjectPtr<UInputAction> MeleeAction;
	UPROPERTY() TObjectPtr<UInputAction> NextWeaponAction;
	UPROPERTY() TObjectPtr<UInputAction> Slot1Action;
	UPROPERTY() TObjectPtr<UInputAction> Slot2Action;

	UPROPERTY() TArray<TObjectPtr<AZMWeapon>> Weapons;
	UPROPERTY() TObjectPtr<AZMInteractable> FocusedInteractable;

	TArray<EZMPerk> Perks;
	int32 CurrentWeaponIndex = 0;

	// Tuning (cm, cm/s, degrees)
	UPROPERTY(EditAnywhere, Category = "Tuning") float BaseFOV = 80.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float SprintFOVBoost = 6.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float LookSensitivity = 1.0f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float GamepadLookRate = 160.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float JogSpeed = 420.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float SprintSpeed = 630.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float AdsSpeed = 230.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float CrouchSpeed = 220.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float SlideSpeed = 920.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float SlideDuration = 0.75f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float StandEyeHeight = 162.f; // above floor
	UPROPERTY(EditAnywhere, Category = "Tuning") float CrouchEyeHeight = 102.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float SlideEyeHeight = 86.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float RegenDelay = 3.0f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float RegenRate = 90.f; // hp/s
	UPROPERTY(EditAnywhere, Category = "Tuning") float MeleeDamage = 150.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float MeleeRange = 140.f;
	UPROPERTY(EditAnywhere, Category = "Tuning") float InteractRange = 190.f;

	// State
	FVector2D MoveInput = FVector2D::ZeroVector;
	FVector2D LookDelta = FVector2D::ZeroVector;
	bool bSprinting = false;
	bool bAdsHeld = false;
	bool bFireHeld = false;
	bool bSliding = false;
	bool bDowned = false;
	float AdsAlpha = 0.f;
	float SprintAlpha = 0.f;
	float SprintOutRemaining = 0.f;
	float SlideTime = 0.f;
	FVector SlideDir = FVector::ForwardVector;
	float EyeHeightAboveFloor = 162.f;
	float CurrentFOV = 80.f;
	float PendingRecoilPitch = 0.f;
	float PendingRecoilYaw = 0.f;
	float Health = 100.f;
	float MaxHealth = 100.f;
	float TimeSinceDamage = 100.f;
	float DamageFlash = 0.f;
	FVector LastDamageDir = FVector::ZeroVector;
	float HitMarkerTime = 0.f;
	bool bHitMarkerKill = false;
	float MeleeCooldown = 0.f;
	float MeleeAnim = 0.f;
	bool bMeleePendingHit = false;
	float SwitchAnim = 0.f;   // 1 -> 0 while raising a new weapon
	float DownedTime = 0.f;
	float BobPhase = 0.f;
	float LandDip = 0.f;
	FRotator SwayRot = FRotator::ZeroRotator;
	FVector SwayLoc = FVector::ZeroVector;
	float WallLower = 0.f;
	float SensToast = 0.f;
	float SlideTilt = 0.f;
};
