#include "ZMCharacter.h"
#include "ZMWeapon.h"
#include "ZMZombie.h"
#include "ZMInteractable.h"
#include "ZMGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AZMCharacter::AZMCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 92.f);

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->bOrientRotationToMovement = false;
	Move->MaxWalkSpeed = JogSpeed;
	Move->MaxWalkSpeedCrouched = CrouchSpeed;
	Move->MaxAcceleration = 3200.f;
	Move->BrakingDecelerationWalking = 2600.f;
	Move->GroundFriction = 10.f;
	Move->JumpZVelocity = 430.f;
	Move->GravityScale = 1.35f;
	Move->AirControl = 0.35f;
	Move->NavAgentProps.bCanCrouch = true;
	Move->SetCrouchedHalfHeight(58.f);
	Move->bCanWalkOffLedgesWhenCrouching = true;
	Move->PerchRadiusThreshold = 15.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, StandEyeHeight - 92.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(BaseFOV);

	ViewModelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ViewModelRoot"));
	ViewModelRoot->SetupAttachment(Camera);

	// No third-person body in first person.
	GetMesh()->SetOwnerNoSee(true);

	EyeHeightAboveFloor = StandEyeHeight;
	CurrentFOV = BaseFOV;
}

void AZMCharacter::BeginPlay()
{
	Super::BeginPlay();
	Health = MaxHealth = 100.f;
	GiveWeapon("MK9");
}

FVector AZMCharacter::GetPawnViewLocation() const
{
	return Camera ? Camera->GetComponentLocation() : Super::GetPawnViewLocation();
}

// ---------------------------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------------------------

void AZMCharacter::BuildInput()
{
	if (MappingContext) return;

	auto MakeAction = [this](const TCHAR* Name, EInputActionValueType Type)
	{
		UInputAction* A = NewObject<UInputAction>(this, Name);
		A->ValueType = Type;
		return A;
	};

	MoveAction = MakeAction(TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction = MakeAction(TEXT("IA_Look"), EInputActionValueType::Axis2D);
	GamepadLookAction = MakeAction(TEXT("IA_GamepadLook"), EInputActionValueType::Axis2D);
	JumpAction = MakeAction(TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction = MakeAction(TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	CrouchAction = MakeAction(TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	AdsAction = MakeAction(TEXT("IA_Ads"), EInputActionValueType::Boolean);
	FireAction = MakeAction(TEXT("IA_Fire"), EInputActionValueType::Boolean);
	ReloadAction = MakeAction(TEXT("IA_Reload"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(TEXT("IA_Interact"), EInputActionValueType::Boolean);
	MeleeAction = MakeAction(TEXT("IA_Melee"), EInputActionValueType::Boolean);
	NextWeaponAction = MakeAction(TEXT("IA_NextWeapon"), EInputActionValueType::Boolean);
	Slot1Action = MakeAction(TEXT("IA_Slot1"), EInputActionValueType::Boolean);
	Slot2Action = MakeAction(TEXT("IA_Slot2"), EInputActionValueType::Boolean);
	SensUpAction = MakeAction(TEXT("IA_SensUp"), EInputActionValueType::Boolean);
	SensDownAction = MakeAction(TEXT("IA_SensDown"), EInputActionValueType::Boolean);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Zombies"));
	UInputMappingContext* IMC = MappingContext;

	// WASD -> Axis2D (X = right, Y = forward)
	{
		FEnhancedActionKeyMapping& W = IMC->MapKey(MoveAction, EKeys::W);
		W.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		FEnhancedActionKeyMapping& S = IMC->MapKey(MoveAction, EKeys::S);
		S.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(this));
		S.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		FEnhancedActionKeyMapping& A = IMC->MapKey(MoveAction, EKeys::A);
		A.Modifiers.Add(NewObject<UInputModifierNegate>(this));
		IMC->MapKey(MoveAction, EKeys::D);
		IMC->MapKey(MoveAction, EKeys::Gamepad_Left2D);
	}
	IMC->MapKey(LookAction, EKeys::Mouse2D);
	IMC->MapKey(GamepadLookAction, EKeys::Gamepad_Right2D);

	IMC->MapKey(JumpAction, EKeys::SpaceBar);
	IMC->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
	IMC->MapKey(SprintAction, EKeys::LeftShift);
	IMC->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
	IMC->MapKey(CrouchAction, EKeys::LeftControl);
	IMC->MapKey(CrouchAction, EKeys::C);
	IMC->MapKey(CrouchAction, EKeys::Gamepad_FaceButton_Right);
	IMC->MapKey(AdsAction, EKeys::RightMouseButton);
	IMC->MapKey(AdsAction, EKeys::Gamepad_LeftTrigger);
	IMC->MapKey(FireAction, EKeys::LeftMouseButton);
	IMC->MapKey(FireAction, EKeys::Gamepad_RightTrigger);
	IMC->MapKey(ReloadAction, EKeys::R);
	IMC->MapKey(ReloadAction, EKeys::Gamepad_FaceButton_Left);
	IMC->MapKey(InteractAction, EKeys::F);
	IMC->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
	IMC->MapKey(MeleeAction, EKeys::V);
	IMC->MapKey(MeleeAction, EKeys::Gamepad_RightThumbstick);
	IMC->MapKey(NextWeaponAction, EKeys::MouseScrollUp);
	IMC->MapKey(NextWeaponAction, EKeys::MouseScrollDown);
	IMC->MapKey(NextWeaponAction, EKeys::Gamepad_FaceButton_Top);
	IMC->MapKey(Slot1Action, EKeys::One);
	IMC->MapKey(Slot2Action, EKeys::Two);
	IMC->MapKey(SensUpAction, EKeys::RightBracket);
	IMC->MapKey(SensDownAction, EKeys::LeftBracket);
}

void AZMCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	BuildInput();
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Sub->ClearAllMappings();
			Sub->AddMappingContext(MappingContext, 0);
		}
	}
}

void AZMCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	BuildInput();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("EAFPS: Enhanced Input component missing. Check Config/DefaultInput.ini."));
		return;
	}

	EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AZMCharacter::Input_Move);
	EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AZMCharacter::Input_Move);
	EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AZMCharacter::Input_Look);
	EIC->BindAction(GamepadLookAction, ETriggerEvent::Triggered, this, &AZMCharacter::Input_GamepadLook);
	EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AZMCharacter::Input_JumpStart);
	EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AZMCharacter::Input_SprintStart);
	EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AZMCharacter::Input_CrouchStart);
	EIC->BindAction(AdsAction, ETriggerEvent::Started, this, &AZMCharacter::Input_AdsStart);
	EIC->BindAction(AdsAction, ETriggerEvent::Completed, this, &AZMCharacter::Input_AdsStop);
	EIC->BindAction(FireAction, ETriggerEvent::Started, this, &AZMCharacter::Input_FireStart);
	EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AZMCharacter::Input_FireStop);
	EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &AZMCharacter::Input_Reload);
	EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AZMCharacter::Input_Interact);
	EIC->BindAction(MeleeAction, ETriggerEvent::Started, this, &AZMCharacter::Input_Melee);
	EIC->BindAction(NextWeaponAction, ETriggerEvent::Started, this, &AZMCharacter::Input_NextWeapon);
	EIC->BindAction(Slot1Action, ETriggerEvent::Started, this, &AZMCharacter::Input_Slot1);
	EIC->BindAction(Slot2Action, ETriggerEvent::Started, this, &AZMCharacter::Input_Slot2);
	EIC->BindAction(SensUpAction, ETriggerEvent::Started, this, &AZMCharacter::Input_SensUp);
	EIC->BindAction(SensDownAction, ETriggerEvent::Started, this, &AZMCharacter::Input_SensDown);
}

void AZMCharacter::Input_Move(const FInputActionValue& Value)
{
	MoveInput = Value.Get<FVector2D>();
}

static float AdsSensitivityScale(float CurrentFOV, float BaseFOV)
{
	// Keep on-screen motion consistent while zoomed (monitor-distance 0% matching).
	return FMath::Tan(FMath::DegreesToRadians(CurrentFOV * 0.5f)) / FMath::Tan(FMath::DegreesToRadians(BaseFOV * 0.5f));
}

void AZMCharacter::Input_Look(const FInputActionValue& Value)
{
	const FVector2D V = Value.Get<FVector2D>();
	const float Scale = LookSensitivity * AdsSensitivityScale(CurrentFOV, BaseFOV);
	AddControllerYawInput(V.X * Scale);
	AddControllerPitchInput(V.Y * Scale);
	LookDelta += V * Scale;
}

void AZMCharacter::Input_GamepadLook(const FInputActionValue& Value)
{
	FVector2D V = Value.Get<FVector2D>();
	// Response curve: precise near centre, fast at the edge.
	V.X = FMath::Sign(V.X) * FMath::Pow(FMath::Abs(V.X), 2.f);
	V.Y = FMath::Sign(V.Y) * FMath::Pow(FMath::Abs(V.Y), 2.f);
	const float Dt = GetWorld()->GetDeltaSeconds();
	const float Scale = GamepadLookRate * Dt * AdsSensitivityScale(CurrentFOV, BaseFOV);
	AddControllerYawInput(V.X * Scale);
	AddControllerPitchInput(V.Y * Scale);
	LookDelta += V * Scale;
}

void AZMCharacter::Input_SensUp()
{
	LookSensitivity = FMath::Min(LookSensitivity * 1.1f, 20.f);
	SensToast = 1.5f;
}

void AZMCharacter::Input_SensDown()
{
	LookSensitivity = FMath::Max(LookSensitivity / 1.1f, 0.02f);
	SensToast = 1.5f;
}

void AZMCharacter::Input_JumpStart()
{
	if (bDowned) return;
	if (bSliding)
	{
		EndSlide();
		UnCrouch();
		Jump();
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
		return;
	}
	Jump();
}

void AZMCharacter::Input_SprintStart()
{
	if (bDowned || bSliding) return;
	if (bSprinting)
	{
		bSprinting = false;
		SprintOutRemaining = GetCurrentWeapon() ? GetCurrentWeapon()->GetStats().SprintOutTime : 0.15f;
		return;
	}
	if (MoveInput.Y > 0.3f)
	{
		if (bIsCrouched) UnCrouch();
		bSprinting = true;
		bAdsHeld = false;
	}
}

void AZMCharacter::Input_CrouchStart()
{
	if (bDowned) return;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (bSprinting && Move->IsMovingOnGround() && GetVelocity().Size2D() > JogSpeed * 0.9f)
	{
		StartSlide();
		return;
	}
	if (bSliding)
	{
		EndSlide();
		return;
	}
	if (bIsCrouched) UnCrouch(); else Crouch();
}

void AZMCharacter::Input_AdsStart() { bAdsHeld = true; if (bSprinting) { bSprinting = false; SprintOutRemaining = 0.f; } }
void AZMCharacter::Input_AdsStop()  { bAdsHeld = false; }

void AZMCharacter::Input_FireStart()
{
	bFireHeld = true;
	if (bSprinting)
	{
		bSprinting = false;
		SprintOutRemaining = GetCurrentWeapon() ? GetCurrentWeapon()->GetStats().SprintOutTime : 0.15f;
	}
	if (AZMWeapon* W = GetCurrentWeapon()) W->SetTriggerHeld(true);
}

void AZMCharacter::Input_FireStop()
{
	bFireHeld = false;
	if (AZMWeapon* W = GetCurrentWeapon()) W->SetTriggerHeld(false);
}

void AZMCharacter::Input_Reload()
{
	if (AZMWeapon* W = GetCurrentWeapon())
	{
		if (MeleeAnim <= 0.f && SwitchAnim <= 0.f) W->StartReload();
	}
}

void AZMCharacter::Input_Interact()
{
	if (AZMGameMode* GM = GetWorld()->GetAuthGameMode<AZMGameMode>())
	{
		if (GM->GetPhase() == EZMMatchPhase::GameOver)
		{
			GM->RequestRestart();
			return;
		}
	}
	if (bDowned) return;
	if (FocusedInteractable)
	{
		FocusedInteractable->Interact(this);
	}
}

void AZMCharacter::Input_Melee()
{
	if (bDowned || MeleeCooldown > 0.f) return;
	if (AZMWeapon* W = GetCurrentWeapon()) W->CancelReload();
	bSprinting = false;
	MeleeAnim = 1.f;
	MeleeCooldown = 0.75f;
	bMeleePendingHit = true;
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Knife_Swing"));
}

void AZMCharacter::Input_NextWeapon()
{
	if (Weapons.Num() > 1) SwitchToWeapon((CurrentWeaponIndex + 1) % Weapons.Num());
}

void AZMCharacter::Input_Slot1() { SwitchToWeapon(0); }
void AZMCharacter::Input_Slot2() { SwitchToWeapon(1); }

// ---------------------------------------------------------------------------------------------
// Weapons
// ---------------------------------------------------------------------------------------------

AZMWeapon* AZMCharacter::GetCurrentWeapon() const
{
	return Weapons.IsValidIndex(CurrentWeaponIndex) ? Weapons[CurrentWeaponIndex].Get() : nullptr;
}

AZMWeapon* AZMCharacter::FindWeapon(FName WeaponId) const
{
	for (AZMWeapon* W : Weapons)
	{
		if (W && W->GetStats().Id == WeaponId) return W;
	}
	return nullptr;
}

bool AZMCharacter::HasWeapon(FName WeaponId) const
{
	return FindWeapon(WeaponId) != nullptr;
}

AZMWeapon* AZMCharacter::SpawnWeapon(FName WeaponId)
{
	const FZMWeaponStats* Stats = ZM::FindWeapon(WeaponId);
	if (!Stats) return nullptr;
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.Instigator = this;
	AZMWeapon* W = GetWorld()->SpawnActor<AZMWeapon>(AZMWeapon::StaticClass(), FTransform::Identity, Params);
	if (!W) return nullptr;
	W->Init(*Stats, this);
	W->AttachToComponent(ViewModelRoot, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	return W;
}

void AZMCharacter::GiveWeapon(FName WeaponId)
{
	if (AZMWeapon* Owned = FindWeapon(WeaponId))
	{
		Owned->RefillAmmo();
		return;
	}
	AZMWeapon* NewWeapon = SpawnWeapon(WeaponId);
	if (!NewWeapon) return;

	if (Weapons.Num() < GetMaxWeaponSlots())
	{
		Weapons.Add(NewWeapon);
		SwitchToWeapon(Weapons.Num() - 1);
	}
	else
	{
		if (AZMWeapon* Old = GetCurrentWeapon()) Old->Destroy();
		Weapons[CurrentWeaponIndex] = NewWeapon;
		SwitchToWeapon(CurrentWeaponIndex);
	}
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Weapon_Raise"));
}

void AZMCharacter::SwitchToWeapon(int32 Index)
{
	if (!Weapons.IsValidIndex(Index)) return;
	for (int32 i = 0; i < Weapons.Num(); ++i)
	{
		if (!Weapons[i]) continue;
		const bool bActive = (i == Index);
		Weapons[i]->SetActorHiddenInGame(!bActive);
		Weapons[i]->SetActorTickEnabled(bActive);
		if (!bActive)
		{
			Weapons[i]->CancelReload();
			Weapons[i]->SetTriggerHeld(false);
		}
	}
	if (Index != CurrentWeaponIndex || SwitchAnim <= 0.f)
	{
		SwitchAnim = 1.f;
	}
	CurrentWeaponIndex = Index;
	if (bFireHeld)
	{
		Weapons[Index]->SetTriggerHeld(true);
	}
}

void AZMCharacter::RefillAllAmmo()
{
	for (AZMWeapon* W : Weapons)
	{
		if (W) W->RefillAmmo();
	}
}

void AZMCharacter::GivePerk(EZMPerk Perk)
{
	if (HasPerk(Perk)) return;
	Perks.Add(Perk);
	if (Perk == EZMPerk::IronGut)
	{
		MaxHealth = 250.f;
		Health = MaxHealth;
	}
}

bool AZMCharacter::CanFire() const
{
	return !bSprinting && SprintOutRemaining <= 0.f && SwitchAnim < 0.3f && MeleeAnim <= 0.f;
}

float AZMCharacter::GetMoveSpreadFactor() const
{
	const float Speed = FMath::Clamp(GetVelocity().Size2D() / JogSpeed, 0.f, 1.f);
	const bool bAir = GetCharacterMovement()->IsFalling();
	return Speed + (bAir ? 1.5f : 0.f);
}

void AZMCharacter::AddRecoil(float Pitch, float Yaw)
{
	PendingRecoilPitch += Pitch;
	PendingRecoilYaw += Yaw;
}

void AZMCharacter::NotifyHitMarker(bool bKill, bool bHead)
{
	HitMarkerTime = bKill ? 0.3f : 0.18f;
	bHitMarkerKill = bKill;
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound(bKill ? (bHead ? "SFX_Hit_Head" : "SFX_Hit_Kill") : "SFX_Hit_Marker"), 0.7f);
}

void AZMCharacter::OnWeaponFired()
{
	bSprinting = false;
}

// ---------------------------------------------------------------------------------------------
// Health
// ---------------------------------------------------------------------------------------------

void AZMCharacter::ReceiveZombieHit(float Damage, const FVector& FromLocation)
{
	if (bDowned) return;
	AZMGameMode* GM = GetWorld()->GetAuthGameMode<AZMGameMode>();
	if (GM && GM->GetPhase() == EZMMatchPhase::GameOver) return;

	Health -= Damage;
	TimeSinceDamage = 0.f;
	DamageFlash = 1.f;
	LastDamageDir = (FromLocation - GetActorLocation()).GetSafeNormal2D();
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Player_Hurt"));

	// Hit flinch.
	if (AController* C = GetController())
	{
		FRotator R = C->GetControlRotation();
		R.Pitch += FMath::FRandRange(1.5f, 3.f);
		R.Yaw += FMath::FRandRange(-2.f, 2.f);
		C->SetControlRotation(R);
	}

	if (Health <= 0.f)
	{
		Health = 0.f;
		GoDown();
	}
}

void AZMCharacter::GoDown()
{
	bDowned = true;
	DownedTime = 0.f;
	bSprinting = false;
	bAdsHeld = false;
	if (bSliding) EndSlide();
	Crouch();
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Player_Down"));

	if (!HasPerk(EZMPerk::SecondWind))
	{
		if (AZMGameMode* GM = GetWorld()->GetAuthGameMode<AZMGameMode>())
		{
			GM->OnPlayerDied(this);
		}
	}
}

void AZMCharacter::Revive()
{
	bDowned = false;
	Perks.Reset();       // perks are lost when downed
	MaxHealth = 100.f;
	Health = MaxHealth;
	TimeSinceDamage = 100.f;
	UnCrouch();
	// Clear space around the player so they are not instantly downed again.
	for (TActorIterator<AZMZombie> It(GetWorld()); It; ++It)
	{
		if (!It->IsDead() && FVector::Dist(It->GetActorLocation(), GetActorLocation()) < 400.f)
		{
			It->LaunchCharacter((It->GetActorLocation() - GetActorLocation()).GetSafeNormal2D() * 900.f + FVector(0, 0, 200), true, true);
		}
	}
}

void AZMCharacter::UpdateHealth(float DeltaSeconds)
{
	TimeSinceDamage += DeltaSeconds;
	DamageFlash = FMath::Max(DamageFlash - DeltaSeconds * 1.6f, 0.f);

	if (bDowned)
	{
		DownedTime += DeltaSeconds;
		if (HasPerk(EZMPerk::SecondWind) && DownedTime >= 6.f)
		{
			Revive();
		}
		return;
	}
	if (TimeSinceDamage > RegenDelay && Health < MaxHealth)
	{
		Health = FMath::Min(Health + RegenRate * DeltaSeconds, MaxHealth);
	}
}

// ---------------------------------------------------------------------------------------------
// Movement
// ---------------------------------------------------------------------------------------------

void AZMCharacter::StartSlide()
{
	bSliding = true;
	bSprinting = false;
	SlideTime = 0.f;
	SlideDir = GetVelocity().GetSafeNormal2D();
	Crouch();
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->GroundFriction = 0.f;
	Move->BrakingDecelerationWalking = 0.f;
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Slide"));
}

void AZMCharacter::EndSlide()
{
	if (!bSliding) return;
	bSliding = false;
	UCharacterMovementComponent* Move = GetCharacterMovement();
	Move->GroundFriction = 10.f;
	Move->BrakingDecelerationWalking = 2600.f;
}

void AZMCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AZMCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
}

void AZMCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	const float Impact = FMath::Clamp(-GetCharacterMovement()->Velocity.Z / 900.f, 0.f, 1.f);
	LandDip = FMath::Max(LandDip, Impact);
	if (Impact > 0.25f)
	{
		UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Land"), Impact);
	}
}

void AZMCharacter::UpdateMovement(float DeltaSeconds)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	AZMWeapon* Weapon = GetCurrentWeapon();
	const float WeaponScale = Weapon ? Weapon->GetStats().MoveSpeedScale : 1.f;

	// Sprint ends when you stop pushing forward, aim, or leave the ground for long.
	if (bSprinting && (MoveInput.Y < 0.3f || bDowned))
	{
		bSprinting = false;
		SprintOutRemaining = Weapon ? Weapon->GetStats().SprintOutTime : 0.15f;
	}
	SprintOutRemaining = FMath::Max(SprintOutRemaining - DeltaSeconds, 0.f);
	SprintAlpha = FMath::FInterpTo(SprintAlpha, bSprinting ? 1.f : 0.f, DeltaSeconds, 10.f);

	float Target = FMath::Lerp(JogSpeed, AdsSpeed, AdsAlpha);
	if (bSprinting) Target = SprintSpeed;
	if (bDowned) Target = 60.f;
	Move->MaxWalkSpeed = Target * WeaponScale;
	Move->MaxWalkSpeedCrouched = (bDowned ? 60.f : CrouchSpeed) * WeaponScale;

	if (bSliding)
	{
		SlideTime += DeltaSeconds;
		const float A = FMath::Clamp(SlideTime / SlideDuration, 0.f, 1.f);
		const float Speed = FMath::Lerp(SlideSpeed, CrouchSpeed, A * A);
		// Light steering.
		const FVector Wish = (GetActorForwardVector() * MoveInput.Y + GetActorRightVector() * MoveInput.X).GetSafeNormal2D();
		if (!Wish.IsNearlyZero())
		{
			SlideDir = FMath::VInterpNormalRotationTo(SlideDir, Wish, DeltaSeconds, 45.f);
		}
		Move->Velocity.X = SlideDir.X * Speed;
		Move->Velocity.Y = SlideDir.Y * Speed;
		if (A >= 1.f || !Move->IsMovingOnGround())
		{
			EndSlide();
		}
	}
	else if (!MoveInput.IsNearlyZero())
	{
		AddMovementInput(GetActorForwardVector(), MoveInput.Y);
		AddMovementInput(GetActorRightVector(), MoveInput.X);
	}
}

// ---------------------------------------------------------------------------------------------
// Camera & viewmodel
// ---------------------------------------------------------------------------------------------

void AZMCharacter::UpdateCamera(float DeltaSeconds)
{
	AZMWeapon* Weapon = GetCurrentWeapon();
	const bool bCanAds = bAdsHeld && !bSprinting && Weapon && !Weapon->IsReloading() && MeleeAnim <= 0.f && !bSliding;
	const float AdsTime = Weapon ? Weapon->GetStats().AdsTime : 0.2f;
	AdsAlpha = FMath::Clamp(AdsAlpha + (bCanAds ? 1.f : -1.f) * DeltaSeconds / AdsTime, 0.f, 1.f);
	const float AdsEase = AdsAlpha * AdsAlpha * (3.f - 2.f * AdsAlpha);

	const float Zoom = Weapon ? Weapon->GetStats().AdsZoom : 1.f;
	CurrentFOV = BaseFOV / FMath::Lerp(1.f, Zoom, AdsEase) + SprintFOVBoost * SprintAlpha + (bSliding ? 4.f : 0.f);
	Camera->SetFieldOfView(CurrentFOV);

	// Eye height measured from the floor so crouch / slide transitions are continuous.
	float TargetEye = StandEyeHeight;
	if (bDowned) TargetEye = 60.f;
	else if (bSliding) TargetEye = SlideEyeHeight;
	else if (bIsCrouched) TargetEye = CrouchEyeHeight;
	EyeHeightAboveFloor = FMath::FInterpTo(EyeHeightAboveFloor, TargetEye, DeltaSeconds, 12.f);
	LandDip = FMath::FInterpTo(LandDip, 0.f, DeltaSeconds, 6.f);
	const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	Camera->SetRelativeLocation(FVector(0.f, 0.f, EyeHeightAboveFloor - HalfHeight - LandDip * 8.f));

	// Recoil climb: apply most of it this frame, the rest smoothly over the next few.
	if (AController* C = GetController())
	{
		const float Apply = FMath::Min(1.f, DeltaSeconds * 28.f);
		const float P = PendingRecoilPitch * Apply;
		const float Y = PendingRecoilYaw * Apply;
		PendingRecoilPitch -= P;
		PendingRecoilYaw -= Y;
		if (!FMath::IsNearlyZero(P) || !FMath::IsNearlyZero(Y))
		{
			FRotator R = C->GetControlRotation();
			R.Pitch += P;
			R.Yaw += Y;
			C->SetControlRotation(R);
		}
	}
}

void AZMCharacter::UpdateViewModel(float DeltaSeconds)
{
	AZMWeapon* Weapon = GetCurrentWeapon();
	if (!Weapon) return;
	const FZMWeaponStats& S = Weapon->GetStats();
	const float AdsEase = AdsAlpha * AdsAlpha * (3.f - 2.f * AdsAlpha);
	const float HipW = 1.f - AdsEase;

	FVector Loc = FMath::Lerp(S.HipOffset, S.GetAdsOffset(), AdsEase);
	FRotator Rot = FRotator::ZeroRotator;

	// Sway: viewmodel lags behind look input.
	const FRotator SwayTarget(
		FMath::Clamp(-LookDelta.Y * 0.9f, -6.f, 6.f),
		FMath::Clamp(-LookDelta.X * 0.9f, -6.f, 6.f),
		FMath::Clamp(-LookDelta.X * 1.6f, -8.f, 8.f));
	SwayRot = FMath::RInterpTo(SwayRot, SwayTarget * FMath::Lerp(1.f, 0.15f, AdsEase), DeltaSeconds, 9.f);
	LookDelta = FVector2D::ZeroVector;
	Rot += SwayRot;
	const FVector LocalVel = GetActorTransform().InverseTransformVectorNoScale(GetVelocity());
	SwayLoc = FMath::VInterpTo(SwayLoc, FVector(0.f, -LocalVel.Y * 0.004f, -LocalVel.Z * 0.0025f) * HipW, DeltaSeconds, 8.f);
	Loc += SwayLoc;

	// Bob.
	const float Speed = GetCharacterMovement()->IsMovingOnGround() && !bSliding ? GetVelocity().Size2D() : 0.f;
	BobPhase += DeltaSeconds * Speed / (bSprinting ? 52.f : 68.f);
	const float BobAmp = FMath::Clamp(Speed / JogSpeed, 0.f, 1.6f) * FMath::Lerp(1.f, 0.12f, AdsEase);
	Loc += FVector(0.f, FMath::Sin(BobPhase) * 0.55f, -FMath::Abs(FMath::Cos(BobPhase)) * 0.45f) * BobAmp;
	Rot.Roll += FMath::Sin(BobPhase) * 0.6f * BobAmp;

	// Idle breathing.
	const float T = GetWorld()->GetTimeSeconds();
	Loc.Z += FMath::Sin(T * 1.6f) * 0.12f * HipW;

	// Sprint pose: weapon canted and lowered.
	Loc += FVector(-4.f, -3.f, -4.f) * SprintAlpha;
	Rot += FRotator(-12.f, 28.f, -18.f) * SprintAlpha;

	// Slide tilt.
	SlideTilt = FMath::FInterpTo(SlideTilt, bSliding ? 1.f : 0.f, DeltaSeconds, 8.f);
	Rot.Roll -= 12.f * SlideTilt;
	Loc.Z -= 1.5f * SlideTilt;

	// Reload: dip down and roll in, come back up at the end.
	if (Weapon->IsReloading())
	{
		const float A = Weapon->GetReloadAlpha();
		const float Dip = FMath::Sin(FMath::Clamp(A, 0.f, 1.f) * PI);
		Loc += FVector(-2.f, -2.f, -7.f) * Dip;
		Rot += FRotator(-14.f, 6.f, 32.f) * Dip;
	}

	// Weapon raise after switching.
	SwitchAnim = FMath::Max(SwitchAnim - DeltaSeconds / 0.45f, 0.f);
	Loc.Z -= 18.f * SwitchAnim * SwitchAnim;
	Rot.Pitch -= 25.f * SwitchAnim * SwitchAnim;

	// Melee jab.
	if (MeleeAnim > 0.f)
	{
		const float Prev = MeleeAnim;
		MeleeAnim = FMath::Max(MeleeAnim - DeltaSeconds / 0.45f, 0.f);
		const float Swing = FMath::Sin((1.f - MeleeAnim) * PI);
		Loc += FVector(8.f, -10.f, 2.f) * Swing;
		Rot += FRotator(-8.f, -35.f, 25.f) * Swing;
		if (bMeleePendingHit && Prev > 0.7f && MeleeAnim <= 0.7f)
		{
			bMeleePendingHit = false;
			DoMeleeHit();
		}
	}
	MeleeCooldown = FMath::Max(MeleeCooldown - DeltaSeconds, 0.f);

	// Landing.
	Loc.Z -= LandDip * 3.f;

	// Lower the weapon when it would clip into a wall.
	{
		FHitResult Hit;
		FCollisionQueryParams Q(SCENE_QUERY_STAT(ZMWallLower), false, this);
		const FVector Start = Camera->GetComponentLocation();
		const FVector End = Start + Camera->GetForwardVector() * (S.BodySize.X + 30.f);
		const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Camera, Q) && !Cast<AZMZombie>(Hit.GetActor());
		WallLower = FMath::FInterpTo(WallLower, bBlocked ? 1.f : 0.f, DeltaSeconds, 10.f);
		Loc += FVector(-10.f, 0.f, -6.f) * WallLower;
		Rot += FRotator(-20.f, 10.f, 0.f) * WallLower;
	}

	Loc += Weapon->GetKickOffset();
	Rot += Weapon->GetKickRotation();

	ViewModelRoot->SetRelativeLocationAndRotation(Loc, Rot);
}

void AZMCharacter::DoMeleeHit()
{
	FVector Eye;
	FRotator EyeRot;
	GetActorEyesViewPoint(Eye, EyeRot);
	FCollisionQueryParams Q(SCENE_QUERY_STAT(ZMMelee), false, this);
	for (AZMWeapon* W : Weapons) Q.AddIgnoredActor(W);
	FHitResult Hit;
	const FVector End = Eye + EyeRot.Vector() * MeleeRange;
	if (GetWorld()->SweepSingleByChannel(Hit, Eye, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(22.f), Q))
	{
		if (AZMZombie* Z = Cast<AZMZombie>(Hit.GetActor()))
		{
			const bool bKilled = Z->ApplyHit(MeleeDamage, false, true, this, Hit.ImpactPoint, EyeRot.Vector());
			NotifyHitMarker(bKilled, false);
		}
		UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Knife_Hit"));
	}
}

void AZMCharacter::UpdateInteraction()
{
	FocusedInteractable = nullptr;
	if (bDowned) return;
	const FVector Eye = Camera->GetComponentLocation();
	const FVector Fwd = Camera->GetForwardVector();
	float BestScore = 0.f;
	for (TActorIterator<AZMInteractable> It(GetWorld()); It; ++It)
	{
		const FVector To = It->GetInteractLocation() - Eye;
		const float Dist = To.Size();
		if (Dist > InteractRange) continue;
		const float Dot = FVector::DotProduct(To / FMath::Max(Dist, 1.f), Fwd);
		if (Dot < 0.55f && Dist > 80.f) continue;
		const float Score = Dot + (1.f - Dist / InteractRange);
		if (Score > BestScore)
		{
			BestScore = Score;
			FocusedInteractable = *It;
		}
	}
}

void AZMCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	HitMarkerTime = FMath::Max(HitMarkerTime - DeltaSeconds, 0.f);
	SensToast = FMath::Max(SensToast - DeltaSeconds, 0.f);

	UpdateHealth(DeltaSeconds);
	UpdateMovement(DeltaSeconds);
	UpdateCamera(DeltaSeconds);
	UpdateViewModel(DeltaSeconds);
	UpdateInteraction();
}
