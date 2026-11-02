"""Run in the Unreal Editor after compiling RhythmBattle and dancer."""
import unreal

TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
LIB = unreal.EditorAssetLibrary

def native(path):
    cls = unreal.load_class(None, path)
    if not cls:
        raise RuntimeError("Missing native class: " + path)
    return cls

def create(name, folder, asset_class, factory):
    path = folder + "/" + name
    if LIB.does_asset_exist(path):
        existing = LIB.load_asset(path)
        unreal.log("Rhythm asset exists: " + path)
        return existing, False
    asset = TOOLS.create_asset(name, folder, asset_class, factory)
    if not asset:
        raise RuntimeError("Failed to create " + path)
    unreal.log("Created rhythm asset: " + path)
    return asset, True

def blueprint(name, folder, parent, widget=False):
    factory = unreal.WidgetBlueprintFactory() if widget else unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    cls = unreal.WidgetBlueprint if widget else unreal.Blueprint
    asset, created = create(name, folder, cls, factory)
    if created:
        unreal.BlueprintEditorLibrary.compile_blueprint(asset)
    return asset, created

def generated(asset):
    cls = unreal.load_class(None, asset.get_path_name() + "_C")
    if not cls:
        raise RuntimeError("No generated class: " + asset.get_path_name())
    return cls

def save(asset):
    if not LIB.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError("Could not save " + asset.get_path_name())
    unreal.log("Saved rhythm asset: " + asset.get_path_name())

# 1. UI
widget, made = blueprint("WBP_RhythmBattle", "/Game/Rhythm/UI", native("/Script/RhythmBattle.RhythmBattleWidget"), True)
if widget:
    save(widget)

# 2. Modifiers
modifier_parent = native("/Script/RhythmBattle.RhythmModifierEventAction")
for name, kind, value, beats in (("BP_SpeedBoost", unreal.RhythmModifierType.VISUAL_SPEED, 1.35, 8.0),
                                  ("BP_ScoreBoost", unreal.RhythmModifierType.SCORE, 2.0, 4.0)):
    asset, made = blueprint(name, "/Game/Rhythm/Events", modifier_parent)
    if asset:
        default = unreal.get_default_object(generated(asset))
        spec = default.get_editor_property("modifier")
        spec.set_editor_property("id", "SpeedBoost" if name == "BP_SpeedBoost" else "ScoreBoost")
        spec.set_editor_property("scope", unreal.RhythmModifierScope.BATTLE if name == "BP_SpeedBoost" else unreal.RhythmModifierScope.PLAYER)
        spec.set_editor_property("player_index", 0)
        spec.set_editor_property("type", kind)
        spec.set_editor_property("value", value)
        spec.set_editor_property("duration_seconds", beats)
        spec.set_editor_property("duration_in_beats", True)
        default.set_editor_property("modifier", spec)
        save(asset)

# 3. Taunt
taunt, made = blueprint("BP_Taunt", "/Game/Rhythm/Events", native("/Script/RhythmBattle.RhythmPauseEventAction"))
if taunt:
    save(taunt)

# 4. Input Mapping
context, context_made = create("IMC_Rhythm", "/Game/Rhythm/Input", unreal.InputMappingContext, unreal.InputMappingContext_Factory())
# Always ensure clean state for IMC_Rhythm
if context:
    context.unmap_all()

# Mapping definitions: (ActionName, Key1, Key2, PlayerIndex, LaneId)
# 6 lanes total: 3 left (P0: W/A/S) + 3 right (P1: arrows)
input_configs = [
    ("IA_Rhythm_P0L0", "W", "Gamepad_FaceButton_Left", 0, 0),
    ("IA_Rhythm_P0L1", "A", "Gamepad_FaceButton_Bottom", 0, 1),
    ("IA_Rhythm_P0L2", "S", "Gamepad_FaceButton_Right", 0, 2),
    ("IA_Rhythm_P1L0", "Up", "Gamepad_LeftShoulder", 1, 0),
    ("IA_Rhythm_P1L1", "Right", "Gamepad_RightShoulder", 1, 1),
    ("IA_Rhythm_P1L2", "Down", "Gamepad_RightTrigger", 1, 2),
]
actions = {}

for name, k1, k2, p_idx, l_id in input_configs:
    action, action_made = create(name, "/Game/Rhythm/Input", unreal.InputAction, unreal.InputAction_Factory())
    if action:
        actions[(p_idx, l_id)] = action
        action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
        save(action)
        
        if context:
            for k_str in (k1, k2):
                key = unreal.Key()
                key.set_editor_property("key_name", unreal.Name(k_str))
                context.map_key(action, key)

if context:
    save(context)

# Update the checked-in demo chart as well as the runtime fallback chart.
chart = LIB.load_asset("/Game/Rhythm/Charts/DA_RhythmDemo")
if chart:
    lanes = []
    colors = ((1.0, 0.4, 0.65), (0.3, 1.0, 0.9), (0.4, 1.0, 0.5))
    for player in range(2):
        for lane_id in range(3):
            y = (0.54 if player == 0 else 0.60) + lane_id * 0.14
            lane = unreal.RhythmLaneLayout()
            lane.set_editor_property("player_index", player)
            lane.set_editor_property("lane_id", lane_id)
            lane.set_editor_property("start", unreal.Vector2D(0.06 if player == 0 else 0.94, y))
            lane.set_editor_property("hit_target", unreal.Vector2D(0.30 if player == 0 else 0.70, y))
            lane.set_editor_property("color", unreal.LinearColor(*colors[lane_id], 1.0))
            lane.set_editor_property("input_action", actions[(player, lane_id)])
            lanes.append(lane)
    notes = []
    for beat in range(4, 58):
        for player in range(2):
            note = unreal.RhythmNoteDefinition()
            note.set_editor_property("player_index", player)
            note.set_editor_property("lane_id", (beat + player) % 3)
            note.set_editor_property("beat", float(beat))
            notes.append(note)
    chart.set_editor_property("lanes", lanes)
    chart.set_editor_property("notes", notes)
    save(chart)

# 5. Player Controller
controller, made = blueprint("BP_RhythmPlayerController", "/Game/Rhythm", native("/Script/RhythmBattle.RhythmDemoPlayerController"))
if controller:
    default = unreal.get_default_object(generated(controller))
    default.set_editor_property("authored_mapping", context)
    default.set_editor_property("hud_class", generated(widget))
    save(controller)

# 6. Game Mode
mode, made = blueprint("BP_RhythmArenaGameMode", "/Game/Rhythm", native("/Script/dancer.DanceRhythmArenaGameMode"))
if mode:
    default = unreal.get_default_object(generated(mode))
    default.set_editor_property("player_controller_class", generated(controller))
    default.set_editor_property("demo_chart", chart)
    save(mode)

# Make the existing arena playable without a manual World Settings edit.
for arena_path in ("/Game/DanceBattle/Maps/L_DanceBattle", "/Game/Rhythm/Maps/L_RhythmDemo"):
    if not LIB.does_asset_exist(arena_path):
        raise RuntimeError("Missing arena map: " + arena_path)
    if not unreal.EditorLevelLibrary.load_level(arena_path):
        raise RuntimeError("Could not load arena map: " + arena_path)
    world = unreal.EditorLevelLibrary.get_editor_world()
    world.get_world_settings().set_editor_property("game_mode_override", generated(mode))
    if not unreal.EditorLevelLibrary.save_current_level():
        raise RuntimeError("Could not save arena map: " + arena_path)
    unreal.log("Configured rhythm game mode on " + arena_path)

unreal.log("Rhythm editor asset creation complete")
