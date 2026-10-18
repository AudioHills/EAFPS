#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZMTransientFX.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;

/** Asset-free, short-lived visual effect: impact puffs, blood bursts and tracers. */
UCLASS(NotPlaceable)
class EAFPS_API AZMTransientFX : public AActor
{
	GENERATED_BODY()

public:
	AZMTransientFX();

	static void SpawnImpact(UWorld* World, const FVector& Location, const FVector& Normal, bool bFlesh);
	static void SpawnTracer(UWorld* World, const FVector& Start, const FVector& End);

	virtual void Tick(float DeltaSeconds) override;

protected:
	UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
	UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;

	float Life = 0.1f;
	float Age = 0.f;
	FVector StartScale = FVector::OneVector;
	FVector EndScale = FVector::ZeroVector;
	FVector Velocity = FVector::ZeroVector;
	float StartLight = 0.f;
};
