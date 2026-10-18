# EAFPS — round-based zombies FPS (Unreal Engine 5.x, C++)

Goal: a classic COD-Zombies-style base game (feel, pacing, HUD, economy) that we later make unique.
Original names/art only — no COD trademarks, logos, ripped assets or sounds.

## Machine
Windows, Ryzen 7800X3D, RTX 4080 Super, 32 GB RAM, Visual Studio Community 2026 (MSVC 14.51), Python 3.15.
Engine: UE 5.8.3 at `E:\UE_5.8` (registered under HKCU\Software\Epic Games\Unreal Engine\Builds; not in Program Files).
Repo: `C:\Dev\EAFPS`. Git has no GitHub credentials on this PC yet — pushing needs an interactive sign-in.

## Build / run (keep output filtered — UE logs are huge, only read errors)
- Build editor target (fails while any editor with Live Coding is open — close it first):
  `"E:\UE_5.8\Engine\Build\BatchFiles\Build.bat" EAFPSEditor Win64 Development -Project="C:\Dev\EAFPS\EAFPS.uproject" -WaitMutex`
  Filter to lines containing `C:\Dev\EAFPS` plus `Result:` — engine headers emit hundreds of C4996 deprecation warnings.
- Run game without the editor UI: `"E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" C:\Dev\EAFPS\EAFPS.uproject -game -windowed -ResX=1600 -ResY=900 -log`
- Logs: `Saved\Logs\EAFPS.log`. Grep for `Error`, `EAFPS`, `LogScript`. One `LogCrowdFollowing: Unable to find
  RecastNavMesh` warning at startup is benign (the crowd manager picks the navmesh up when it registers).
- Setup (idempotent, headless): `UnrealEditor.exe C:\Dev\EAFPS\EAFPS.uproject -ExecutePythonScript="C:/Dev/EAFPS/Scripts/Setup.py" -ZMSetupQuit`
  After regenerating sounds with `python Tools/gen_sfx.py`, add `-ZMReimportAudio`.
- Playtest automation (non-shipping, append to the -game command line):
  `-ZMShots=5,20,40` screenshots with HUD to `Saved\Screenshots\WindowsEditor\ZM_<t>.png` · `-ZMQuitAt=45` ·
  `-ZMBot` aim/fire/reload bot · `-ZMGod` · `-ZMRound=9` · `-ZMPoints=20000` ·
  `-ZMView=x,y,z,pitch,yaw/...` camera per screenshot · `-ZMSelfTest` scripted doors/wall buy/perk/box/power-ups/
  game over/restart (logs `EAFPS TEST:` lines) · `-ExecCmds=DisableAllScreenMessages` hides "Preparing shaders".

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

## Status
Done: all classes compile on 5.8 with no warnings from our code; Setup.py and Tools/gen_sfx.py (38 sounds, stdlib only)
work; the greybox "Outpost" arena plays end to end (rounds 1-9 checked; -ZMSelfTest passes every step).
Content: `Content/Maps` and `Content/Audio` are committed. `Content/Characters` (template mannequin, ~126 MB) is
gitignored and recreated by Setup.py from `E:\UE_5.8\Templates\TemplateResources\High\Characters`.
Next, in order:
1. Art pass with CC0 assets (Poly Haven textures/HDRIs, Quaternius zombies + anims, Kenney props). Zombies are currently
   the tinted mannequin with ABP_Unarmed locomotion; MM_Attack_01 is the swing, played single-node (the ABP has no slot).
2. Greybox weapons are cube models; the viewmodel needs real meshes/anims (ADS sight alignment derives from BodySize).
3. Mystery box needs a "?" visual and beam; no barricades/windows yet (zombies rise from dirt patches).
4. Critique round against reference footage once art is in.

## Conventions
- Match existing style: tabs, `ZM`/`AZM` prefixes, short comments only where non-obvious.
- Every asset reference must degrade gracefully when missing.
- Critique rounds happen at milestones, with a concrete checklist, to save usage.
