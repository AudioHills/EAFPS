#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ZMHUD.generated.h"

class AZMCharacter;
class AZMGameMode;

/** Canvas-drawn HUD in the classic zombies layout: round tally bottom-left, score right, ammo bottom-right. */
UCLASS()
class EAFPS_API AZMHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void DrawHUD() override;

protected:
	void DrawCrosshair(AZMCharacter* Player, float Scale);
	void DrawRound(AZMGameMode* GM, float Scale);
	void DrawScore(AZMCharacter* Player, AZMGameMode* GM, float Scale);
	void DrawAmmo(AZMCharacter* Player, float Scale);
	void DrawPerks(AZMCharacter* Player, float Scale);
	void DrawDamage(AZMCharacter* Player, float Scale);
	void DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, UFont* Font, float Scale, bool bShadow = true);
	void DrawRightAligned(const FString& Text, float RightX, float Y, const FLinearColor& Color, UFont* Font, float Scale);

	float LastRoundDrawn = -1.f;
};
