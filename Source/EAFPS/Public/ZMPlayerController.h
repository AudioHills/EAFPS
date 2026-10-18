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
	 *   -ZMView=x,y,z,pitch,yaw/...   place the player before each screenshot
	 *   -ZMSelfTest        scripted run through doors, wall buy, perk, box, power-ups, game over, restart
	 */
	void TickDevAutomation(float DeltaSeconds);
	void TickBot(AZMCharacter* ZMPlayer, float DeltaSeconds);
	void BuildSelfTest();
	void TickSelfTest(AZMCharacter* ZMPlayer);
	void TestLog(const FString& Msg) const;

	struct FTestStep { float Time; TFunction<void(AZMCharacter*)> Run; };
	TArray<FTestStep> TestSteps;
	int32 NextTestStep = 0;

	TArray<float> ShotTimes;
	int32 NextShot = 0;
	int32 RunIndex = 0;
	float QuitAt = -1.f;
	float DevTime = 0.f;
	bool bBot = false;
	int32 AppliedView = -1;
	FString ViewSpec;
	bool bBotFiring = false;
#endif
};
