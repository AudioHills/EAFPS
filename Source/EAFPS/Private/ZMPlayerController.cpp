#include "ZMPlayerController.h"
#include "ZMCharacter.h"
#include "ZMWeapon.h"
#include "ZMZombie.h"
#include "ZMGameMode.h"
#include "ZMPlayerState.h"
#include "ZMPowerUp.h"
#include "ZMInteractable.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

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

#if !UE_BUILD_SHIPPING
	static int32 LevelRuns = 0;
	RunIndex = LevelRuns++;
	const TCHAR* Cmd = FCommandLine::Get();
	FString Shots;
	if (FParse::Value(Cmd, TEXT("ZMShots="), Shots, false))
	{
		TArray<FString> Parts;
		Shots.ParseIntoArray(Parts, TEXT(","));
		for (const FString& P : Parts) ShotTimes.Add(FCString::Atof(*P));
		ShotTimes.Sort();
	}
	FParse::Value(Cmd, TEXT("ZMQuitAt="), QuitAt);
	FParse::Value(Cmd, TEXT("ZMView="), ViewSpec, false);
	bBot = FParse::Param(Cmd, TEXT("ZMBot"));
	// Only once per process: the test ends by restarting the level, which creates a new controller.
	static bool bSelfTestStarted = false;
	if (FParse::Param(Cmd, TEXT("ZMSelfTest")) && !bSelfTestStarted)
	{
		bSelfTestStarted = true;
		BuildSelfTest();
	}
#endif
}

void AZMPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
	TickDevAutomation(DeltaSeconds);
#endif
}

#if !UE_BUILD_SHIPPING
void AZMPlayerController::TickDevAutomation(float DeltaSeconds)
{
	DevTime += DeltaSeconds;
	AZMCharacter* ZMPlayer = Cast<AZMCharacter>(GetPawn());

	// Views are '/'-separated; view N is applied shortly before screenshot N.
	if (ZMPlayer && !ViewSpec.IsEmpty() && ShotTimes.IsValidIndex(NextShot) && DevTime >= ShotTimes[NextShot] - 0.7f && AppliedView < NextShot)
	{
		AppliedView = NextShot;
		TArray<FString> Views;
		ViewSpec.ParseIntoArray(Views, TEXT("/"));
		TArray<FString> V;
		if (Views.IsValidIndex(NextShot)) Views[NextShot].ParseIntoArray(V, TEXT(","));
		if (V.Num() >= 5)
		{
			ZMPlayer->TeleportTo(FVector(FCString::Atof(*V[0]), FCString::Atof(*V[1]), FCString::Atof(*V[2])), FRotator::ZeroRotator);
			SetControlRotation(FRotator(FCString::Atof(*V[3]), FCString::Atof(*V[4]), 0.f));
		}
	}

	while (ShotTimes.IsValidIndex(NextShot) && DevTime >= ShotTimes[NextShot])
	{
		const FString File = FPaths::Combine(FPaths::ScreenShotDir(), RunIndex == 0 ? FString::Printf(TEXT("ZM_%03d.png"), int32(ShotTimes[NextShot])) : FString::Printf(TEXT("ZM_r%d_%03d.png"), RunIndex, int32(ShotTimes[NextShot])));
		FScreenshotRequest::RequestScreenshot(File, true, false);
		UE_LOG(LogTemp, Display, TEXT("EAFPS: screenshot %s"), *File);
		++NextShot;
	}

	if (bBot && ZMPlayer) TickBot(ZMPlayer, DeltaSeconds);
	if (ZMPlayer) TickSelfTest(ZMPlayer);

	if (QuitAt > 0.f && DevTime >= QuitAt)
	{
		QuitAt = -1.f;
		ConsoleCommand(TEXT("quit"));
	}
}

void AZMPlayerController::TickBot(AZMCharacter* ZMPlayer, float DeltaSeconds)
{
	const FVector Eye = ZMPlayer->GetPawnViewLocation();
	AZMZombie* Best = nullptr;
	float BestDist = TNumericLimits<float>::Max();
	for (TActorIterator<AZMZombie> It(GetWorld()); It; ++It)
	{
		if (It->IsDead()) continue;
		const float D = FVector::Dist(It->GetActorLocation(), Eye);
		if (D < BestDist && LineOfSightTo(*It))
		{
			Best = *It;
			BestDist = D;
		}
	}

	bool bWantFire = false;
	ZMPlayer->MoveInput = FVector2D::ZeroVector;
	if (Best)
	{
		// Aim at the upper chest, a little sloppy like a person would.
		const FVector Target = Best->GetActorLocation() + FVector(0.f, 0.f, 45.f);
		const FRotator Want = (Target - Eye).Rotation();
		const FRotator Now = GetControlRotation();
		SetControlRotation(FMath::RInterpTo(Now, Want, DeltaSeconds, 7.f));
		const float Err = FMath::Abs(FRotator::NormalizeAxis(Want.Yaw - Now.Yaw)) + FMath::Abs(FRotator::NormalizeAxis(Want.Pitch - Now.Pitch));
		bWantFire = Err < 4.f;
		if (BestDist < 380.f) ZMPlayer->MoveInput = FVector2D(0.f, -1.f);
	}

	AZMWeapon* W = ZMPlayer->GetCurrentWeapon();
	if (W && W->GetMag() == 0) bWantFire = false;
	if (bWantFire != bBotFiring)
	{
		bBotFiring = bWantFire;
		if (bWantFire) ZMPlayer->Input_FireStart(); else ZMPlayer->Input_FireStop();
	}
	else if (bWantFire && W && !W->GetStats().bAutomatic)
	{
		// Tap semi-autos.
		ZMPlayer->Input_FireStop();
		ZMPlayer->Input_FireStart();
	}
	if (W && W->GetMag() == 0 && !W->IsReloading()) ZMPlayer->Input_Reload();
}

void AZMPlayerController::TestLog(const FString& Msg) const
{
	UE_LOG(LogTemp, Display, TEXT("EAFPS TEST: %s"), *Msg);
}

void AZMPlayerController::TickSelfTest(AZMCharacter* ZMPlayer)
{
	while (TestSteps.IsValidIndex(NextTestStep) && DevTime >= TestSteps[NextTestStep].Time)
	{
		TestSteps[NextTestStep++].Run(ZMPlayer);
	}
}

void AZMPlayerController::BuildSelfTest()
{
	// Locations match the default arena built by AZMArenaBuilder.
	auto Points = [](AZMCharacter* P) { const AZMPlayerState* PS = P->GetPlayerState<AZMPlayerState>(); return PS ? PS->GetPoints() : -1; };
	auto Goto = [this](AZMCharacter* P, const FVector& Loc, float Yaw)
	{
		P->TeleportTo(Loc, FRotator::ZeroRotator);
		SetControlRotation(FRotator(-5.f, Yaw, 0.f));
	};
	auto Use = [this, Points](AZMCharacter* P, const TCHAR* What)
	{
		AZMInteractable* I = P->GetFocusedInteractable();
		const FString Prompt = I ? I->GetPrompt(P).ToString() : FString(TEXT("<nothing focused>"));
		const int32 Before = Points(P);
		P->Input_Interact();
		TestLog(FString::Printf(TEXT("%s: prompt='%s' points %d -> %d"), What, *Prompt, Before, Points(P)));
	};
	auto Weapons = [](AZMCharacter* P)
	{
		FString S;
		for (AZMWeapon* W : P->GetWeapons()) if (W) S += W->GetStats().Id.ToString() + TEXT(" ");
		return S;
	};
	auto Drop = [](AZMCharacter* P, EZMPowerUp Type)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (AZMPowerUp* D = P->GetWorld()->SpawnActor<AZMPowerUp>(AZMPowerUp::StaticClass(), P->GetActorLocation() - FVector(0, 0, 40), FRotator::ZeroRotator, Params)) D->Init(Type);
	};

	TestSteps = {
		{ 2.0f, [=, this](AZMCharacter* P) { TestLog(FString::Printf(TEXT("start points=%d weapons=%s"), Points(P), *Weapons(P))); } },
		{ 3.0f, [=, this](AZMCharacter* P) { Goto(P, FVector(850, 0, 100), 0.f); } },
		{ 3.5f, [=, this](AZMCharacter* P) { Use(P, TEXT("door north")); } },
		{ 5.0f, [=, this](AZMCharacter* P) { Goto(P, FVector(2650, -400, 100), 0.f); } },
		{ 5.5f, [=, this](AZMCharacter* P) { Use(P, TEXT("wallbuy RANGER")); TestLog(TEXT("weapons=") + Weapons(P)); } },
		{ 6.0f, [=, this](AZMCharacter* P) { Use(P, TEXT("wallbuy RANGER ammo (full, expect no-op)")); } },
		{ 7.0f, [=, this](AZMCharacter* P) { Goto(P, FVector(2000, -720, 100), -90.f); } },
		{ 7.5f, [=, this](AZMCharacter* P) { Use(P, TEXT("perk IronGut")); TestLog(FString::Printf(TEXT("max health=%.0f"), P->GetMaxHealth())); } },
		{ 8.5f, [=, this](AZMCharacter* P) { Goto(P, FVector(2600, 350, 100), 0.f); } },
		{ 9.0f, [=, this](AZMCharacter* P) { Use(P, TEXT("mystery box buy")); } },
		{ 14.0f, [=, this](AZMCharacter* P) { Use(P, TEXT("mystery box take")); TestLog(TEXT("weapons=") + Weapons(P)); } },
		{ 15.0f, [=, this](AZMCharacter* P) { Goto(P, FVector(150, 850, 100), 90.f); } },
		{ 15.5f, [=, this](AZMCharacter* P) { Use(P, TEXT("door east")); } },
		{ 17.0f, [=, this](AZMCharacter* P) { Drop(P, EZMPowerUp::DoublePoints); } },
		{ 17.5f, [=, this](AZMCharacter* P) { Drop(P, EZMPowerUp::InstaKill); } },
		{ 18.0f, [=, this](AZMCharacter* P) { Drop(P, EZMPowerUp::MaxAmmo); } },
		{ 18.5f, [=, this](AZMCharacter* P) { Drop(P, EZMPowerUp::Nuke); } },
		{ 19.0f, [=, this](AZMCharacter* P)
			{
				const AZMGameMode* GM = GetWorld()->GetAuthGameMode<AZMGameMode>();
				TestLog(FString::Printf(TEXT("powerups: insta=%d double=%d points=%d round=%d left=%d"), GM->IsInstaKill(), GM->IsDoublePoints(), Points(P), GM->GetRound(), GM->GetZombiesRemaining()));
			} },
		{ 21.0f, [=, this](AZMCharacter* P) { P->ReceiveZombieHit(100.f, P->GetActorLocation() + FVector(100, 0, 0)); P->ReceiveZombieHit(100.f, P->GetActorLocation() + FVector(100, 0, 0)); P->ReceiveZombieHit(100.f, P->GetActorLocation() + FVector(100, 0, 0)); } },
		{ 21.5f, [=, this](AZMCharacter* P) { TestLog(FString::Printf(TEXT("after hits: downed=%d phase=%d"), P->IsDowned(), int32(GetWorld()->GetAuthGameMode<AZMGameMode>()->GetPhase()))); } },
		{ 25.0f, [=, this](AZMCharacter* P) { TestLog(TEXT("requesting restart")); P->Input_Interact(); } },
	};
}
#endif

