#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ZMZombieAIController.generated.h"

/** Chases the player along the navmesh with crowd avoidance. */
UCLASS()
class EAFPS_API AZMZombieAIController : public AAIController
{
	GENERATED_BODY()

public:
	AZMZombieAIController(const FObjectInitializer& ObjectInitializer);
	virtual void Tick(float DeltaSeconds) override;

protected:
	float RepathTimer = 0.f;
};
