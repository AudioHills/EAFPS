#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZMTypes.h"
#include "ZMPowerUp.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UTextRenderComponent;

/** Glowing drop left by a zombie. Collected by walking through it; blinks out after 30s. */
UCLASS(NotPlaceable)
class EAFPS_API AZMPowerUp : public AActor
{
	GENERATED_BODY()

public:
	AZMPowerUp();
	void Init(EZMPowerUp InType);
	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Glow;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
	EZMPowerUp Type = EZMPowerUp::MaxAmmo;
	float Age = 0.f;
	FVector BaseLocation;
};
