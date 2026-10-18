#include "ZMGameMode.h"
#include "ZMCharacter.h"
#include "ZMZombie.h"
#include "ZMSpawnPoint.h"
#include "ZMPlayerController.h"
#include "ZMPlayerState.h"
#include "ZMHUD.h"
#include "ZMPowerUp.h"
#include "ZMArenaBuilder.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "ZM"

namespace
{
	constexpr float IntroTime = 3.f;
	constexpr float RoundBreakTime = 10.f;
	constexpr float PowerUpDuration = 30.f;
	constexpr int32 MaxDropsPerRound = 4;
}

AZMGameMode::AZMGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = AZMCharacter::StaticClass();
	PlayerControllerClass = AZMPlayerController::StaticClass();
	PlayerStateClass = AZMPlayerState::StaticClass();
	HUDClass = AZMHUD::StaticClass();
}

void AZMGameMode::StartPlay()
{
	UWorld* World = GetWorld();
	bool bHasSpawnPoints = false;
	for (TActorIterator<AZMSpawnPoint> It(World); It; ++It)
	{
		bHasSpawnPoints = true;
		break;
	}
	AZMArenaBuilder* Builder = nullptr;
	if (!bHasSpawnPoints)
	{
		Builder = World->SpawnActor<AZMArenaBuilder>(FVector::ZeroVector, FRotator::ZeroRotator);
	}

	Super::StartPlay();

	if (Builder)
	{
		// The level's own PlayerStart may sit anywhere; put the player in the built start room.
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* PC = It->Get();
			if (APawn* Pawn = PC ? PC->GetPawn() : nullptr)
			{
				Pawn->TeleportTo(AZMArenaBuilder::GetPlayerStart(), FRotator::ZeroRotator);
				PC->SetControlRotation(FRotator::ZeroRotator);
			}
		}
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (AZMPlayerState* PS = It->Get() ? It->Get()->GetPlayerState<AZMPlayerState>() : nullptr)
		{
			PS->ResetForMatch(ZM::StartingPoints);
		}
	}

	ActiveZones.Reset();
	ActiveZones.Add(0);
	Phase = EZMMatchPhase::Intro;
	PhaseTime = 0.f;
	Round = 0;
}

void AZMGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PhaseTime += DeltaSeconds;
	AnnouncementTime = FMath::Max(AnnouncementTime - DeltaSeconds, 0.f);
	InstaKillTime = FMath::Max(InstaKillTime - DeltaSeconds, 0.f);
	DoublePointsTime = FMath::Max(DoublePointsTime - DeltaSeconds, 0.f);

	AliveZombies.RemoveAll([](const TObjectPtr<AZMZombie>& Z) { return !IsValid(Z) || Z->IsDead(); });

	switch (Phase)
	{
	case EZMMatchPhase::Intro:
		if (PhaseTime >= IntroTime) StartRound(1);
		break;

	case EZMMatchPhase::InRound:
		SpawnTimer -= DeltaSeconds;
		if (ZombiesToSpawn > 0 && AliveZombies.Num() < ZM::MaxAliveZombies && SpawnTimer <= 0.f)
		{
			if (TrySpawnZombie())
			{
				--ZombiesToSpawn;
				SpawnTimer = ZM::SpawnDelayForRound(Round);
			}
			else
			{
				SpawnTimer = 0.4f;
			}
		}
		if (ZombiesToSpawn <= 0 && AliveZombies.Num() == 0)
		{
			EndRound();
		}
		break;

	case EZMMatchPhase::BetweenRounds:
		if (PhaseTime >= RoundBreakTime) StartRound(Round + 1);
		break;

	case EZMMatchPhase::GameOver:
		break;
	}
}

void AZMGameMode::StartRound(int32 NewRound)
{
	Round = NewRound;
	BestRound = FMath::Max(BestRound, Round);
	ZombiesToSpawn = ZM::ZombiesForRound(Round);
	Phase = EZMMatchPhase::InRound;
	PhaseTime = 0.f;
	SpawnTimer = 1.5f;
	DropsThisRound = 0;
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Round_Start"));
}

void AZMGameMode::EndRound()
{
	Phase = EZMMatchPhase::BetweenRounds;
	PhaseTime = 0.f;
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Round_End"));
}

AZMSpawnPoint* AZMGameMode::PickSpawnPoint(const FVector& PlayerLocation) const
{
	struct FCandidate { AZMSpawnPoint* Point; float Dist; };
	TArray<FCandidate> Candidates;
	for (TActorIterator<AZMSpawnPoint> It(GetWorld()); It; ++It)
	{
		if (!ActiveZones.Contains(It->Zone)) continue;
		const float Dist = FVector::Dist(It->GetActorLocation(), PlayerLocation);
		if (Dist < 450.f) continue; // never right on top of the player
		Candidates.Add({ *It, Dist });
	}
	if (Candidates.Num() == 0) return nullptr;
	// Prefer the closest few so pressure follows the player through the map.
	Candidates.Sort([](const FCandidate& A, const FCandidate& B) { return A.Dist < B.Dist; });
	const int32 Pool = FMath::Min(Candidates.Num(), 4);
	return Candidates[FMath::RandRange(0, Pool - 1)].Point;
}

bool AZMGameMode::TrySpawnZombie()
{
	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!Player) return false;
	AZMSpawnPoint* Point = PickSpawnPoint(Player->GetActorLocation());
	if (!Point) return false;

	const FVector Loc = Point->GetActorLocation() + FVector(FMath::FRandRange(-40.f, 40.f), FMath::FRandRange(-40.f, 40.f), 0.f);
	const FRotator Rot(0.f, (Player->GetActorLocation() - Loc).Rotation().Yaw, 0.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	AZMZombie* Zombie = GetWorld()->SpawnActor<AZMZombie>(AZMZombie::StaticClass(), Loc, Rot, Params);
	if (!Zombie) return false;
	Zombie->InitForRound(Round, ZM::ZombieHealthForRound(Round));
	AliveZombies.Add(Zombie);
	return true;
}

void AZMGameMode::OnZombieHit(AZMCharacter* By)
{
	if (AZMPlayerState* PS = By ? By->GetPlayerState<AZMPlayerState>() : nullptr)
	{
		PS->AddPoints(ZM::PointsPerHit);
	}
}

void AZMGameMode::OnZombieKilled(AZMZombie* Zombie, AZMCharacter* By, bool bHead, bool bMelee, bool bAwardPoints, bool bRespawn)
{
	AliveZombies.Remove(Zombie);
	if (bRespawn)
	{
		++ZombiesToSpawn;
		return;
	}
	if (bAwardPoints && By)
	{
		if (AZMPlayerState* PS = By->GetPlayerState<AZMPlayerState>())
		{
			PS->AddPoints(bMelee ? ZM::PointsMeleeKill : (bHead ? ZM::PointsHeadKill : ZM::PointsBodyKill));
			++PS->Kills;
			if (bHead) ++PS->Headshots;
		}
	}
	if (Zombie && bAwardPoints)
	{
		MaybeDropPowerUp(Zombie->GetActorLocation());
	}
}

void AZMGameMode::MaybeDropPowerUp(const FVector& Location)
{
	++KillsSinceDrop;
	if (DropsThisRound >= MaxDropsPerRound) return;
	// Low base chance that ramps up the longer it has been since the last drop.
	const float Chance = 0.02f + KillsSinceDrop * 0.0025f;
	if (FMath::FRand() >= Chance) return;

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	const FVector DropLoc(Location.X, Location.Y, Location.Z - 20.f);
	if (AZMPowerUp* Drop = GetWorld()->SpawnActor<AZMPowerUp>(AZMPowerUp::StaticClass(), DropLoc, FRotator::ZeroRotator, Params))
	{
		Drop->Init(static_cast<EZMPowerUp>(FMath::RandRange(0, 3)));
		++DropsThisRound;
		KillsSinceDrop = 0;
	}
}

void AZMGameMode::ApplyPowerUp(EZMPowerUp Type, AZMCharacter* By)
{
	Announce(ZM::PowerUpName(Type), ZM::PowerUpColor(Type), 2.5f);
	static const FName Sounds[] = { "SFX_PowerUp_MaxAmmo", "SFX_PowerUp_InstaKill", "SFX_PowerUp_DoublePoints", "SFX_PowerUp_Nuke" };
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound(Sounds[static_cast<int32>(Type)]));

	switch (Type)
	{
	case EZMPowerUp::MaxAmmo:
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (AZMCharacter* C = It->Get() ? Cast<AZMCharacter>(It->Get()->GetPawn()) : nullptr) C->RefillAllAmmo();
		}
		break;
	case EZMPowerUp::InstaKill:
		InstaKillTime = PowerUpDuration;
		break;
	case EZMPowerUp::DoublePoints:
		DoublePointsTime = PowerUpDuration;
		break;
	case EZMPowerUp::Nuke:
	{
		const TArray<TObjectPtr<AZMZombie>> Victims = AliveZombies;
		for (AZMZombie* Z : Victims)
		{
			if (IsValid(Z) && !Z->IsDead())
			{
				Z->Kill(By, false, false, FVector(0.f, 0.f, 1.f), false);
			}
		}
		if (AZMPlayerState* PS = By ? By->GetPlayerState<AZMPlayerState>() : nullptr)
		{
			PS->AddPoints(ZM::PointsNuke);
		}
		break;
	}
	}
}

void AZMGameMode::OnPlayerDied(AZMCharacter* Player)
{
	if (Phase == EZMMatchPhase::GameOver) return;
	Phase = EZMMatchPhase::GameOver;
	PhaseTime = 0.f;
	Announce(LOCTEXT("GameOver", "GAME OVER"), FLinearColor(0.75f, 0.03f, 0.02f), 1000.f);
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Game_Over"));
}

void AZMGameMode::ActivateZone(int32 Zone)
{
	ActiveZones.Add(Zone);
}

void AZMGameMode::Announce(const FText& Text, const FLinearColor& Color, float Duration)
{
	Announcement = Text;
	AnnouncementColor = Color;
	AnnouncementTime = Duration;
}

void AZMGameMode::RequestRestart()
{
	if (Phase != EZMMatchPhase::GameOver || PhaseTime < 2.f) return;
	UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}

#undef LOCTEXT_NAMESPACE
