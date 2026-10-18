#include "ZMInteractable.h"
#include "ZMCharacter.h"
#include "ZMPlayerState.h"
#include "ZMTypes.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"

#define LOCTEXT_NAMESPACE "ZM"

AZMInteractable::AZMInteractable()
{
	PrimaryActorTick.bCanEverTick = false;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;
}

FText AZMInteractable::CostPrompt(const FText& Action, int32 Cost)
{
	return FText::Format(LOCTEXT("CostPrompt", "Press F to {0} [Cost: {1}]"), Action, FText::AsNumber(Cost));
}

bool AZMInteractable::TryPurchase(AZMCharacter* User, int32 Cost) const
{
	AZMPlayerState* PS = User ? User->GetPlayerState<AZMPlayerState>() : nullptr;
	if (!PS || !PS->SpendPoints(Cost))
	{
		UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Deny"));
		return false;
	}
	UGameplayStatics::PlaySound2D(this, ZM::LoadSound("SFX_Purchase"));
	return true;
}

UTextRenderComponent* AZMInteractable::MakeLabel(const FName Name, const FVector& RelLoc, const FRotator& RelRot, float Size, const FColor& Color)
{
	// Constructor-only (uses CreateDefaultSubobject).
	UTextRenderComponent* T = CreateDefaultSubobject<UTextRenderComponent>(Name);
	T->SetupAttachment(Root);
	T->SetRelativeLocationAndRotation(RelLoc, RelRot);
	T->SetHorizontalAlignment(EHTA_Center);
	T->SetVerticalAlignment(EVRTA_TextCenter);
	T->SetWorldSize(Size);
	T->SetTextRenderColor(Color);
	T->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	T->SetCastShadow(false);
	return T;
}

#undef LOCTEXT_NAMESPACE
