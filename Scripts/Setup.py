"""One-time project setup. Run inside the Unreal Editor (Tools > Execute Python Script), or headless:

    UnrealEditor.exe C:/Dev/EAFPS/EAFPS.uproject -ExecutePythonScript="C:/Dev/EAFPS/Scripts/Setup.py" -ZMSetupQuit

Idempotent; safe to re-run. Steps:
  1. Copy the engine's template mannequin into /Game/Characters/Mannequins.
  2. Write the zombie mesh / anim blueprint paths into Config/DefaultGame.ini.
  3. Import SourceAssets/Audio/*.wav into /Game/Audio (generating them first if missing).
  4. Create /Game/Maps/Zombies_Arena: empty level + large NavMeshBoundsVolume + PlayerStart.
"""

import os
import re
import shutil
import sys

import unreal

PROJECT_DIR = os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
ENGINE_DIR = os.path.normpath(unreal.Paths.convert_relative_path_to_full(unreal.Paths.engine_dir()))
CONTENT_DIR = os.path.join(PROJECT_DIR, "Content")
AUDIO_SRC = os.path.join(PROJECT_DIR, "SourceAssets", "Audio")
MAP_PATH = "/Game/Maps/Zombies_Arena"


def log(msg):
    unreal.log(f"[ZMSetup] {msg}")


def warn(msg):
    unreal.log_warning(f"[ZMSetup] {msg}")


# ---------------------------------------------------------------------------
# 1. Mannequin
# ---------------------------------------------------------------------------

def copy_mannequin():
    dest = os.path.join(CONTENT_DIR, "Characters", "Mannequins")
    if os.path.isdir(dest):
        log("Mannequin already present")
    else:
        candidates = [
            os.path.join(ENGINE_DIR, "..", "Templates", "TemplateResources", "High", "Characters", "Content", "Mannequins"),
            os.path.join(ENGINE_DIR, "..", "Templates", "TP_ThirdPerson", "Content", "Characters", "Mannequins"),
            os.path.join(ENGINE_DIR, "..", "Templates", "TP_ThirdPersonBP", "Content", "Characters", "Mannequins"),
        ]
        src = next((os.path.normpath(c) for c in candidates if os.path.isdir(c)), None)
        if not src:
            warn("No template mannequin found in the engine; zombies will use the greybox body")
            return
        log(f"Copying mannequin from {src}")
        shutil.copytree(src, dest)
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous(["/Game/Characters"], True)


def find_asset(class_name, name_patterns, root="/Game/Characters"):
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = registry.get_assets_by_path(root, recursive=True)
    found = [a for a in assets if str(a.asset_class_path.asset_name) == class_name]
    for pattern in name_patterns:
        for a in found:
            if re.fullmatch(pattern, str(a.asset_name)):
                return a
    return None


def configure_zombie_assets():
    mesh = find_asset("SkeletalMesh", [r"SKM_Manny", r"SKM_Manny_Simple", r"SKM_Manny.*", r"SK_Mannequin"])
    abp = find_asset("AnimBlueprint", [r"ABP_Unarmed", r"ABP_Manny", r"ABP_.*"])
    mesh_path = f"{mesh.package_name}.{mesh.asset_name}" if mesh else ""
    abp_path = f"{abp.package_name}.{abp.asset_name}_C" if abp else ""
    attack = find_asset("AnimSequence", [r"MM_Attack_01", r"MM_Attack_.*"])
    attack_path = f"{attack.package_name}.{attack.asset_name}" if attack else ""
    log(f"Zombie mesh: {mesh_path or '(none, greybox)'}")
    log(f"Zombie anim: {abp_path or '(none)'}")

    ini = os.path.join(PROJECT_DIR, "Config", "DefaultGame.ini")
    with open(ini, "r", encoding="utf-8") as f:
        text = f.read()
    section = "[/Script/EAFPS.ZMSettings]"
    if section not in text:
        text = text.rstrip() + f"\n\n{section}\nZombieMesh=\nZombieAnimClass=\n"

    def set_key(src, key, value):
        pattern = re.compile(rf"^{key}=.*$", re.MULTILINE)
        if pattern.search(src):
            return pattern.sub(f"{key}={value}", src)
        return src.replace(section, f"{section}\n{key}={value}", 1)

    text = set_key(text, "ZombieMesh", mesh_path)
    text = set_key(text, "ZombieAnimClass", abp_path)
    text = set_key(text, "ZombieAttackAnim", attack_path)
    with open(ini, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    log("Updated Config/DefaultGame.ini")


# ---------------------------------------------------------------------------
# 2. Audio
# ---------------------------------------------------------------------------

def import_audio():
    wavs = sorted(f for f in os.listdir(AUDIO_SRC) if f.lower().endswith(".wav")) if os.path.isdir(AUDIO_SRC) else []
    if not wavs:
        log("No WAVs found, running Tools/gen_sfx.py")
        sys.path.insert(0, os.path.join(PROJECT_DIR, "Tools"))
        import gen_sfx
        gen_sfx.build()
        wavs = sorted(f for f in os.listdir(AUDIO_SRC) if f.lower().endswith(".wav"))

    tasks = []
    for name in wavs:
        task = unreal.AssetImportTask()
        task.filename = os.path.join(AUDIO_SRC, name)
        task.destination_path = "/Game/Audio"
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    imported = sum(1 for t in tasks if t.imported_object_paths)
    log(f"Imported {imported}/{len(tasks)} sounds into /Game/Audio")


# ---------------------------------------------------------------------------
# 3. Map
# ---------------------------------------------------------------------------

def create_map():
    level_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

    if unreal.EditorAssetLibrary.does_asset_exist(MAP_PATH):
        log("Map exists, loading it to refresh its actors")
        level_subsystem.load_level(MAP_PATH)
    else:
        log(f"Creating {MAP_PATH}")
        level_subsystem.new_level(MAP_PATH)

    for actor in actor_subsystem.get_all_level_actors():
        if isinstance(actor, (unreal.NavMeshBoundsVolume, unreal.PlayerStart)):
            actor_subsystem.destroy_actor(actor)

    # Spawning from the class object goes through the box-volume actor factory, which builds the brush.
    nav = actor_subsystem.spawn_actor_from_object(unreal.NavMeshBoundsVolume.static_class(), unreal.Vector(900, 900, 200))
    if nav:
        # Default brush is 200uu; cover the whole arena (X/Y roughly -1000..2800) with margin.
        nav.set_actor_scale3d(unreal.Vector(30.0, 30.0, 6.0))
        origin, extent = nav.get_actor_bounds(False)
        log(f"NavMeshBoundsVolume bounds extent: {extent}")
        if extent.x < 100:
            warn("NavMeshBoundsVolume has no brush; add one manually in the editor (Place Actors > Volumes)")
    else:
        warn("Failed to spawn NavMeshBoundsVolume")

    actor_subsystem.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 120), unreal.Rotator(0, 0, 0))

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    settings = world.get_world_settings()
    settings.set_editor_property("force_no_precomputed_lighting", True)

    # Build once so the RecastNavMesh actor is saved in the map (the crowd manager needs it at startup).
    # The arena itself is spawned at runtime, so runtime generation rebuilds the tiles.
    unreal.SystemLibrary.execute_console_command(world, "RebuildNavigation")
    level_subsystem.save_current_level()
    has_nav = any(isinstance(a, unreal.RecastNavMesh) for a in actor_subsystem.get_all_level_actors())
    log(f"Map saved (RecastNavMesh present: {has_nav})")


def main():
    log(f"Project: {PROJECT_DIR}")
    steps = [copy_mannequin, configure_zombie_assets, import_audio, create_map]
    for step in steps:
        try:
            step()
        except Exception as e:  # keep going so one failure doesn't block the rest
            unreal.log_error(f"[ZMSetup] {step.__name__} failed: {e}")
    unreal.get_editor_subsystem(unreal.EditorAssetSubsystem).save_directory("/Game", only_if_is_dirty=True, recursive=True)
    log("Done")
    if "-ZMSetupQuit" in unreal.SystemLibrary.get_command_line():
        unreal.SystemLibrary.quit_editor()


main()
