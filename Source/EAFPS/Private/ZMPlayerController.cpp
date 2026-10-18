#include "ZMPlayerController.h"
#include "ZMCharacter.h"
#include "ZMWeapon.h"
#include "ZMZombie.h"
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
	const TCHAR* Cmd = FCommandLine::Get();
	FString Shots;
	if (FParse::Value(Cmd, TEXT("ZMShots="), Shots))
	{
		TArray<FString> Parts;
		Shots.ParseIntoArray(Parts, TEXT(","));
		for (const FString& P : Parts) ShotTimes.Add(FCString::Atof(*P));
		ShotTimes.Sort();
	}
	FParse::Value(Cmd, TEXT("ZMQuitAt="), QuitAt);
	FParse::Value(Cmd, TEXT("ZMView="), ViewSpec);
	bBot = FParse::Param(Cmd, TEXT("ZMBot"));
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

	if (!bViewApplied && ZMPlayer && !ViewSpec.IsEmpty() && DevTime > 0.5f)
	{
		bViewApplied = true;
		TArray<FString> V;
		ViewSpec.ParseIntoArray(V, TEXT(","));
		if (V.Num() >= 5)
		{
			ZMPlayer->TeleportTo(FVector(FCString::Atof(*V[0]), FCString::Atof(*V[1]), FCString::Atof(*V[2])), FRotator::ZeroRotator);
			SetControlRotation(FRotator(FCString::Atof(*V[3]), FCString::Atof(*V[4]), 0.f));
		}
	}

	while (ShotTimes.IsValidIndex(NextShot) && DevTime >= ShotTimes[NextShot])
	{
		const FString File = FPaths::Combine(FPaths::ScreenShotDir(), FString::Printf(TEXT("ZM_%03d.png"), int32(ShotTimes[NextShot])));
		FScreenshotRequest::RequestScreenshot(File, true, false);
		UE_LOG(LogTemp, Display, TEXT("EAFPS: screenshot %s"), *File);
		++NextShot;
	}

	if (bBot && ZMPlayer) TickBot(ZMPlayer, DeltaSeconds);

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
#endif
