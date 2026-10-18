#pragma once

#include "CoreMinimal.h"
#include "ZMTypes.generated.h"

UENUM(BlueprintType)
enum class EZMWeaponClass : uint8
{
	Pistol,
	SMG,
	Rifle,
	Shotgun,
	LMG,
	Sniper
};

UENUM(BlueprintType)
enum class EZMPerk : uint8
{
	IronGut,     // max health 100 -> 250
	QuickHands,  // reload speed x2
	DoubleTap,   // fire rate x1.33
	SecondWind   // solo self-revive
};

UENUM(BlueprintType)
enum class EZMPowerUp : uint8
{
	MaxAmmo,
	InstaKill,
	DoublePoints,
	Nuke
};

USTRUCT(BlueprintType)
struct FZMWeaponStats
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName Id;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText DisplayName;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) EZMWeaponClass Class = EZMWeaponClass::Pistol;

	UPROPERTY(EditAnywhere, BlueprintReadOnly) float Damage = 40.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HeadshotMultiplier = 2.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 Pellets = 1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RoundsPerMinute = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bAutomatic = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MagSize = 30;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MaxReserve = 180;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float ReloadTime = 2.0f;      // tactical (round chambered)
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float EmptyReloadTime = 2.6f; // from empty
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float FalloffStart = 1500.f;  // cm
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float FalloffEnd = 4000.f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MinDamageScale = 0.6f;

	// Spread is a cone half-angle in degrees.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float HipSpread = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AdsSpread = 0.15f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MoveSpreadAdd = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SpreadPerShot = 0.35f;

	// Recoil in degrees per shot. Yaw is drawn from [-YawRange, +YawRange] biased by YawBias.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RecoilPitch = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RecoilYawRange = 0.25f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float RecoilYawBias = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AdsRecoilScale = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float VisualKick = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AdsTime = 0.22f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float AdsZoom = 1.25f;      // FOV divisor while aimed
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float SprintOutTime = 0.18f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) float MoveSpeedScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 WallCost = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bInMysteryBox = true;

	// Name of the sound asset under /Game/Audio (imported by Scripts/Setup.py).
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName FireSound = TEXT("SFX_Rifle_Fire");

	// First-person hip placement relative to the camera (cm). The ADS position is derived from the
	// model so the sight line always sits exactly on the camera axis.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector HipOffset = FVector(28.f, 12.f, -14.f);
	// Greybox body size (length, width, height) for the placeholder model.
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FVector BodySize = FVector(40.f, 4.f, 7.f);
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FLinearColor Tint = FLinearColor(0.09f, 0.09f, 0.095f);

	float GetFireInterval() const { return 60.f / FMath::Max(RoundsPerMinute, 1.f); }
	/** Hip position, pushed forward for long guns so the stock never sits against the camera. */
	FVector GetHipOffset() const
	{
		const float RearExtent = (Class == EZMWeaponClass::Pistol ? 0.3f : 0.57f) * BodySize.X;
		return FVector(FMath::Max(HipOffset.X, RearExtent + 16.f), HipOffset.Y, HipOffset.Z);
	}
	/** Height of the sight posts above the model origin. */
	float GetSightHeight() const { return BodySize.Z * 0.5f + 1.6f; }
	/** Model origin offset from the camera when fully aimed. */
	FVector GetAdsOffset() const { return FVector(10.f + BodySize.X * 0.3f, 0.f, -GetSightHeight()); }
};

namespace ZM
{
	/** Every weapon in the game. Wall buys and the mystery box reference these by Id. */
	EAFPS_API const TArray<FZMWeaponStats>& GetWeaponCatalog();
	EAFPS_API const FZMWeaponStats* FindWeapon(FName Id);

	/** Round formulas (modelled on the classic round-based zombies curve). */
	EAFPS_API int32 ZombiesForRound(int32 Round);
	EAFPS_API float ZombieHealthForRound(int32 Round);
	EAFPS_API float SpawnDelayForRound(int32 Round);

	EAFPS_API FText PerkName(EZMPerk Perk);
	EAFPS_API int32 PerkCost(EZMPerk Perk);
	EAFPS_API FLinearColor PerkColor(EZMPerk Perk);
	EAFPS_API FText PowerUpName(EZMPowerUp PowerUp);
	EAFPS_API FLinearColor PowerUpColor(EZMPowerUp PowerUp);

	/** Loads /Game/Audio/<Name>; returns nullptr (silently) if not imported yet. */
	EAFPS_API class USoundBase* LoadSound(FName Name);
	/** Shared 3D attenuation for world sounds (zombies, impacts, machines). */
	EAFPS_API class USoundAttenuation* WorldAttenuation();
	/** Plays a sound at a location with WorldAttenuation (no-op if the sound isn't imported). */
	EAFPS_API void PlayWorldSound(const UObject* WorldContext, FName Sound, const FVector& Location, float Volume = 1.f, float Pitch = 1.f);
	/** /Engine/BasicShapes/BasicShapeMaterial tinted with a dynamic instance. */
	EAFPS_API class UMaterialInstanceDynamic* MakeTintedMaterial(UObject* Outer, const FLinearColor& Color);
	EAFPS_API class UStaticMesh* CubeMesh();
	EAFPS_API class UStaticMesh* CylinderMesh();
	EAFPS_API class UStaticMesh* SphereMesh();

	constexpr int32 MaxAliveZombies = 24;
	constexpr int32 StartingPoints = 500;
	constexpr int32 PointsPerHit = 10;
	constexpr int32 PointsBodyKill = 60;
	constexpr int32 PointsHeadKill = 100;
	constexpr int32 PointsMeleeKill = 130;
	constexpr int32 PointsNuke = 400;
}
