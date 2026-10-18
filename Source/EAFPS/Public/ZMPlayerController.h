#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ZMPlayerController.generated.h"

class AZMCharacter;

UCLASS()
class EAFPS_API AZMPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AZMPlayerController();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

#if !UE_BUILD_SHIPPING
protected:
	/**
	 * Playtest automation from the command line (non-shipping only):
	 *   -ZMShots=8,20,45   screenshots (with HUD) at those times, to Saved/Screenshots
	 *   -ZMQuitAt=60       quit after N seconds
	 *   -ZMBot             aim at the nearest zombie, fire, reload, back away
	 *   -ZMView=x,y,z,pitch,yaw   place the player for a static shot
	 */
	void TickDevAutomation(float DeltaSeconds);
	void TickBot(AZMCharacter* ZMPlayer, float DeltaSeconds);

	TArray<float> ShotTimes;
	int32 NextShot = 0;
	float QuitAt = -1.f;
	float DevTime = 0.f;
	bool bBot = false;
	bool bViewApplied = false;
	FString ViewSpec;
	bool bBotFiring = false;
#endif
};
