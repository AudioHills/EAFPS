#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ZMPlayerController.generated.h"

UCLASS()
class EAFPS_API AZMPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AZMPlayerController();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
};
