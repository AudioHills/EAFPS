#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZMSpawnPoint.generated.h"

/** Where zombies climb out. Only spawn points whose zone has been opened are used. */
UCLASS()
class EAFPS_API AZMSpawnPoint : public AActor
{
	GENERATED_BODY()

public:
	AZMSpawnPoint();

	UPROPERTY(EditAnywhere, Category = "Zombies") int32 Zone = 0;
};
