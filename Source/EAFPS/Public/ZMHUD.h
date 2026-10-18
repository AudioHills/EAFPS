#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Fonts/SlateFontInfo.h"
#include "ZMHUD.generated.h"

class AZMCharacter;
class AZMGameMode;
class UFont;

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
	void DrawPowerUps(AZMGameMode* GM, float Scale);
	void DrawDamage(AZMCharacter* Player, float Scale);
	void DrawPrompts(AZMCharacter* Player, AZMGameMode* GM, float Scale);
	void DrawGameOver(AZMCharacter* Player, AZMGameMode* GM, float Scale);

	/** Text helpers. Size is the font height in pixels at 1080p (pre-scaled by Scale). Align: 0 left, 0.5 centre, 1 right. */
	FSlateFontInfo MakeFont(float Size, bool bBold) const;
	FVector2D MeasureLabel(const FString& Text, const FSlateFontInfo& Font) const;
	void DrawLabel(const FString& Text, float X, float Y, float Size, const FLinearColor& Color, float Align = 0.f, bool bBold = true, bool bShadow = true);
	void DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Size, bool bShadow = true);
	void DrawRightAligned(const FString& Text, float RightX, float Y, const FLinearColor& Color, float Size);
	void DrawLineThick(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Thickness);
	void DrawBox(float X, float Y, float W, float H, const FLinearColor& Color);

	UPROPERTY() TObjectPtr<UFont> UIFont;

	int32 LastRoundDrawn = -1;
	float RoundChangeTime = -100.f;
};
