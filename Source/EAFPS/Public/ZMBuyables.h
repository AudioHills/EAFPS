#pragma once

#include "CoreMinimal.h"
#include "ZMInteractable.h"
#include "ZMTypes.h"
#include "ZMBuyables.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UTextRenderComponent;

/** Chalk outline on the wall: buy a weapon, or its ammo at half price if you own it. */
UCLASS()
class EAFPS_API AZMWallBuy : public AZMInteractable
{
	GENERATED_BODY()

public:
	AZMWallBuy();
	virtual void BeginPlay() override;
	virtual FText GetPrompt(const AZMCharacter* User) const override;
	virtual void Interact(AZMCharacter* User) override;

	UPROPERTY(EditAnywhere, Category = "Zombies") FName WeaponId = "VANDAL";

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Board;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> ModelRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
};

/** Debris / door blocking a zone. Opening it activates that zone's spawners. */
UCLASS()
class EAFPS_API AZMDoor : public AZMInteractable
{
	GENERATED_BODY()

public:
	AZMDoor();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FText GetPrompt(const AZMCharacter* User) const override;
	virtual void Interact(AZMCharacter* User) override;

	UPROPERTY(EditAnywhere, Category = "Zombies") int32 Cost = 750;
	UPROPERTY(EditAnywhere, Category = "Zombies") int32 UnlocksZone = 1;
	UPROPERTY(EditAnywhere, Category = "Zombies") FVector Size = FVector(30.f, 300.f, 300.f);

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Panel;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	bool bOpening = false;
	float OpenTime = 0.f;
};

/** Perk vending machine. */
UCLASS()
class EAFPS_API AZMPerkMachine : public AZMInteractable
{
	GENERATED_BODY()

public:
	AZMPerkMachine();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FText GetPrompt(const AZMCharacter* User) const override;
	virtual void Interact(AZMCharacter* User) override;

	UPROPERTY(EditAnywhere, Category = "Zombies") EZMPerk Perk = EZMPerk::IronGut;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Sign;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	float FlickerTime = 0.f;
};

/** 950 points for a random weapon. Cycles models, then offers one for a few seconds. */
UCLASS()
class EAFPS_API AZMMysteryBox : public AZMInteractable
{
	GENERATED_BODY()

public:
	AZMMysteryBox();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual FText GetPrompt(const AZMCharacter* User) const override;
	virtual void Interact(AZMCharacter* User) override;

	UPROPERTY(EditAnywhere, Category = "Zombies") int32 Cost = 950;

protected:
	enum class EState : uint8 { Idle, Rolling, Offering };
	void ShowWeapon(FName Id);

	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Crate;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Lid;
	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> WeaponRoot;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Beam;
	UPROPERTY() TObjectPtr<USceneComponent> ShownModel;

	EState State = EState::Idle;
	float StateTime = 0.f;
	float NextCycle = 0.f;
	FName OfferedWeapon;
	FName ShownWeapon;
	TWeakObjectPtr<AZMCharacter> Buyer;
};
