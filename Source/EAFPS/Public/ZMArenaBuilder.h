#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZMArenaBuilder.generated.h"

class UStaticMeshComponent;

/**
 * Builds the default greybox map ("Outpost") at runtime when the level has no spawn points:
 * a lit courtyard start room, two purchasable side zones, wall buys, perks, mystery box,
 * lighting, fog and post-processing. Lets the game run in a completely empty level.
 */
UCLASS()
class EAFPS_API AZMArenaBuilder : public AActor
{
	GENERATED_BODY()

public:
	AZMArenaBuilder();
	virtual void BeginPlay() override;

	/** Player start location for the built arena. */
	static FVector GetPlayerStart() { return FVector(0.f, 0.f, 120.f); }

protected:
	void Box(const FVector& Center, const FVector& Size, const FLinearColor& Color, bool bCollide = true);
	void Wall(const FVector& From, const FVector& To, float Height, float Thickness, const FLinearColor& Color);
	void Light(const FVector& Location, const FLinearColor& Color, float Intensity, float Radius, bool bFlicker = false);
	void Spawner(const FVector& Location, int32 Zone);
	void Atmosphere();

	template <typename T>
	T* Place(const FVector& Location, const FRotator& Rotation);

	UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
};
