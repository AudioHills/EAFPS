#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ZMPlayerState.generated.h"

UCLASS()
class EAFPS_API AZMPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	/** Adds points (double points applied here). Returns the amount actually awarded. */
	int32 AddPoints(int32 Amount);
	bool CanAfford(int32 Cost) const { return Points >= Cost; }
	bool SpendPoints(int32 Cost);

	int32 GetPoints() const { return Points; }
	void ResetForMatch(int32 StartPoints) { Points = StartPoints; Kills = 0; Headshots = 0; Popups.Reset(); }

	int32 Kills = 0;
	int32 Headshots = 0;

	/** Floating "+10" / "-1000" popups the HUD draws next to the score. */
	struct FPopup { int32 Amount; float Time; float Offset; };
	TArray<FPopup> Popups;

protected:
	int32 Points = 0;
};
