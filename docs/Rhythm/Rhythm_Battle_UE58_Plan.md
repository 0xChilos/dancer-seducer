# Rhythm Battle UI / Gameplay – UE 5.8 C++ Implementation Plan

**Reference:** User-supplied *Madagascar: Escape 2 Africa* dance-off screenshot. Recreate the interaction language (moving dots along straight lanes, stationary circular hit targets, score feedback near each character) without copying the game's copyrighted assets or arena. Initial prototype: two lanes per character. Architecture supports up to six lanes per character (12 total).

**Target:** Reusable Unreal Engine 5.8 runtime plugin with data-driven rhythm charts, precise input scoring, Blueprint-driven events/pauses/modifiers, and 3D-arena HUD integration. Implementation to be delegated to local Unsloth Gemma agents (8,192 context) through an orchestrator with a *single serialized Unreal MCP editor connection*.

## 0. Hard rules

1. C++ owns gameplay timing, input judgment, state transitions, and score. Widget animations never determine whether a note hits.
2. Blueprint owns authoring: layouts, event actions, cosmetic animations, and overrides. Artists/designers must tune gameplay without C++ recompilation.
3. Hold a single canonical song timeline (`SongTimeSeconds`) and immutable hit times. Game thread frame deltas must never accumulate the song clock.
4. Prefer Quartz for music start/quantization. Calibrate game-thread input timestamps against audio onset; document output/input offsets. Pausing audio and game logic must keep their timebases consistent.
5. `VisualSpeedMultiplier` modifies visual travel/lead, not note hit timestamps. Smooth transitions; do not teleport active notes when speed changes.
6. MCP is an experimental editor tool, used only during development. The shipped game must not depend on MCP or Unsloth.
7. Do not overwrite existing project classes/assets without discovery and approval. Keep all new assets inside the plugin or `Content/Rhythm/`.

## 1. UX / visual specification

- Show a left and right character in a 3D arena with screen-space rhythm overlays located near each character.
- On each side, render one or more straight paths (horizontal or diagonal, never curved), evenly spaced decorative white/yellow dots, moving active notes, and stationary circular button targets. A hit shows subtle target flash, result text (Perfect / Good / OK / Miss), and numeric feedback. Combo and score are shown separately per side.
- An optional center-top HUD shows song progress and battle phase. Make this switchable.
- **Prototype**: 2 lanes per side; expand to 6 lanes/side through data, not conditional UI hardcoding. Different lane types (tap, optional hold) allowed; tap is MVP.
- `FRhythmLaneLayout` exposes `Side`, `LaneId`, `Start`, `HitTarget` (normalized viewport coordinates), style references, visibility, and input action mapping.
- Render each lane as a single straight segment from `Start` to `HitTarget`. Use `FMath::Lerp(Start, HitTarget, Progress)` for the moving note, where `Progress = Clamp(1 - (HitTimeSeconds - SongTimeSeconds) / ApproachDurationSeconds, 0, 1)`. A lane may be horizontal or diagonal, but never bends. Space decorative dots evenly with linear interpolation. No Bézier curves, control points, splines, or arc-length lookup tables are needed. Cache static lane geometry and reuse draw items; do not create/destroy UObjects every frame.
- Make `ApproachDurationSeconds`/visual speed configurable globally and per lane. Speed adjustments must leave authored hit times unchanged; transition active note trajectories smoothly to prevent jumps.
- UMG root (`URhythmBattleWidget`) + custom `URhythmLaneWidget` that draws straight lane segments/notes via batched Slate/UMG painting; not hundreds of independently-ticking widgets.
- Viewport anchoring options: fixed normalized screen coordinates for first iteration; optional projected character position with UI offset in later iteration. Test viewport resizing, safe zones, and aspect ratios.

## 2. Technical layout

```
Plugins/RhythmBattle/
  RhythmBattle.uplugin
  Source/RhythmBattle/
    RhythmBattle.Build.cs
    Public/
      RhythmBattleTypes.h
      RhythmBattleComponent.h
      RhythmChart.h
      RhythmBattleConfig.h
      RhythmEventAction.h
      RhythmModifierManager.h
      RhythmBattleWidget.h
      RhythmLaneWidget.h
    Private/
      RhythmBattleComponent.cpp
      RhythmClock.cpp
      RhythmNoteScheduler.cpp
      RhythmHitJudge.cpp
      RhythmScoreCalculator.cpp
      RhythmModifierManager.cpp
      RhythmEventAction.cpp
      RhythmBattleWidget.cpp
      RhythmLaneWidget.cpp
    Private/Tests/
      RhythmJudgeTests.cpp
      RhythmClockTests.cpp
      RhythmPauseTests.cpp
      RhythmModifierTests.cpp
  Content/ (Blueprint examples and materials, when appropriate)
Content/Rhythm/
  Charts/DA_RhythmDemo
  Config/DA_RhythmDefault
  UI/WBP_RhythmBattle
  UI/WBP_RhythmLane
  Input/IMC_Rhythm, IA_RhythmLane_01...
  Events/BP_RhythmEvent_Taunt, BP_RhythmEvent_SpeedBoost
```

**Dependencies:** Core, CoreUObject, Engine, UMG, Slate, SlateCore, EnhancedInput, GameplayTags; include Quartz/audio module(s) after confirming exact UE 5.8 module names and API in installed engine. Test module dependencies should be editor/test only.

### Source-level interfaces

- `URhythmChart : UPrimaryDataAsset` — BPM, time signature, offset, immutable `FRhythmNoteDefinition` array, event markers, lane list.
- `URhythmBattleConfig : UPrimaryDataAsset` — timing windows, score tiers, combo rules, visual approach duration, default speed, input compensation, HUD style, pause policy.
- `URhythmBattleComponent : UActorComponent` — orchestrates playback, input judgment, score, event queue, and Blueprint events. Support `StartBattle`, `StopBattle`, `PauseRhythm(Reason)`, `ResumeRhythm(Reason)`, `SubmitLaneInput`, `SetVisualSpeed`, `AddTimedModifier`, `RemoveModifier`, `TriggerRhythmEvent`, `SetLaneEnabled`, `GetBattleState`.
- `FRhythmClock` — time provider with deterministic fake clock interface for tests. Quartz/audio-to-game timestamp alignment is injected rather than coupled to widget ticks.
- `FRhythmNoteScheduler` — per-player/per-lane sorted queues by target hit time; stable IDs; states Upcoming/Active/Hit/Missed/Skipped. Use a bounded lookahead and sweep missed notes once.
- `FRhythmHitJudge` — closest eligible note in the pressed lane within window; exactly-once scoring; signed timing error (early/late).
- `FRhythmScoreCalculator` — fixed-point/int points, configurable score tiers, chain combo, score multipliers, reset rules. Avoid point ambiguity between visible counters and score.
- `URhythmModifierManager` — modifiers with ID, scope (battle/player/lane), type (VisualSpeed/Score/HitWindow/Density/LaneDisable), value, duration in beats or seconds, priority, and restoration strategy. Define stacking and conflict rules.
- `URhythmEventAction : UObject` — `Blueprintable, EditInlineNew, DefaultToInstanced`, Blueprint-native hooks Execute/Cancel/Complete. Event cue has an ID, beat/time, optional conditions, and action. All event execution happens on game thread, separated from hit judgment.
- `URhythmBattleWidget` — consumes lightweight immutable snapshot each paint/update: `SongTimeSeconds`, lane note views, points, combo, hit result; owns no judgment or audio state.

## 3. Timing, movement, and judgment

At BPM 120, `SecondsPerBeat=60/BPM=0.5`. A note is authored at musical `BeatIndex` with `HitTimeSeconds = SongOffset + BeatIndex*SecondsPerBeat` (extend for BPM changes with a tempo map later).

Initial defaults:

| Tier | Maximum absolute error | Base points |
| --- | ---: | ---: |
| Perfect | 0.045 s | 100 |
| Good | 0.100 s | 70 |
| OK | 0.160 s | 30 |
| Miss | >0.160 s | 0 |

For visible approach duration `D`, basic render progress `u = Clamp(1 - (HitTime-SongTime)/D,0,1)`. A speed multiplier changes the approach duration, not the hit time. For instantaneous modifier changes, use an eased renderer state over 100–200 ms or a per-note reparameterization preserving current progress and target time. Test speed changes mid-approach explicitly.

Input time = mapped song time from monotonic timestamp - calibrated input offset. Never judge from target-circle collision or projected pixel location. When paused, reject inputs or route them to the explicit paused-event interaction system.

## 4. Blueprint events and pause semantics

Event trigger modes: (a) authored chart marker at beat/time, (b) reactive score/combo/miss condition, (c) external Blueprint call. Expose `OnRhythmEventTriggered`, `OnNoteHit`, `OnNoteMiss`, `OnScoreChanged`, `OnComboChanged`, `OnRhythmPaused`, `OnRhythmResumed`, `OnBattleFinished` as BlueprintAssignable multicast delegates.

`PauseRhythm(FName Reason)` and `ResumeRhythm(FName Reason)` use a set of active pause reasons; only resume when no reasons remain. Define distinct stop modes:

- `PauseAll`: freeze canonical chart clock, pause audio, freeze notes/judgment; on resume preserve timeline.
- `StopSpawning`: no future visual spawns; existing notes continue; chart clock runs.
- `DisableLane`: suppress/skip (not hold) affected lane notes while chart clock runs; explicit miss policy.
- `AwaitEvent`: same as PauseAll, but an asynchronous Blueprint callback releases its specific reason; optionally use a 1-beat count-in.
- `EndBattle`: clear unresolved notes, fade audio, broadcast final scores.

Never freeze a lane's display while audio and chart time continue unless its notes are explicitly marked skipped; this otherwise creates impossible timing.

Suggested demo events:

1. Beat 16: `OnRhythmEventTriggered(Taunt)`, `PauseRhythm("Taunt")`, play animation, `ResumeRhythm("Taunt")` on callback, resume with count-in.
2. Beat 32: visual speed x1.35 for 8 beats (no hit-time changes).
3. Combo >= 20: score multiplier x2 for 4 beats; modifier expires automatically.
4. Beat 48: disable an opponent lane for 2 beats, skip its notes, then restore.

Expose Blueprint nodes to query current effective values and active modifiers. Include style changes as an optional separate cosmetic event.

## 5. Proposed implementation phases (dependency ordered)

### P0 — Project reconnaissance & baseline
- Inspect project version, source control, existing UI/character architecture and input system. Enable UE 5.8 Unreal MCP (`ModelContextProtocol`) and `All Toolsets`. Start server locally, inspect toolsets via `list_toolsets`, avoid sending the entire schema into the 8k context. Create a feature branch and record clean compile.
- **Gate:** Runtime plugin compiles empty; confirmed MCP introspection; baseline smoke test succeeds.

### P1 — Data types & deterministic judge
- Implement types, chart/config data assets, lane keys, input timestamp model, score/judgment and tests.
- **Gate:** Exact boundary tests for ±45/100/160ms, nearest-note selection, duplicate press, miss expiration, stable score.

### P2 — Audio timeline, note scheduler, pause
- Integrate Quartz start, song clock, scheduler, start/stop/pause/resume, reason tokens, deterministic fake clock tests.
- **Gate:** BPM 120 test chart yields targets at precise 0.5s multiples; pausing shifts neither rhythm relationships nor score; repeated pause reasons safe.

### P3 — C++/UMG renderer with temporary assets
- Draw fixed 2-lane-per-side test overlay, straight segments, dotted lanes, hit circles, glowing approach notes, points/combo. Data-driven layout scaffolding.
- **Gate:** A designer can move start points and hit targets to set each straight lane angle and length without code edits. Rendered notes arrive at hit circle at judged timestamp. Two independent player HUDs visible.

### P4 — Input integration and playable vertical slice
- Enhanced Input mapping contexts, focus handling, keyboard/gamepad press edges, UI/audio calibration, hit feedback, character animation event binding.
- **Gate:** 30-second playable demo with two lanes/side; scoring and combo work; 60/120 FPS changes do not affect logical judgments.

### P5 — Blueprint event director / modifiers
- Build timeline markers, condition triggers, action base class, pause/stop semantics, modifiers and sample BP events.
- **Gate:** Beat-16 taunt pause, beat-32 speed boost, combo score boost, and lane suppression function without C++ edits. Undo/cancel modifiers correctly.

### P6 — 12 lanes, tuning UI, QA/handoff
- Test up to six lanes per player, reusable chart/layout assets, optional hold notes after tap baseline is stable. Add editor-facing details, help docs, performance test, screenshot reference comparison.
- **Gate:** 12 lanes/100 simultaneous visible note objects (stress scenario) without per-frame UObject churn. No unsynchronized gameplay after pause/resume or visual speed changes. Test on 16:9 and 21:9.

## 6. QA plan

Automation tests using UE's C++ test framework and in-editor Functional Tests:
- Timing at all judgment thresholds; negative and positive signed offsets.
- Same-lane adjacent notes; double press; early/late; missed note cleanup.
- Quartz scheduling: delayed start and pause/resume alignment.
- Two independent sides; disabled lane; opponent autonomy.
- Modifier priorities, stacking, durations, cancellation, and reinstate original state.
- Blueprint-triggered nested pauses and asynchronous event completion.
- Editor asset-load checks and CI compile.
- Visual tests at 1080p and ultrawide; HUD stays near characters and does not occlude important scene elements.
- Performance targets should be measured against a declared test machine: 60 FPS gameplay on the target platform, no new per-note UObjects per frame, and no game-thread hitch when active note count changes.

## 7. Agent operating procedure

Use `AGENT_ORCHESTRATION.md` and `TASKS.json` from this package. Limit the local Gemma to 8,192 tokens per call: small task prompt, only 1–3 relevant file snippets, concise MCP tool schema, one tool call at a time, and a short persisted handoff. Separate C++ code writing from Unreal asset modification. Multiple agents may plan/write independent files in parallel worktrees, but Unreal MCP calls MUST be serialized on one editor process. Require compilation/tests and orchestrator review before merge.

## 8. Explicit non-goals for first playable

No multiplayer replication, beat chart authoring GUI, network sync, motion-captured dance animation system, complex hold/swipe/spin gesture types, or full music-tempo-map editing. The core model should not prevent their later addition.

## 9. Evidence / reference docs

- UE 5.8 Unreal MCP: https://dev.epicgames.com/documentation/unreal-engine/unreal-mcp-in-unreal-editor
- UE 5.8 Quartz: https://dev.epicgames.com/documentation/unreal-engine/overview-of-quartz-in-unreal-engine
- UE 5.8 Enhanced Input: https://dev.epicgames.com/documentation/unreal-engine/enhanced-input-in-unreal-engine
- UE 5.8 Automation Testing: https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine
- Unsloth local model + agent integration: https://www.unsloth.ai/
