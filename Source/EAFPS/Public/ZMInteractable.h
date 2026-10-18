#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZMInteractable.generated.h"

class AZMCharacter;
class UTextRenderComponent;

/** Anything the player can buy or use with the interact key. */
UCLASS(Abstract)
class EAFPS_API AZMInteractable : public AActor
{
	GENERATED_BODY()

public:
	AZMInteractable();

	/** Prompt shown on the HUD, e.g. "Press F to buy Vandal SMG [Cost: 1000]". Empty = not usable now. */
	virtual FText GetPrompt(const AZMCharacter* User) const { return FText::GetEmpty(); }
	virtual void Interact(AZMCharacter* User) {}
	virtual FVector GetInteractLocation() const { return GetActorLocation(); }

	static FText CostPrompt(const FText& Action, int32 Cost);

protected:
	bool TryPurchase(AZMCharacter* User, int32 Cost) const;
	UTextRenderComponent* MakeLabel(const FName Name, const FVector& RelLoc, const FRotator& RelRot, float Size, const FColor& Color);

	UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Root;
};
