#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ZMSettings.generated.h"

class USkeletalMesh;
class UAnimInstance;

/** Project Settings > Game > Zombies. Asset hookups that the code falls back on gracefully. */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Zombies"))
class EAFPS_API UZMSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** Humanoid mesh used for zombies (UE mannequin by default). Empty = greybox capsule zombie. */
	UPROPERTY(Config, EditAnywhere, Category = "Zombie")
	TSoftObjectPtr<USkeletalMesh> ZombieMesh;

	/** Locomotion anim blueprint compatible with ZombieMesh. */
	UPROPERTY(Config, EditAnywhere, Category = "Zombie")
	TSoftClassPtr<UAnimInstance> ZombieAnimClass;

	virtual FName GetCategoryName() const override { return TEXT("Game"); }
};
