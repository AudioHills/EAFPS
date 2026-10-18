#include "ZMSpawnPoint.h"
#include "Components/ArrowComponent.h"

AZMSpawnPoint::AZMSpawnPoint()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

#if WITH_EDITORONLY_DATA
	UArrowComponent* Arrow = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
	if (Arrow)
	{
		Arrow->SetupAttachment(RootComponent);
		Arrow->ArrowColor = FColor(200, 30, 30);
	}
#endif
}
