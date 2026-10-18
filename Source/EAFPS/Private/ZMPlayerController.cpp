#include "ZMPlayerController.h"

AZMPlayerController::AZMPlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	bShowMouseCursor = false;
}

void AZMPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (IsLocalController())
	{
		SetInputMode(FInputModeGameOnly());
	}
}

void AZMPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}
