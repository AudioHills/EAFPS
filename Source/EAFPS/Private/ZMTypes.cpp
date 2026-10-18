#include "ZMTypes.h"
#include "Sound/SoundBase.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundAttenuation.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "ZM"

namespace
{
	FZMWeaponStats MakeWeapon(FName Id, const FText& Name, EZMWeaponClass Class)
	{
		FZMWeaponStats W;
		W.Id = Id;
		W.DisplayName = Name;
		W.Class = Class;
		return W;
	}

	TArray<FZMWeaponStats> BuildCatalog()
	{
		TArray<FZMWeaponStats> Out;

		{
			FZMWeaponStats W = MakeWeapon("MK9", LOCTEXT("MK9", "MK9 Pistol"), EZMWeaponClass::Pistol);
			W.Damage = 40; W.HeadshotMultiplier = 2.5f; W.RoundsPerMinute = 420; W.bAutomatic = false;
			W.MagSize = 8; W.MaxReserve = 80; W.ReloadTime = 1.45f; W.EmptyReloadTime = 1.85f;
			W.HipSpread = 2.0f; W.AdsSpread = 0.1f; W.SpreadPerShot = 0.6f;
			W.RecoilPitch = 1.4f; W.RecoilYawRange = 0.35f; W.RecoilYawBias = 0.0f; W.VisualKick = 1.6f;
			W.AdsTime = 0.16f; W.AdsZoom = 1.15f; W.SprintOutTime = 0.12f; W.MoveSpeedScale = 1.0f;
			W.bInMysteryBox = false; W.FireSound = "SFX_Pistol_Fire";
			W.HipOffset = FVector(26.f, 11.f, -13.f);
			W.BodySize = FVector(18.f, 3.2f, 5.f);
			Out.Add(W);
		}
		{
			FZMWeaponStats W = MakeWeapon("VANDAL", LOCTEXT("Vandal", "Vandal SMG"), EZMWeaponClass::SMG);
			W.Damage = 32; W.HeadshotMultiplier = 2.0f; W.RoundsPerMinute = 820; W.bAutomatic = true;
			W.MagSize = 32; W.MaxReserve = 192; W.ReloadTime = 2.0f; W.EmptyReloadTime = 2.5f;
			W.HipSpread = 2.4f; W.AdsSpread = 0.35f; W.SpreadPerShot = 0.15f; W.MoveSpreadAdd = 1.0f;
			W.RecoilPitch = 0.42f; W.RecoilYawRange = 0.3f; W.RecoilYawBias = 0.04f; W.VisualKick = 0.7f;
			W.AdsTime = 0.18f; W.AdsZoom = 1.2f; W.SprintOutTime = 0.14f; W.MoveSpeedScale = 1.0f;
			W.WallCost = 1000; W.FireSound = "SFX_SMG_Fire";
			W.BodySize = FVector(32.f, 4.f, 7.f);
			Out.Add(W);
		}
		{
			FZMWeaponStats W = MakeWeapon("RANGER", LOCTEXT("Ranger", "Ranger AR"), EZMWeaponClass::Rifle);
			W.Damage = 55; W.HeadshotMultiplier = 2.0f; W.RoundsPerMinute = 680; W.bAutomatic = true;
			W.MagSize = 30; W.MaxReserve = 210; W.ReloadTime = 2.2f; W.EmptyReloadTime = 2.8f;
			W.HipSpread = 3.2f; W.AdsSpread = 0.12f; W.SpreadPerShot = 0.2f;
			W.RecoilPitch = 0.55f; W.RecoilYawRange = 0.22f; W.RecoilYawBias = 0.06f; W.VisualKick = 0.9f;
			W.AdsTime = 0.24f; W.AdsZoom = 1.35f; W.SprintOutTime = 0.2f; W.MoveSpeedScale = 0.95f;
			W.WallCost = 1400; W.FireSound = "SFX_Rifle_Fire";
			W.BodySize = FVector(46.f, 4.f, 7.5f);
			Out.Add(W);
		}
		{
			FZMWeaponStats W = MakeWeapon("BREACHER", LOCTEXT("Breacher", "Breacher Shotgun"), EZMWeaponClass::Shotgun);
			W.Damage = 48; W.HeadshotMultiplier = 1.5f; W.Pellets = 8; W.RoundsPerMinute = 75; W.bAutomatic = false;
			W.MagSize = 6; W.MaxReserve = 54; W.ReloadTime = 2.6f; W.EmptyReloadTime = 3.2f;
			W.FalloffStart = 500.f; W.FalloffEnd = 1500.f; W.MinDamageScale = 0.2f;
			W.HipSpread = 5.5f; W.AdsSpread = 4.0f; W.SpreadPerShot = 0.f; W.MoveSpreadAdd = 0.5f;
			W.RecoilPitch = 3.2f; W.RecoilYawRange = 0.8f; W.RecoilYawBias = 0.f; W.VisualKick = 3.0f;
			W.AdsTime = 0.26f; W.AdsZoom = 1.15f; W.SprintOutTime = 0.22f; W.MoveSpeedScale = 0.95f;
			W.WallCost = 1500; W.FireSound = "SFX_Shotgun_Fire";
			W.BodySize = FVector(48.f, 4.5f, 6.5f);
			Out.Add(W);
		}
		{
			FZMWeaponStats W = MakeWeapon("BULLDOG", LOCTEXT("Bulldog", "Bulldog LMG"), EZMWeaponClass::LMG);
			W.Damage = 65; W.HeadshotMultiplier = 2.0f; W.RoundsPerMinute = 720; W.bAutomatic = true;
			W.MagSize = 100; W.MaxReserve = 400; W.ReloadTime = 4.6f; W.EmptyReloadTime = 5.4f;
			W.HipSpread = 4.5f; W.AdsSpread = 0.3f; W.SpreadPerShot = 0.12f;
			W.RecoilPitch = 0.5f; W.RecoilYawRange = 0.35f; W.RecoilYawBias = 0.08f; W.VisualKick = 1.0f;
			W.AdsTime = 0.38f; W.AdsZoom = 1.35f; W.SprintOutTime = 0.32f; W.MoveSpeedScale = 0.85f;
			W.FireSound = "SFX_LMG_Fire";
			W.BodySize = FVector(56.f, 6.f, 10.f);
			Out.Add(W);
		}
		{
			FZMWeaponStats W = MakeWeapon("LONGBOW", LOCTEXT("Longbow", "Longbow Sniper"), EZMWeaponClass::Sniper);
			W.Damage = 500; W.HeadshotMultiplier = 4.0f; W.RoundsPerMinute = 55; W.bAutomatic = false;
			W.MagSize = 5; W.MaxReserve = 40; W.ReloadTime = 3.0f; W.EmptyReloadTime = 3.6f;
			W.FalloffStart = 10000.f; W.FalloffEnd = 20000.f; W.MinDamageScale = 1.f;
			W.HipSpread = 7.0f; W.AdsSpread = 0.0f; W.SpreadPerShot = 0.f;
			W.RecoilPitch = 4.0f; W.RecoilYawRange = 0.6f; W.VisualKick = 3.5f;
			W.AdsTime = 0.42f; W.AdsZoom = 4.0f; W.SprintOutTime = 0.35f; W.MoveSpeedScale = 0.9f;
			W.FireSound = "SFX_Sniper_Fire";
			W.BodySize = FVector(64.f, 4.5f, 8.f);
			Out.Add(W);
		}
		{
			FZMWeaponStats W = MakeWeapon("HAMMER", LOCTEXT("Hammer", "Hammer Revolver"), EZMWeaponClass::Pistol);
			W.Damage = 160; W.HeadshotMultiplier = 2.5f; W.RoundsPerMinute = 170; W.bAutomatic = false;
			W.MagSize = 6; W.MaxReserve = 48; W.ReloadTime = 2.4f; W.EmptyReloadTime = 2.4f;
			W.HipSpread = 2.2f; W.AdsSpread = 0.05f; W.SpreadPerShot = 0.f;
			W.RecoilPitch = 3.4f; W.RecoilYawRange = 0.5f; W.VisualKick = 2.8f;
			W.AdsTime = 0.2f; W.AdsZoom = 1.2f; W.SprintOutTime = 0.14f;
			W.FireSound = "SFX_Revolver_Fire";
			W.HipOffset = FVector(26.f, 11.f, -13.f);
			W.BodySize = FVector(22.f, 3.6f, 6.f);
			W.Tint = FLinearColor(0.25f, 0.25f, 0.27f);
			Out.Add(W);
		}
		return Out;
	}
}

const TArray<FZMWeaponStats>& ZM::GetWeaponCatalog()
{
	static const TArray<FZMWeaponStats> Catalog = BuildCatalog();
	return Catalog;
}

const FZMWeaponStats* ZM::FindWeapon(FName Id)
{
	return GetWeaponCatalog().FindByPredicate([Id](const FZMWeaponStats& W) { return W.Id == Id; });
}

int32 ZM::ZombiesForRound(int32 Round)
{
	static const int32 Early[] = { 6, 8, 13, 18, 24, 27, 28, 28, 29 };
	if (Round <= 0) return 0;
	if (Round <= 9) return Early[Round - 1];
	return FMath::RoundToInt(0.09f * Round * Round - 0.0029f * Round + 23.958f);
}

float ZM::ZombieHealthForRound(int32 Round)
{
	if (Round < 10)
	{
		return 150.f + 100.f * (FMath::Max(Round, 1) - 1);
	}
	return 950.f * FMath::Pow(1.1f, float(Round - 9));
}

float ZM::SpawnDelayForRound(int32 Round)
{
	return FMath::Max(2.0f * FMath::Pow(0.95f, float(FMath::Max(Round, 1) - 1)), 0.1f);
}

FText ZM::PerkName(EZMPerk Perk)
{
	switch (Perk)
	{
	case EZMPerk::IronGut:    return LOCTEXT("IronGut", "Iron Gut");
	case EZMPerk::QuickHands: return LOCTEXT("QuickHands", "Quick Hands");
	case EZMPerk::DoubleTap:  return LOCTEXT("DoubleTap", "Double Tap");
	case EZMPerk::SecondWind: return LOCTEXT("SecondWind", "Second Wind");
	}
	return FText::GetEmpty();
}

int32 ZM::PerkCost(EZMPerk Perk)
{
	switch (Perk)
	{
	case EZMPerk::IronGut:    return 2500;
	case EZMPerk::QuickHands: return 3000;
	case EZMPerk::DoubleTap:  return 2000;
	case EZMPerk::SecondWind: return 500;
	}
	return 0;
}

FLinearColor ZM::PerkColor(EZMPerk Perk)
{
	switch (Perk)
	{
	case EZMPerk::IronGut:    return FLinearColor(0.9f, 0.08f, 0.05f);
	case EZMPerk::QuickHands: return FLinearColor(0.15f, 0.9f, 0.2f);
	case EZMPerk::DoubleTap:  return FLinearColor(0.95f, 0.65f, 0.05f);
	case EZMPerk::SecondWind: return FLinearColor(0.1f, 0.45f, 1.0f);
	}
	return FLinearColor::White;
}

FText ZM::PowerUpName(EZMPowerUp PowerUp)
{
	switch (PowerUp)
	{
	case EZMPowerUp::MaxAmmo:      return LOCTEXT("MaxAmmo", "MAX AMMO!");
	case EZMPowerUp::InstaKill:    return LOCTEXT("InstaKill", "INSTA-KILL!");
	case EZMPowerUp::DoublePoints: return LOCTEXT("DoublePoints", "DOUBLE POINTS!");
	case EZMPowerUp::Nuke:         return LOCTEXT("Nuke", "KABOOM!");
	}
	return FText::GetEmpty();
}

FLinearColor ZM::PowerUpColor(EZMPowerUp PowerUp)
{
	switch (PowerUp)
	{
	case EZMPowerUp::MaxAmmo:      return FLinearColor(0.2f, 1.f, 0.3f);
	case EZMPowerUp::InstaKill:    return FLinearColor(1.f, 0.2f, 0.15f);
	case EZMPowerUp::DoublePoints: return FLinearColor(1.f, 0.85f, 0.1f);
	case EZMPowerUp::Nuke:         return FLinearColor(1.f, 0.5f, 0.1f);
	}
	return FLinearColor::White;
}

USoundBase* ZM::LoadSound(FName Name)
{
	// Sounds are rooted once loaded so the raw-pointer cache can never dangle.
	static TMap<FName, USoundBase*> Cache;
	if (Name.IsNone()) return nullptr;
	if (USoundBase** Found = Cache.Find(Name)) return *Found;

	const FString Path = FString::Printf(TEXT("/Game/Audio/%s.%s"), *Name.ToString(), *Name.ToString());
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (Sound) Sound->AddToRoot();
	Cache.Add(Name, Sound);
	return Sound;
}

USoundAttenuation* ZM::WorldAttenuation()
{
	static USoundAttenuation* Atten = nullptr;
	if (!Atten)
	{
		Atten = NewObject<USoundAttenuation>(GetTransientPackage(), TEXT("ZM_WorldAttenuation"));
		Atten->AddToRoot();
		Atten->Attenuation.bAttenuate = true;
		Atten->Attenuation.bSpatialize = true;
		Atten->Attenuation.AttenuationShape = EAttenuationShape::Sphere;
		Atten->Attenuation.AttenuationShapeExtents = FVector(250.f, 0.f, 0.f);
		Atten->Attenuation.FalloffDistance = 3000.f;
		Atten->Attenuation.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
	}
	return Atten;
}

void ZM::PlayWorldSound(const UObject* WorldContext, FName Sound, const FVector& Location, float Volume, float Pitch)
{
	if (USoundBase* S = LoadSound(Sound))
	{
		UGameplayStatics::PlaySoundAtLocation(WorldContext, S, Location, Volume, Pitch, 0.f, WorldAttenuation());
	}
}

UMaterialInstanceDynamic* ZM::MakeTintedMaterial(UObject* Outer, const FLinearColor& Color)
{
	static UMaterialInterface* Base = nullptr;
	if (!Base)
	{
		Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
		if (Base) Base->AddToRoot();
	}
	if (!Base) return nullptr;
	UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Base, Outer);
	MID->SetVectorParameterValue(TEXT("Color"), Color);
	return MID;
}

static UStaticMesh* LoadEngineMesh(const TCHAR* Path, UStaticMesh*& Slot)
{
	if (!Slot)
	{
		Slot = LoadObject<UStaticMesh>(nullptr, Path);
		if (Slot) Slot->AddToRoot();
	}
	return Slot;
}

UStaticMesh* ZM::CubeMesh()     { static UStaticMesh* M = nullptr; return LoadEngineMesh(TEXT("/Engine/BasicShapes/Cube.Cube"), M); }
UStaticMesh* ZM::CylinderMesh() { static UStaticMesh* M = nullptr; return LoadEngineMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"), M); }
UStaticMesh* ZM::SphereMesh()   { static UStaticMesh* M = nullptr; return LoadEngineMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"), M); }

#undef LOCTEXT_NAMESPACE
