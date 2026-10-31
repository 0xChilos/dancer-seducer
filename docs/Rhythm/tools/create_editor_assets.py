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
input_configs = [
    ("IA_Rhythm_P0L0", "A", "Gamepad_FaceButton_Left", 0, 0),
    ("IA_Rhythm_P0L1", "D", "Gamepad_FaceButton_Bottom", 0, 1),
    ("IA_Rhythm_P1L0", "J", "Gamepad_LeftShoulder", 1, 0),
    ("IA_Rhythm_P1L1", "L", "Gamepad_RightShoulder", 1, 1),
]

for name, k1, k2, p_idx, l_id in input_configs:
    action, action_made = create(name, "/Game/Rhythm/Input", unreal.InputAction, unreal.InputAction_Factory())
    if action:
        action.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
        save(action)
        
        if context:
            for k_str in (k1, k2):
                key = unreal.Key()
                key.set_editor_property("key_name", unreal.Name(k_str))
                context.map_key(action, key)

if context:
    save(context)

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
    save(mode)

unreal.log("Rhythm editor asset creation complete")
