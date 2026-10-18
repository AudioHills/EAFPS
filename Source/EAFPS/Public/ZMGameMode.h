#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZMTypes.h"
#include "ZMGameMode.generated.h"

class AZMZombie;
class AZMSpawnPoint;
class AZMCharacter;

UENUM()
enum class EZMMatchPhase : uint8 { Intro, InRound, BetweenRounds, GameOver };

UCLASS()
class EAFPS_API AZMGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZMGameMode();

	virtual void StartPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// --- Events ---
	void OnZombieHit(AZMCharacter* By);
	void OnZombieKilled(AZMZombie* Zombie, AZMCharacter* By, bool bHead, bool bMelee, bool bAwardPoints, bool bRespawn);
	void OnPlayerDied(AZMCharacter* Player);
	void ActivateZone(int32 Zone);
	void ApplyPowerUp(EZMPowerUp Type, AZMCharacter* By);
	void RequestRestart();

	// --- Queries ---
	int32 GetRound() const { return Round; }
	EZMMatchPhase GetPhase() const { return Phase; }
	float GetPhaseTime() const { return PhaseTime; }
	int32 GetZombiesRemaining() const { return ZombiesToSpawn + AliveZombies.Num(); }
	bool IsInstaKill() const { return InstaKillTime > 0.f; }
	bool IsDoublePoints() const { return DoublePointsTime > 0.f; }
	float GetInstaKillTime() const { return InstaKillTime; }
	float GetDoublePointsTime() const { return DoublePointsTime; }
	const FText& GetAnnouncement() const { return Announcement; }
	float GetAnnouncementTime() const { return AnnouncementTime; }
	FLinearColor GetAnnouncementColor() const { return AnnouncementColor; }
	int32 GetHighestRoundThisSession() const { return BestRound; }

protected:
	void StartRound(int32 NewRound);
	void EndRound();
	bool TrySpawnZombie();
	AZMSpawnPoint* PickSpawnPoint(const FVector& PlayerLocation) const;
	void Announce(const FText& Text, const FLinearColor& Color, float Duration);
	void MaybeDropPowerUp(const FVector& Location);

	UPROPERTY() TArray<TObjectPtr<AZMZombie>> AliveZombies;
	TSet<int32> ActiveZones;

	EZMMatchPhase Phase = EZMMatchPhase::Intro;
	float PhaseTime = 0.f;
	int32 Round = 0;
	int32 BestRound = 0;
	int32 ZombiesToSpawn = 0;
	float SpawnTimer = 0.f;
	float InstaKillTime = 0.f;
	float DoublePointsTime = 0.f;
	int32 DropsThisRound = 0;
	int32 KillsSinceDrop = 0;
	FText Announcement;
	float AnnouncementTime = 0.f;
	FLinearColor AnnouncementColor = FLinearColor::White;
};
