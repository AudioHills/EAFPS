# EAFPS — round-based zombies FPS (Unreal Engine 5.x, C++)

Goal: a classic COD-Zombies-style base game (feel, pacing, HUD, economy) that we later make unique.
Original names/art only — no COD trademarks, logos, ripped assets or sounds.

## Machine
Windows, Ryzen 7800X3D, RTX 4080 Super, 32 GB RAM, VS 2022. The installed engine is newer than 5.5 (5.8.3).
First step on the PC: find the installed engine folder under `C:\Program Files\Epic Games\`, `EngineAssociation`
in EAFPS.uproject is already "5.8"; fix any API deprecations the newer engine reports.
Repo should live at a short path (e.g. `C:\Dev\EAFPS`), not OneDrive.

## Build / run (keep output filtered — UE logs are huge, only read errors)
- Build editor target:
  `"<UE>\Engine\Build\BatchFiles\Build.bat" EAFPSEditor Win64 Development -Project="C:\Dev\EAFPS\EAFPS.uproject" -WaitMutex 2>&1 | findstr /R /C:"error" /C:"warning C" /C:"Result:"`
  (`<UE>` is the installed engine folder, e.g. `C:\Program Files\Epic Games\UE_5.8`)
- Run game without the editor UI: `"<UE>\Engine\Binaries\Win64\UnrealEditor.exe" C:\Dev\EAFPS\EAFPS.uproject -game -windowed -ResX=1600 -ResY=900 -log`
- Logs: `Saved\Logs\EAFPS.log`. Grep for `Error`, `EAFPS`, `LogScript`.
- Screenshots for critique: console command `HighResShot 1920x1080`, written to `Saved\Screenshots\`.

## Architecture (Source/EAFPS)
Everything works with zero imported assets (engine basic shapes + BasicShapeMaterial "Color" param),
then upgrades when real assets are hooked up via `UZMSettings` (Project Settings > Game > Zombies, DefaultGame.ini).
- `ZMTypes` — weapon catalog (all stats), round formulas (count/health/spawn delay), perk/power-up data,
  helpers: `ZM::LoadSound(Name)` loads `/Game/Audio/<Name>` (nullptr if missing), `PlayWorldSound`, `MakeTintedMaterial`.
- `ZMWeapon` — hitscan, spread/bloom, recoil, falloff, pellets, reload (tactical/empty), greybox model built from cubes.
- `ZMCharacter` — Enhanced Input created at runtime (no input assets), sprint/slide/crouch, ADS with FOV-matched
  sensitivity, viewmodel sway/bob/kick/reload/switch/melee poses, health regen, perks, downed/Second Wind.
- `ZMZombie` + `ZMZombieAIController` — crowd-following navmesh chase, gaits by round, melee windup, headshots by bone
  (`head`/`neck`), ragdoll; greybox body if no mannequin.
- `ZMGameMode` — rounds, spawning (max 24 alive), zones, points, power-ups, game over/restart.
- `ZMPlayerState` — points + popups. `ZMHUD` — canvas HUD (tally-mark rounds, score popups, ammo, crosshair, hitmarkers).
- `ZMInteractable` / `ZMBuyables` — wall buys, doors (unlock zones), perk machines, mystery box. `ZMPowerUp` — drops.
- `ZMArenaBuilder` — builds the default greybox map + lighting/fog/post at runtime if the level has no spawn points,
  so an empty level with a NavMeshBoundsVolume is enough to play.

## Status (handoff from cloud session — nothing has been compiled yet)
Written: uproject, Config/*, Build/Target files, ZMTypes, ZMSettings, ZMTransientFX, ZMWeapon, ZMCharacter, all headers.
TODO, in order:
1. Write missing .cpp: ZMZombie, ZMZombieAIController, ZMGameMode, ZMPlayerState, ZMHUD, ZMPlayerController,
   ZMInteractable, ZMBuyables, ZMPowerUp, ZMSpawnPoint, ZMArenaBuilder.
2. Compile and fix errors.
3. `Scripts/Setup.py` (run in editor): create `/Game/Maps/Zombies_Arena` (empty + large NavMeshBoundsVolume + PlayerStart),
   import `SourceAssets/Audio/*.wav` to `/Game/Audio`, find mannequin mesh/ABP and write paths into DefaultGame.ini.
   Mannequin comes from the Third Person template content (Engine `Templates/TP_ThirdPerson`), copy it into Content.
4. `Tools/gen_sfx.py` — synthesize placeholder WAVs (names referenced in code: SFX_*_Fire, SFX_Reload_Out/In,
   SFX_Dry_Fire, SFX_Hit_Marker/Kill/Head, SFX_Impact, SFX_Knife_Swing/Hit, SFX_Player_Hurt/Down, SFX_Slide, SFX_Land,
   SFX_Weapon_Raise, SFX_Low_Ammo, SFX_Zombie_Groan/Attack/Death, SFX_Round_Start/End, SFX_Purchase, SFX_PowerUp_*, SFX_Game_Over).
5. Playtest loop: launch, screenshot, fix. Then art pass with CC0 assets (Poly Haven textures/HDRIs, Quaternius zombies,
   Kenney), then critique rounds against reference footage.

## Conventions
- Match existing style: tabs, `ZM`/`AZM` prefixes, short comments only where non-obvious.
- Every asset reference must degrade gracefully when missing.
- Critique rounds happen at milestones, with a concrete checklist, to save usage.
