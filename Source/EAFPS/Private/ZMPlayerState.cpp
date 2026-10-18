#include "ZMPlayerState.h"
#include "ZMGameMode.h"
#include "Engine/World.h"

int32 AZMPlayerState::AddPoints(int32 Amount)
{
	if (Amount <= 0) return 0;
	if (const AZMGameMode* GM = GetWorld()->GetAuthGameMode<AZMGameMode>())
	{
		if (GM->IsDoublePoints()) Amount *= 2;
	}
	Points += Amount;
	SetScore(float(Points));
	Popups.Add({ Amount, float(GetWorld()->GetTimeSeconds()), FMath::FRandRange(-1.f, 1.f) });
	return Amount;
}

bool AZMPlayerState::SpendPoints(int32 Cost)
{
	if (!CanAfford(Cost)) return false;
	Points -= Cost;
	SetScore(float(Points));
	Popups.Add({ -Cost, float(GetWorld()->GetTimeSeconds()), 0.f });
	return true;
}
