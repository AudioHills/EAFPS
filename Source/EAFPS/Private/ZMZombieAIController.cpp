#include "ZMZombieAIController.h"
#include "ZMZombie.h"
#include "ZMCharacter.h"
#include "Navigation/CrowdFollowingComponent.h"
#include "Kismet/GameplayStatics.h"

AZMZombieAIController::AZMZombieAIController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCrowdFollowingComponent>(TEXT("PathFollowingComponent")))
{
	PrimaryActorTick.bCanEverTick = true;
	bWantsPlayerState = false;
	// Stagger the first path request so a wave doesn't path on the same frame.
	RepathTimer = FMath::FRandRange(0.f, 0.3f);
}

void AZMZombieAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AZMZombie* Zombie = Cast<AZMZombie>(GetPawn());
	if (!Zombie || Zombie->IsDead()) return;

	RepathTimer -= DeltaSeconds;
	if (RepathTimer > 0.f) return;

	APawn* Target = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Target)
	{
		RepathTimer = 0.5f;
		return;
	}

	const float Dist = FVector::Dist(Target->GetActorLocation(), Zombie->GetActorLocation());
	RepathTimer = Dist < 800.f ? 0.25f : 0.6f;

	// MoveToActor tracks the goal on its own; only (re)issue when idle or the goal changed.
	const UPathFollowingComponent* PFC = GetPathFollowingComponent();
	if (GetMoveStatus() == EPathFollowingStatus::Idle || (PFC && PFC->GetMoveGoal() != Target))
	{
		MoveToActor(Target, 40.f, true, true, false, nullptr, true);
	}
}
