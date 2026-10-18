#include "ZMHUD.h"
#include "ZMCharacter.h"
#include "ZMGameMode.h"
#include "ZMPlayerState.h"
#include "ZMWeapon.h"
#include "ZMInteractable.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Camera/PlayerCameraManager.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Rendering/SlateRenderer.h"

#define LOCTEXT_NAMESPACE "ZM"

namespace
{
	const FLinearColor RoundRed(0.5f, 0.015f, 0.01f);
	const FLinearColor ScoreColor(0.95f, 0.88f, 0.55f);
	const FLinearColor TextWhite(0.92f, 0.92f, 0.9f);
	constexpr float PopupLife = 1.1f;
}

// ---------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------

FSlateFontInfo AZMHUD::MakeFont(float Size, bool bBold) const
{
	// Slate font sizes are points at 96 DPI; convert from the pixel height we want.
	return FSlateFontInfo(UIFont, FMath::Max(Size * 0.75f, 4.f), bBold ? FName("Bold") : FName("Regular"));
}

FVector2D AZMHUD::MeasureLabel(const FString& Text, const FSlateFontInfo& Font) const
{
	if (!FSlateApplication::IsInitialized()) return FVector2D(Text.Len() * Font.Size * 0.6f, Font.Size * 1.33f);
	return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font);
}

void AZMHUD::DrawLabel(const FString& Text, float X, float Y, float Size, const FLinearColor& Color, float Align, bool bBold, bool bShadow)
{
	if (Text.IsEmpty() || Color.A <= 0.01f) return;
	const FSlateFontInfo Font = MakeFont(Size, bBold);
	const FVector2D Dim = MeasureLabel(Text, Font);
	FCanvasTextItem Item(FVector2D(X - Dim.X * Align, Y), FText::FromString(Text), Font, Color);
	if (bShadow)
	{
		Item.EnableShadow(FLinearColor(0.f, 0.f, 0.f, Color.A * 0.85f), FVector2D(FMath::Max(Size * 0.06f, 1.f)));
	}
	Canvas->DrawItem(Item);
}

void AZMHUD::DrawCentered(const FString& Text, float CenterX, float Y, const FLinearColor& Color, float Size, bool bShadow)
{
	DrawLabel(Text, CenterX, Y, Size, Color, 0.5f, true, bShadow);
}

void AZMHUD::DrawRightAligned(const FString& Text, float RightX, float Y, const FLinearColor& Color, float Size)
{
	DrawLabel(Text, RightX, Y, Size, Color, 1.f, true, true);
}

void AZMHUD::DrawLineThick(const FVector2D& A, const FVector2D& B, const FLinearColor& Color, float Thickness)
{
	FCanvasLineItem Line(A, B);
	Line.SetColor(Color);
	Line.LineThickness = Thickness;
	Line.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Line);
}

void AZMHUD::DrawBox(float X, float Y, float W, float H, const FLinearColor& Color)
{
	FCanvasTileItem Tile(FVector2D(X, Y), FVector2D(W, H), Color);
	Tile.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(Tile);
}

// ---------------------------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------------------------

void AZMHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!Canvas) return;
	if (!UIFont)
	{
		UIFont = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
		if (!UIFont) return;
	}

	const float Scale = Canvas->ClipY / 1080.f;
	AZMCharacter* Player = Cast<AZMCharacter>(GetOwningPawn());
	AZMGameMode* GM = GetWorld()->GetAuthGameMode<AZMGameMode>();
	if (!Player || !GM) return;

	if (GM->GetPhase() == EZMMatchPhase::GameOver)
	{
		DrawDamage(Player, Scale);
		DrawGameOver(Player, GM, Scale);
		return;
	}

	DrawDamage(Player, Scale);
	DrawCrosshair(Player, Scale);
	DrawRound(GM, Scale);
	DrawScore(Player, GM, Scale);
	DrawAmmo(Player, Scale);
	DrawPerks(Player, Scale);
	DrawPowerUps(GM, Scale);
	DrawPrompts(Player, GM, Scale);
}

// ---------------------------------------------------------------------------------------------
// Crosshair & hitmarker
// ---------------------------------------------------------------------------------------------

void AZMHUD::DrawCrosshair(AZMCharacter* Player, float Scale)
{
	const FVector2D C(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	AZMWeapon* Weapon = Player->GetCurrentWeapon();

	const float Alpha = (1.f - FMath::Clamp(Player->GetAdsAlpha() * 2.5f, 0.f, 1.f)) * (Player->IsSprinting() || Player->IsDowned() ? 0.f : 1.f);
	if (Weapon && Alpha > 0.01f)
	{
		const float FOV = PlayerOwner && PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetFOVAngle() : 80.f;
		const float HalfFovTan = FMath::Tan(FMath::DegreesToRadians(FOV * 0.5f));
		const float Gap = FMath::Tan(FMath::DegreesToRadians(Weapon->GetCurrentSpread())) / HalfFovTan * Canvas->ClipX * 0.5f + 3.f * Scale;
		const float Len = 9.f * Scale;
		const float Th = FMath::Max(2.f * Scale, 1.f);
		const FLinearColor Col(1.f, 1.f, 1.f, 0.85f * Alpha);
		const FLinearColor Shadow(0.f, 0.f, 0.f, 0.5f * Alpha);
		const FVector2D Dirs[] = { FVector2D(1, 0), FVector2D(-1, 0), FVector2D(0, 1), FVector2D(0, -1) };
		for (const FVector2D& D : Dirs)
		{
			DrawLineThick(C + D * Gap + FVector2D(1, 1), C + D * (Gap + Len) + FVector2D(1, 1), Shadow, Th);
			DrawLineThick(C + D * Gap, C + D * (Gap + Len), Col, Th);
		}
	}

	const float HitT = Player->GetHitMarkerTime();
	if (HitT > 0.f)
	{
		const float A = FMath::Clamp(HitT / 0.18f, 0.f, 1.f);
		const bool bKill = Player->WasHitMarkerKill();
		const FLinearColor Col = bKill ? FLinearColor(1.f, 0.12f, 0.08f, A) : FLinearColor(1.f, 1.f, 1.f, A);
		const float In = 7.f * Scale, Out = (bKill ? 17.f : 14.f) * Scale;
		const FVector2D Diags[] = { FVector2D(1, 1), FVector2D(-1, 1), FVector2D(1, -1), FVector2D(-1, -1) };
		for (const FVector2D& D : Diags)
		{
			const FVector2D N = D.GetSafeNormal();
			DrawLineThick(C + N * In, C + N * Out, Col, FMath::Max(2.5f * Scale, 1.f));
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Round counter: chalk tally marks for 1-5, numerals after.
// ---------------------------------------------------------------------------------------------

void AZMHUD::DrawRound(AZMGameMode* GM, float Scale)
{
	const int32 Round = GM->GetRound();
	if (Round <= 0) return;
	const float Now = GetWorld()->GetTimeSeconds();
	if (Round != LastRoundDrawn)
	{
		LastRoundDrawn = Round;
		RoundChangeTime = Now;
	}

	// Pulse white between rounds, bleed back to red as a new round begins.
	FLinearColor Col = RoundRed;
	if (GM->GetPhase() == EZMMatchPhase::BetweenRounds)
	{
		const float P = 0.5f + 0.5f * FMath::Sin(GM->GetPhaseTime() * 5.f);
		Col = FMath::Lerp(RoundRed, FLinearColor(0.95f, 0.95f, 0.95f), P);
	}
	else
	{
		const float Since = Now - RoundChangeTime;
		Col = FMath::Lerp(FLinearColor(0.95f, 0.95f, 0.95f), RoundRed, FMath::Clamp(Since / 2.5f, 0.f, 1.f));
	}

	const float Left = 60.f * Scale;
	const float Bottom = Canvas->ClipY - 50.f * Scale;
	const float Height = 105.f * Scale;

	if (Round <= 5)
	{
		const float Spacing = 26.f * Scale;
		const float Th = 9.f * Scale;
		for (int32 i = 0; i < FMath::Min(Round, 4); ++i)
		{
			// Hand-drawn look: slight lean and uneven length.
			const float X = Left + i * Spacing;
			const float Lean = ((i * 7) % 3 - 1) * 3.f * Scale;
			DrawLineThick(FVector2D(X + Lean, Bottom - Height), FVector2D(X, Bottom - (i % 2) * 4.f * Scale), Col, Th);
		}
		if (Round == 5)
		{
			DrawLineThick(FVector2D(Left - 14.f * Scale, Bottom - Height * 0.2f), FVector2D(Left + 3 * Spacing + 14.f * Scale, Bottom - Height * 0.8f), Col, Th);
		}
	}
	else
	{
		DrawLabel(FString::FromInt(Round), Left - 6.f * Scale, Bottom - Height * 1.25f, 130.f * Scale, Col, 0.f, true, true);
	}
}

// ---------------------------------------------------------------------------------------------
// Score
// ---------------------------------------------------------------------------------------------

void AZMHUD::DrawScore(AZMCharacter* Player, AZMGameMode* GM, float Scale)
{
	AZMPlayerState* PS = Player->GetPlayerState<AZMPlayerState>();
	if (!PS) return;

	const float Right = Canvas->ClipX - 60.f * Scale;
	const float Y = Canvas->ClipY * 0.63f;
	const float BoxW = 190.f * Scale, BoxH = 46.f * Scale;
	DrawBox(Right - BoxW, Y, BoxW, BoxH, FLinearColor(0.f, 0.f, 0.f, 0.35f));
	DrawBox(Right - BoxW, Y, 5.f * Scale, BoxH, FLinearColor(0.85f, 0.7f, 0.25f, 0.9f));
	DrawRightAligned(FString::FromInt(PS->GetPoints()), Right - 12.f * Scale, Y + 6.f * Scale, ScoreColor, 34.f * Scale);

	// "+10" / "-1000" popups drift away from the score box.
	const float Now = GetWorld()->GetTimeSeconds();
	PS->Popups.RemoveAll([Now](const AZMPlayerState::FPopup& P) { return Now - P.Time > PopupLife; });
	for (const AZMPlayerState::FPopup& P : PS->Popups)
	{
		const float Age = Now - P.Time;
		const float A = 1.f - Age / PopupLife;
		const bool bGain = P.Amount > 0;
		const FLinearColor Col = bGain ? FLinearColor(1.f, 0.85f, 0.2f, A) : FLinearColor(0.95f, 0.15f, 0.1f, A);
		const float X = Right - BoxW - 10.f * Scale - Age * 110.f * Scale;
		const float PY = Y + 8.f * Scale + P.Offset * Age * 45.f * Scale + (bGain ? 0.f : Age * 40.f * Scale);
		DrawRightAligned(FString::Printf(TEXT("%s%d"), bGain ? TEXT("+") : TEXT(""), P.Amount), X, PY, Col, 24.f * Scale);
	}
}

// ---------------------------------------------------------------------------------------------
// Ammo
// ---------------------------------------------------------------------------------------------

void AZMHUD::DrawAmmo(AZMCharacter* Player, float Scale)
{
	AZMWeapon* W = Player->GetCurrentWeapon();
	if (!W) return;
	const FZMWeaponStats& S = W->GetStats();
	const float Right = Canvas->ClipX - 60.f * Scale;
	const float Bottom = Canvas->ClipY - 50.f * Scale;

	const bool bLow = W->GetMag() <= FMath::Max(S.MagSize / 4, 1);
	const FLinearColor MagCol = W->GetMag() == 0 ? FLinearColor(0.95f, 0.12f, 0.08f) : (bLow ? FLinearColor(1.f, 0.6f, 0.2f) : TextWhite);
	const FString Reserve = FString::Printf(TEXT(" / %d"), W->GetReserve());

	const float ReserveSize = 30.f * Scale;
	const float MagSize = 54.f * Scale;
	const FVector2D ReserveDim = MeasureLabel(Reserve, MakeFont(ReserveSize, true));
	DrawRightAligned(Reserve, Right, Bottom - 42.f * Scale, FLinearColor(0.7f, 0.7f, 0.68f), ReserveSize);
	DrawRightAligned(FString::FromInt(W->GetMag()), Right - ReserveDim.X, Bottom - 66.f * Scale, MagCol, MagSize);
	DrawRightAligned(S.DisplayName.ToString(), Right, Bottom - 100.f * Scale, FLinearColor(0.8f, 0.8f, 0.78f, 0.9f), 22.f * Scale);

	// Second weapon, dimmed.
	for (AZMWeapon* Other : Player->GetWeapons())
	{
		if (Other && Other != W)
		{
			DrawRightAligned(Other->GetStats().DisplayName.ToString(), Right, Bottom - 128.f * Scale, FLinearColor(0.6f, 0.6f, 0.6f, 0.5f), 18.f * Scale);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Perks & power-ups
// ---------------------------------------------------------------------------------------------

void AZMHUD::DrawPerks(AZMCharacter* Player, float Scale)
{
	const float Size = 38.f * Scale;
	float X = 60.f * Scale;
	const float Y = Canvas->ClipY - 230.f * Scale;
	for (EZMPerk Perk : Player->GetPerks())
	{
		const FLinearColor C = ZM::PerkColor(Perk);
		DrawBox(X - 2.f * Scale, Y - 2.f * Scale, Size + 4.f * Scale, Size + 4.f * Scale, FLinearColor(0.f, 0.f, 0.f, 0.6f));
		DrawBox(X, Y, Size, Size, FLinearColor(C.R, C.G, C.B, 0.9f));
		const FString Initial = ZM::PerkName(Perk).ToString().Left(1);
		DrawCentered(Initial, X + Size * 0.5f, Y + Size * 0.12f, FLinearColor::White, 26.f * Scale);
		X += Size + 10.f * Scale;
	}
}

void AZMHUD::DrawPowerUps(AZMGameMode* GM, float Scale)
{
	struct FActive { EZMPowerUp Type; float Time; };
	TArray<FActive> Active;
	if (GM->IsInstaKill()) Active.Add({ EZMPowerUp::InstaKill, GM->GetInstaKillTime() });
	if (GM->IsDoublePoints()) Active.Add({ EZMPowerUp::DoublePoints, GM->GetDoublePointsTime() });

	const float Size = 54.f * Scale;
	const float Spacing = 74.f * Scale;
	float X = Canvas->ClipX * 0.5f - (Active.Num() - 1) * Spacing * 0.5f;
	const float Y = Canvas->ClipY - 120.f * Scale;
	const float Now = GetWorld()->GetTimeSeconds();
	for (const FActive& A : Active)
	{
		// Blink when about to expire.
		if (A.Time < 5.f && FMath::Fmod(Now, 0.4f) < 0.15f) { X += Spacing; continue; }
		const FLinearColor C = ZM::PowerUpColor(A.Type);
		DrawBox(X - Size * 0.5f, Y, Size, Size, FLinearColor(C.R * 0.4f, C.G * 0.4f, C.B * 0.4f, 0.75f));
		DrawBox(X - Size * 0.5f, Y + Size - 5.f * Scale, Size * (A.Time / 30.f), 5.f * Scale, FLinearColor(C.R, C.G, C.B, 0.95f));
		const TCHAR* Short = A.Type == EZMPowerUp::InstaKill ? TEXT("IK") : TEXT("x2");
		DrawCentered(Short, X, Y + 8.f * Scale, FLinearColor::White, 30.f * Scale);
		X += Spacing;
	}

	// Big announcement ("MAX AMMO!").
	if (GM->GetAnnouncementTime() > 0.f && !GM->GetAnnouncement().IsEmpty())
	{
		const float T = GM->GetAnnouncementTime();
		FLinearColor C = GM->GetAnnouncementColor();
		C.A = FMath::Clamp(T / 0.5f, 0.f, 1.f);
		DrawCentered(GM->GetAnnouncement().ToString(), Canvas->ClipX * 0.5f, Canvas->ClipY * 0.22f, C, 52.f * Scale);
	}
}

// ---------------------------------------------------------------------------------------------
// Damage feedback
// ---------------------------------------------------------------------------------------------

void AZMHUD::DrawDamage(AZMCharacter* Player, float Scale)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;

	// Low-health blood vignette: stacked translucent bands around the edges.
	const float Missing = 1.f - Player->GetHealth() / FMath::Max(Player->GetMaxHealth(), 1.f);
	const float Vig = FMath::Clamp(FMath::Pow(Missing, 1.4f) * 0.75f + Player->GetDamageFlash() * 0.25f, 0.f, 0.85f);
	if (Vig > 0.01f)
	{
		const int32 Bands = 10;
		for (int32 i = 0; i < Bands; ++i)
		{
			const float T = float(i) / Bands;
			const float Inset = T * 0.2f;
			const float A = Vig * (1.f - T) * 0.22f;
			const FLinearColor C(0.35f, 0.f, 0.f, A);
			const float BW = W * 0.02f, BH = H * 0.02f;
			DrawBox(W * Inset, H * Inset, W * (1.f - 2.f * Inset), BH, C);
			DrawBox(W * Inset, H * (1.f - Inset) - BH, W * (1.f - 2.f * Inset), BH, C);
			DrawBox(W * Inset, H * Inset + BH, BW, H * (1.f - 2.f * Inset) - 2.f * BH, C);
			DrawBox(W * (1.f - Inset) - BW, H * Inset + BH, BW, H * (1.f - 2.f * Inset) - 2.f * BH, C);
		}
	}

	// Direction of the last hit.
	if (Player->GetDamageFlash() > 0.01f && !Player->GetLastDamageDirection().IsNearlyZero())
	{
		const FRotator View = PlayerOwner ? PlayerOwner->GetControlRotation() : FRotator::ZeroRotator;
		const FVector Local = FRotator(0.f, View.Yaw, 0.f).UnrotateVector(Player->GetLastDamageDirection());
		const float Angle = FMath::Atan2(Local.Y, Local.X); // 0 = in front, + = right
		const FVector2D C(W * 0.5f, H * 0.5f);
		const float R = 150.f * Scale;
		const FLinearColor Col(0.85f, 0.05f, 0.02f, Player->GetDamageFlash() * 0.9f);
		for (int32 i = -3; i < 3; ++i)
		{
			const float A0 = Angle + i * 0.09f, A1 = Angle + (i + 1) * 0.09f;
			DrawLineThick(C + FVector2D(FMath::Sin(A0), -FMath::Cos(A0)) * R, C + FVector2D(FMath::Sin(A1), -FMath::Cos(A1)) * R, Col, 8.f * Scale);
		}
	}

	if (Player->IsDowned())
	{
		DrawBox(0.f, 0.f, W, H, FLinearColor(0.25f, 0.f, 0.f, 0.35f));
		if (Player->HasPerk(EZMPerk::SecondWind))
		{
			const float P = FMath::Clamp(Player->GetDownedTime() / 6.f, 0.f, 1.f);
			const float BW = 320.f * Scale, BH = 12.f * Scale;
			DrawCentered(LOCTEXT("SecondWind", "SECOND WIND").ToString(), W * 0.5f, H * 0.58f, FLinearColor(0.3f, 0.6f, 1.f), 30.f * Scale);
			DrawBox(W * 0.5f - BW * 0.5f, H * 0.58f + 44.f * Scale, BW, BH, FLinearColor(0.f, 0.f, 0.f, 0.6f));
			DrawBox(W * 0.5f - BW * 0.5f, H * 0.58f + 44.f * Scale, BW * P, BH, FLinearColor(0.3f, 0.6f, 1.f, 0.9f));
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Prompts
// ---------------------------------------------------------------------------------------------

void AZMHUD::DrawPrompts(AZMCharacter* Player, AZMGameMode* GM, float Scale)
{
	const float CX = Canvas->ClipX * 0.5f;
	const float H = Canvas->ClipY;

	if (AZMInteractable* I = Player->GetFocusedInteractable())
	{
		const FText Prompt = I->GetPrompt(Player);
		if (!Prompt.IsEmpty())
		{
			DrawCentered(Prompt.ToString(), CX, H * 0.64f, TextWhite, 26.f * Scale);
		}
	}

	if (AZMWeapon* W = Player->GetCurrentWeapon())
	{
		if (W->GetMag() == 0 && W->GetReserve() == 0)
		{
			DrawCentered(LOCTEXT("NoAmmo", "No Ammo").ToString(), CX, H * 0.57f, FLinearColor(0.95f, 0.15f, 0.1f), 24.f * Scale);
		}
		else if (!W->IsReloading() && W->GetReserve() > 0 && W->GetMag() <= FMath::Max(W->GetStats().MagSize / 4, 1))
		{
			DrawCentered(LOCTEXT("Reload", "Press R to reload").ToString(), CX, H * 0.57f, TextWhite, 24.f * Scale);
		}
	}

	if (Player->GetSensitivityToastTime() > 0.f)
	{
		const FLinearColor C(1.f, 1.f, 1.f, FMath::Clamp(Player->GetSensitivityToastTime(), 0.f, 1.f));
		DrawCentered(FString::Printf(TEXT("Sensitivity %.2f"), Player->GetLookSensitivity()), CX, H * 0.08f, C, 24.f * Scale);
	}

	if (GM->GetPhase() == EZMMatchPhase::Intro)
	{
		const float A = FMath::Clamp(3.f - GM->GetPhaseTime(), 0.f, 1.f);
		DrawCentered(TEXT("OUTPOST"), CX, H * 0.3f, FLinearColor(0.85f, 0.8f, 0.7f, A), 56.f * Scale);
		DrawCentered(LOCTEXT("Survive", "Survive as long as you can").ToString(), CX, H * 0.3f + 70.f * Scale, FLinearColor(0.7f, 0.7f, 0.7f, A), 24.f * Scale);
	}
}

void AZMHUD::DrawGameOver(AZMCharacter* Player, AZMGameMode* GM, float Scale)
{
	const float W = Canvas->ClipX, H = Canvas->ClipY;
	const float T = GM->GetPhaseTime();
	DrawBox(0.f, 0.f, W, H, FLinearColor(0.f, 0.f, 0.f, FMath::Clamp(T / 2.f, 0.f, 0.75f)));

	const float A = FMath::Clamp(T / 1.f, 0.f, 1.f);
	DrawCentered(LOCTEXT("GameOverTitle", "GAME OVER").ToString(), W * 0.5f, H * 0.3f, FLinearColor(0.75f, 0.03f, 0.02f, A), 84.f * Scale);

	const int32 Rounds = GM->GetRound();
	const FString Survived = FText::Format(LOCTEXT("Survived", "You survived {0} {0}|plural(one=round,other=rounds)"), Rounds).ToString();
	DrawCentered(Survived, W * 0.5f, H * 0.3f + 110.f * Scale, FLinearColor(0.9f, 0.9f, 0.88f, A), 34.f * Scale);

	if (const AZMPlayerState* PS = Player->GetPlayerState<AZMPlayerState>())
	{
		const FString Stats = FString::Printf(TEXT("Points %d     Kills %d     Headshots %d"), PS->GetPoints(), PS->Kills, PS->Headshots);
		DrawCentered(Stats, W * 0.5f, H * 0.3f + 165.f * Scale, FLinearColor(0.7f, 0.7f, 0.68f, A), 24.f * Scale);
	}
	if (T > 2.f)
	{
		const float Pulse = 0.6f + 0.4f * FMath::Sin(T * 3.f);
		DrawCentered(LOCTEXT("Restart", "Press F to play again").ToString(), W * 0.5f, H * 0.72f, FLinearColor(1.f, 1.f, 1.f, Pulse), 26.f * Scale);
	}
}

#undef LOCTEXT_NAMESPACE
