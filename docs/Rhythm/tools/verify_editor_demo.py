"""Observe an already-running PIE demo; writes Saved/RhythmDemoQA.json.

Execute in the editor after requesting PIE. Player 0 inputs are simulated through
SubmitLaneInput; this does not exercise physical keyboard input.
"""
import json
import os
import time
import traceback
import unreal

_OUTPUT = os.path.join(unreal.Paths.project_saved_dir(), "RhythmDemoQA.json")
_registered = time.monotonic()
_pie_started = None
_handle = None
_last_time = None
_attempted = set()
_result = {
    "status": "running", "input_method": "simulated SubmitLaneInput",
    "paused_observed": False, "speed_boost_observed": False,
    "score_boost_observed": False, "observed_modifier_ids": [],
    "song_time_monotonic": True, "duplicate_checks": 0,
    "duplicate_failures": [], "submitted_hits": 0,
}


def prop(obj, name):
    return obj.get_editor_property(name)


def class_path(obj):
    return obj.get_class().get_path_name() if obj else None


def finish(error=None):
    global _handle
    _result["status"] = "error" if error else "complete"
    if error:
        _result["error"] = error
    try:
        os.makedirs(os.path.dirname(_OUTPUT), exist_ok=True)
        with open(_OUTPUT, "w", encoding="utf-8") as output:
            json.dump(_result, output, indent=2)
    finally:
        if _handle is not None:
            unreal.unregister_slate_post_tick_callback(_handle)
            _handle = None


def tick(_delta):
    global _last_time, _pie_started
    try:
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not world:
            if time.monotonic() - _registered > 1800:
                raise RuntimeError("Timed out waiting for PIE after 30 minutes")
            return
        if _pie_started is None:
            _pie_started = time.monotonic()
        if time.monotonic() - _pie_started > 120:
            raise RuntimeError("Timed out waiting for song time > 30 seconds after PIE started")
        mode = unreal.GameplayStatics.get_game_mode(world)
        if not mode:
            return
        battle = prop(mode, "battle")
        if not battle:
            return
        controller = unreal.GameplayStatics.get_player_controller(world, 0)
        hud = None
        if controller:
            try:
                hud = prop(controller, "hud")
            except Exception as hud_error:
                _result["widget_inspection_error"] = str(hud_error)
                try:
                    widgets = unreal.WidgetBlueprintLibrary.get_all_widgets_of_class(world, unreal.RhythmBattleWidget, top_level_only=True)
                    hud = widgets[0] if widgets else None
                except Exception as widget_error:
                    _result["widget_enumeration_error"] = str(widget_error)
        _result["battle_path"] = battle.get_path_name()
        _result["game_mode_class"] = class_path(mode)
        chart = prop(mode, "demo_chart")
        if chart:
            _result["chart_note_count"] = len(prop(chart, "notes"))
            _result["chart_lane_pairs"] = [[prop(lane, "player_index"), prop(lane, "lane_id")] for lane in prop(chart, "lanes")]
        _result["controller_class"] = class_path(controller)
        _result["widget_class"] = class_path(hud)
        now = battle.get_song_time_seconds()
        state = battle.get_battle_state()
        _result["song_time"] = now
        if now > 5:
            try:
                _result["audio_sync_active_after_5s"] = bool(prop(battle, "audio_sync"))
                _result["audio_sync_class"] = class_path(prop(battle, "audio_sync"))
            except Exception as audio_error:
                _result["audio_sync_inspection_error"] = str(audio_error)
        _result["state"] = str(state)
        if _last_time is not None and now + 0.0001 < _last_time:
            _result["song_time_monotonic"] = False
        _last_time = now
        if state == unreal.RhythmBattleState.PAUSED:
            _result["paused_observed"] = True
        modifiers = battle.get_active_modifiers()
        ids = [str(prop(m, "id")) for m in modifiers]
        _result["observed_modifier_ids"] = sorted(set(_result["observed_modifier_ids"] + ids))
        if "ComboBoost" in ids:
            _result["score_boost_observed"] = True
        if any(str(prop(m, "id")) == "SpeedBoost" for m in modifiers):
            _result["speed_boost_observed"] = True
        notes = battle.get_note_snapshot()
        if state == unreal.RhythmBattleState.PLAYING:
            for note in notes:
                note_id = prop(note, "note_id")
                target = prop(note, "hit_time_seconds")
                if (prop(note, "player_index") == 0 and prop(note, "spawned")
                        and prop(note, "judgment") == unreal.RhythmJudgment.NONE
                        and note_id not in _attempted and 0 <= now - target <= 0.1):
                    _attempted.add(note_id)
                    lane = prop(note, "lane_id")
                    hit = battle.submit_lane_input(0, lane)
                    if prop(hit, "note_id") == note_id:
                        _result["submitted_hits"] += 1
                        before = [prop(s, "score") for s in battle.get_score_snapshot()]
                        duplicate = battle.submit_lane_input(0, lane)
                        after = [prop(s, "score") for s in battle.get_score_snapshot()]
                        _result["duplicate_checks"] += 1
                        if prop(duplicate, "note_id") != -1 or before != after:
                            _result["duplicate_failures"].append({"note_id": note_id, "before": before, "after": after, "duplicate_note_id": prop(duplicate, "note_id")})
        if now > 30 or state == unreal.RhythmBattleState.FINISHED:
            notes = battle.get_note_snapshot()
            _result["note_counts"] = {}
            for note in notes:
                key = str(prop(note, "player_index")) + ":" + str(prop(note, "judgment"))
                _result["note_counts"][key] = _result["note_counts"].get(key, 0) + 1
            _result["scores"] = [{"player": prop(s, "player_index"), "score": prop(s, "score"), "combo": prop(s, "combo"), "max_combo": prop(s, "max_combo")} for s in battle.get_score_snapshot()]
            finish()
    except Exception:
        finish(traceback.format_exc())


_handle = unreal.register_slate_post_tick_callback(tick)
unreal.log("Rhythm demo QA observer registered; output: " + _OUTPUT)
