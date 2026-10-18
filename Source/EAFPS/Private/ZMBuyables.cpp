#include "ZMBuyables.h"
#include "ZMCharacter.h"
#include "ZMWeapon.h"
#include "ZMGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "ZM"

static void SetupGreyboxPart(UStaticMeshComponent* C, bool bCollide)
{
	C->SetCollisionProfileName(bCollide ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
	C->SetGenerateOverlapEvents(false);
	C->SetCanEverAffectNavigation(bCollide);
}

// ---------------------------------------------------------------------------------------------
// Wall buy
// ---------------------------------------------------------------------------------------------

AZMWallBuy::AZMWallBuy()
{
	Board = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Board"));
	Board->SetupAttachment(Root);
	SetupGreyboxPart(Board, false);

	ModelRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ModelRoot"));
	ModelRoot->SetupAttachment(Root);
	ModelRoot->SetRelativeLocationAndRotation(FVector(4.f, 0.f, 8.f), FRotator(0.f, 90.f, 0.f));

	Label = MakeLabel(TEXT("Label"), FVector(3.f, 0.f, -26.f), FRotator::ZeroRotator, 9.f, FColor(225, 220, 200));
}

void AZMWallBuy::BeginPlay()
{
	Super::BeginPlay();
	const FZMWeaponStats* Stats = ZM::FindWeapon(WeaponId);

	// Chalk board: a pale outline plate on the wall.
	Board->SetStaticMesh(ZM::CubeMesh());
	const float Len = Stats ? Stats->BodySize.X * 1.9f + 30.f : 90.f;
	Board->SetRelativeScale3D(FVector(0.02f, Len / 100.f, 0.62f));
	Board->SetRelativeLocation(FVector(1.f, 0.f, 0.f));
	Board->SetMaterial(0, ZM::MakeTintedMaterial(this, FLinearColor(0.32f, 0.3f, 0.26f)));

	if (Stats)
	{
		AZMWeapon::BuildModel(this, ModelRoot, *Stats, false);
		Label->SetText(FText::Format(LOCTEXT("WallBuyLabel", "{0}  {1}"), Stats->DisplayName, FText::AsNumber(Stats->WallCost)));
	}
}

FText AZMWallBuy::GetPrompt(const AZMCharacter* User) const
{
	const FZMWeaponStats* Stats = ZM::FindWeapon(WeaponId);
	if (!Stats || !User) return FText::GetEmpty();
	if (const AZMWeapon* Owned = User->FindWeapon(WeaponId))
	{
		if (Owned->IsAmmoFull()) return FText::GetEmpty();
		return CostPrompt(FText::Format(LOCTEXT("BuyAmmo", "buy ammo for {0}"), Stats->DisplayName), Stats->WallCost / 2);
	}
	return CostPrompt(FText::Format(LOCTEXT("BuyWeapon", "buy {0}"), Stats->DisplayName), Stats->WallCost);
}

void AZMWallBuy::Interact(AZMCharacter* User)
{
	const FZMWeaponStats* Stats = ZM::FindWeapon(WeaponId);
	if (!Stats || !User) return;
	if (const AZMWeapon* Owned = User->FindWeapon(WeaponId))
	{
		if (Owned->IsAmmoFull()) return;
		if (TryPurchase(User, Stats->WallCost / 2)) User->GiveWeapon(WeaponId);
		return;
	}
	if (TryPurchase(User, Stats->WallCost)) User->GiveWeapon(WeaponId);
}

// ---------------------------------------------------------------------------------------------
// Door / debris
// ---------------------------------------------------------------------------------------------

AZMDoor::AZMDoor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Panel = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Panel"));
	Panel->SetupAttachment(Root);
	SetupGreyboxPart(Panel, true);

	Label = MakeLabel(TEXT("Label"), FVector(17.f, 0.f, 60.f), FRotator::ZeroRotator, 26.f, FColor(230, 210, 160));
}

void AZMDoor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Panel->SetStaticMesh(ZM::CubeMesh());
	Panel->SetRelativeScale3D(Size / 100.f);
	Panel->SetMaterial(0, ZM::MakeTintedMaterial(this, FLinearColor(0.16f, 0.09f, 0.045f)));
	Label->SetRelativeLocation(FVector(Size.X * 0.5f + 2.f, 0.f, Size.Z * 0.2f));
	Label->SetText(FText::AsNumber(Cost));
}

FText AZMDoor::GetPrompt(const AZMCharacter* User) const
{
	if (bOpening) return FText::GetEmpty();
	return CostPrompt(LOCTEXT("ClearDebris", "clear the debris"), Cost);
}

void AZMDoor::Interact(AZMCharacter* User)
{
	if (bOpening || !TryPurchase(User, Cost)) return;
	bOpening = true;
	OpenTime = 0.f;
	Panel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Label->SetVisibility(false);
	SetActorTickEnabled(true);
	ZM::PlayWorldSound(this, "SFX_Door_Open", GetActorLocation());
	if (AZMGameMode* GM = GetWorld()->GetAuthGameMode<AZMGameMode>())
	{
		GM->ActivateZone(UnlocksZone);
	}
}

void AZMDoor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bOpening) return;
	OpenTime += DeltaSeconds;
	const float A = FMath::Clamp(OpenTime / 0.9f, 0.f, 1.f);
	Panel->SetRelativeLocation(FVector(0.f, 0.f, A * A * Size.Z));
	Panel->SetRelativeScale3D(Size / 100.f * FMath::Max(1.f - A, 0.01f));
	if (A >= 1.f) Destroy();
}

// ---------------------------------------------------------------------------------------------
// Perk machine
// ---------------------------------------------------------------------------------------------

AZMPerkMachine::AZMPerkMachine()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(Root);
	Body->SetRelativeLocation(FVector(0.f, 0.f, 95.f));
	Body->SetRelativeScale3D(FVector(0.7f, 0.85f, 1.9f));
	SetupGreyboxPart(Body, true);

	Sign = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Sign"));
	Sign->SetupAttachment(Root);
	Sign->SetRelativeLocation(FVector(4.f, 0.f, 212.f));
	Sign->SetRelativeScale3D(FVector(0.66f, 0.9f, 0.34f));
	SetupGreyboxPart(Sign, false);

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(Root);
	Glow->SetRelativeLocation(FVector(70.f, 0.f, 170.f));
	Glow->SetIntensity(9000.f);
	Glow->SetAttenuationRadius(420.f);
	Glow->SetCastShadows(false);

	Label = MakeLabel(TEXT("Label"), FVector(39.f, 0.f, 212.f), FRotator::ZeroRotator, 13.f, FColor::White);
}

void AZMPerkMachine::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	const FLinearColor C = ZM::PerkColor(Perk);
	Body->SetStaticMesh(ZM::CubeMesh());
	Body->SetMaterial(0, ZM::MakeTintedMaterial(this, C * 0.35f));
	Sign->SetStaticMesh(ZM::CubeMesh());
	Sign->SetMaterial(0, ZM::MakeTintedMaterial(this, C));
	Glow->SetLightColor(C);
	Label->SetText(ZM::PerkName(Perk));
}

FText AZMPerkMachine::GetPrompt(const AZMCharacter* User) const
{
	if (!User || User->HasPerk(Perk)) return FText::GetEmpty();
	return CostPrompt(FText::Format(LOCTEXT("BuyPerk", "buy {0}"), ZM::PerkName(Perk)), ZM::PerkCost(Perk));
}

void AZMPerkMachine::Interact(AZMCharacter* User)
{
	if (!User || User->HasPerk(Perk)) return;
	if (TryPurchase(User, ZM::PerkCost(Perk)))
	{
		User->GivePerk(Perk);
		UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Perk_Drink"));
	}
}

void AZMPerkMachine::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	FlickerTime -= DeltaSeconds;
	if (FlickerTime <= 0.f)
	{
		// Mostly steady with an occasional stutter.
		const bool bStutter = FMath::FRand() < 0.06f;
		Glow->SetIntensity(bStutter ? 1500.f : FMath::FRandRange(8000.f, 10000.f));
		FlickerTime = bStutter ? 0.05f : FMath::FRandRange(0.08f, 0.3f);
	}
}

// ---------------------------------------------------------------------------------------------
// Mystery box
// ---------------------------------------------------------------------------------------------

namespace
{
	constexpr float BoxRollTime = 4.2f;
	constexpr float BoxOfferTime = 9.f;

	TArray<FName> BoxPool(const AZMCharacter* Exclude)
	{
		TArray<FName> Out;
		for (const FZMWeaponStats& W : ZM::GetWeaponCatalog())
		{
			if (W.bInMysteryBox && !(Exclude && Exclude->HasWeapon(W.Id))) Out.Add(W.Id);
		}
		return Out;
	}
}

AZMMysteryBox::AZMMysteryBox()
{
	PrimaryActorTick.bCanEverTick = true;

	Crate = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Crate"));
	Crate->SetupAttachment(Root);
	Crate->SetRelativeLocation(FVector(0.f, 0.f, 26.f));
	Crate->SetRelativeScale3D(FVector(0.56f, 1.15f, 0.52f));
	SetupGreyboxPart(Crate, true);

	Lid = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Lid"));
	Lid->SetupAttachment(Root);
	Lid->SetRelativeLocation(FVector(0.f, 0.f, 56.f));
	Lid->SetRelativeScale3D(FVector(0.58f, 1.17f, 0.08f));
	SetupGreyboxPart(Lid, false);

	WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
	WeaponRoot->SetupAttachment(Root);
	WeaponRoot->SetRelativeLocationAndRotation(FVector(0.f, 0.f, 55.f), FRotator(0.f, 90.f, 0.f));

	Beam = CreateDefaultSubobject<UPointLightComponent>(TEXT("Beam"));
	Beam->SetupAttachment(Root);
	Beam->SetRelativeLocation(FVector(0.f, 0.f, 140.f));
	Beam->SetLightColor(FLinearColor(0.35f, 0.6f, 1.f));
	Beam->SetAttenuationRadius(600.f);
	Beam->SetIntensity(2500.f);
	Beam->SetCastShadows(false);
}

void AZMMysteryBox::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	Crate->SetStaticMesh(ZM::CubeMesh());
	Crate->SetMaterial(0, ZM::MakeTintedMaterial(this, FLinearColor(0.12f, 0.07f, 0.03f)));
	Lid->SetStaticMesh(ZM::CubeMesh());
	Lid->SetMaterial(0, ZM::MakeTintedMaterial(this, FLinearColor(0.2f, 0.12f, 0.05f)));
}

FText AZMMysteryBox::GetPrompt(const AZMCharacter* User) const
{
	switch (State)
	{
	case EState::Idle:
		return CostPrompt(LOCTEXT("BoxBuy", "buy a random weapon"), Cost);
	case EState::Offering:
		if (User && Buyer.Get() == User)
		{
			if (const FZMWeaponStats* S = ZM::FindWeapon(OfferedWeapon))
			{
				return FText::Format(LOCTEXT("BoxTake", "Press F to take {0}"), S->DisplayName);
			}
		}
		break;
	default:
		break;
	}
	return FText::GetEmpty();
}

void AZMMysteryBox::Interact(AZMCharacter* User)
{
	if (!User) return;
	if (State == EState::Idle)
	{
		if (BoxPool(User).Num() == 0 || !TryPurchase(User, Cost)) return;
		Buyer = User;
		State = EState::Rolling;
		StateTime = 0.f;
		NextCycle = 0.f;
		ZM::PlayWorldSound(this, "SFX_Box_Jingle", GetActorLocation());
	}
	else if (State == EState::Offering && Buyer.Get() == User)
	{
		User->GiveWeapon(OfferedWeapon);
		ShowWeapon(NAME_None);
		State = EState::Idle;
		StateTime = 0.f;
	}
}

void AZMMysteryBox::ShowWeapon(FName Id)
{
	if (Id == ShownWeapon) return;
	ShownWeapon = Id;
	if (ShownModel)
	{
		TArray<USceneComponent*> Kids;
		ShownModel->GetChildrenComponents(true, Kids);
		for (USceneComponent* C : Kids) C->DestroyComponent();
		ShownModel->DestroyComponent();
		ShownModel = nullptr;
	}
	const FZMWeaponStats* Stats = ZM::FindWeapon(Id);
	if (!Stats) return;
	ShownModel = NewObject<USceneComponent>(this);
	ShownModel->SetupAttachment(WeaponRoot);
	ShownModel->RegisterComponent();
	// Centre the model over the box.
	ShownModel->SetRelativeLocation(FVector(-Stats->BodySize.X * 0.25f, 0.f, 0.f));
	AZMWeapon::BuildModel(this, ShownModel, *Stats, false);
}

void AZMMysteryBox::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StateTime += DeltaSeconds;

	float LidOpen = 0.f;
	float Rise = 0.f;
	switch (State)
	{
	case EState::Idle:
		Beam->SetIntensity(2500.f + 600.f * FMath::Sin(GetWorld()->GetTimeSeconds() * 2.f));
		break;

	case EState::Rolling:
	{
		LidOpen = FMath::Clamp(StateTime / 0.4f, 0.f, 1.f);
		Rise = FMath::Clamp(StateTime / BoxRollTime, 0.f, 1.f);
		Beam->SetIntensity(14000.f);
		NextCycle -= DeltaSeconds;
		if (NextCycle <= 0.f)
		{
			const TArray<FName> Pool = BoxPool(nullptr);
			if (Pool.Num() > 0) ShowWeapon(Pool[FMath::RandRange(0, Pool.Num() - 1)]);
			// Slows down as it settles.
			NextCycle = FMath::Lerp(0.07f, 0.4f, Rise * Rise);
		}
		if (StateTime >= BoxRollTime)
		{
			const TArray<FName> Pool = BoxPool(Buyer.Get());
			OfferedWeapon = Pool.Num() > 0 ? Pool[FMath::RandRange(0, Pool.Num() - 1)] : NAME_None;
			ShowWeapon(OfferedWeapon);
			State = OfferedWeapon.IsNone() ? EState::Idle : EState::Offering;
			StateTime = 0.f;
		}
		break;
	}

	case EState::Offering:
		LidOpen = 1.f;
		// Sinks back into the box during the last few seconds.
		Rise = 1.f - FMath::Clamp((StateTime - (BoxOfferTime - 3.f)) / 3.f, 0.f, 1.f);
		Beam->SetIntensity(14000.f);
		if (StateTime >= BoxOfferTime || !Buyer.IsValid())
		{
			ShowWeapon(NAME_None);
			State = EState::Idle;
			StateTime = 0.f;
		}
		break;
	}

	Lid->SetRelativeLocationAndRotation(FVector(-24.f * LidOpen, 0.f, 56.f + 24.f * LidOpen), FRotator(70.f * LidOpen, 0.f, 0.f));
	WeaponRoot->SetRelativeLocation(FVector(0.f, 0.f, 40.f + 45.f * Rise));
	WeaponRoot->SetRelativeRotation(FRotator(0.f, 90.f + (State == EState::Rolling ? FMath::Sin(StateTime * 9.f) * 8.f : 0.f), 0.f));
}

#undef LOCTEXT_NAMESPACE
