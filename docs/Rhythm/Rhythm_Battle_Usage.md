# Rhythm Battle usage

The `RhythmBattle` plugin supplies a battle component, chart and configuration data assets, Slate-backed HUD, and demo game mode. The existing arena can use the generated `BP_RhythmArenaGameMode` without replacing its map or level content.

## Setup

1. Build the `dancerEditor` target with UE 5.8. Enable the RhythmBattle plugin and Enhanced Input if they are not already enabled.
2. Open the project in the editor and run `docs/Rhythm/tools/create_editor_assets.py` using **Tools > Execute Python Script**. The script creates missing assets under `/Game/Rhythm`; it leaves existing assets intact. `docs/Rhythm/tools/editor_startup.py` also registers a temporary PIE QA observer and is intended for verification.
3. In the existing arena's World Settings, set GameMode Override to `/Game/Rhythm/BP_RhythmArenaGameMode`. Its controller is `/Game/Rhythm/BP_RhythmPlayerController`, with `/Game/Rhythm/UI/WBP_RhythmBattle` as its HUD. A camera tagged `DanceBattleCamera` is selected when available.
4. Assign a `RhythmChart`, `RhythmBattleConfig`, and optional `DemoMusic` on the game mode defaults. With no chart or config assigned, the demo creates defaults; without music it generates a click track when chart settings permit.
5. Play in editor. Restart creates a fresh battle. `StartBattle` validates the chart and config and returns false for invalid values.

## Controls

| Action | Keyboard | Gamepad |
| --- | --- | --- |
| Player 0 lanes 0 / 1 | A / D | Face Left / Face Bottom |
| Player 1 lanes 0 / 1 | J / L | Left Shoulder / Right Shoulder |
| Restart | R | Special Right |
| Toggle player pause | P | Special Left |

Player 1 autoplay is enabled by default on the demo mode; disable `OpponentAutoplay` for manual play. Inputs call `SubmitLaneInput` for a player and lane. The generated input context contains the four lane actions; restart and pause retain the controller's built-in bindings.

## Authoring charts and input

Create data assets based on `RhythmChart` and `RhythmBattleConfig`. A chart defines BPM, song offset, duration, lane layouts, notes and beat-timed events. Each note references an existing `(PlayerIndex, LaneId)` pair; lane IDs must be unique per player, with at most six lanes per player. Lane start and hit-target coordinates are normalized to the HUD. Tune lane color, dot radius and optional approach duration there. Positive song offset places beats later in the song.

For custom lane input, assign an Input Action to each lane's `InputAction`, map those actions in an Enhanced Input Mapping Context, and set `AuthoredMapping` on the demo player controller Blueprint. The controller binds chart actions during input setup, so assign the chart before play. With an authored mapping, lane bindings come from the chart rather than the built-in lane actions.

The config sets Perfect, Good and OK timing windows, base approach duration, visual speed, score and hit-window multipliers, score tiers, combo multiplier, miss penalty and input latency compensation. A positive input latency compensation is subtracted from the sampled input time. Calibrate it on the target device against audible music and input latency.

## Timing, scoring and events

A hit awards the judgment's base score multiplied by the effective score multiplier and `1 + prior combo × ComboMultiplier`; then combo increases. A miss resets combo and deducts at most the current score. Lane disabling skips affected notes without a miss penalty. `StopSpawning(true)` skips future notes whose spawn time arrives while spawning is disabled; already active notes continue. Use `StopSpawning(false)` to resume spawning.

Timed modifiers can target the battle, a player or a lane. Matching numeric modifiers multiply, subject to the manager's bounds; lane-disable modifiers combine as a logical OR. A modifier with an existing ID replaces it. Durations may be seconds or beats and expire against song time, which stops during a rhythm pause.

`PauseRhythm(Reason)` adds a named reason; `ResumeRhythm(Reason)` removes that reason. The battle resumes only when all reasons are removed. The demo's native Taunt cue pauses for 1.5 seconds of wall time; SpeedBoost and LaneBreak apply timed modifiers. Cue actions can be authored with the generated `/Game/Rhythm/Events` Blueprint examples. `RhythmPauseEventAction` pauses using the event ID and resumes the same reason on completion or cancellation; arrange for an external event or action owner to call completion, since song time is frozen while paused.

With music, the component starts playback through Quartz and uses its transport as the song clock. If Quartz cannot start or its pending command fails or times out, gameplay falls back to the monotonic clock. Playback and gameplay callbacks cross threads, so calibrate latency rather than assuming sample-perfect input judgment. A battle started without music uses the monotonic clock directly.

## Build and verification

Example commands for this checkout (close the editor before building):

```powershell
& 'G:\UE\UE_5.8\Engine\Build\BatchFiles\Build.bat' dancerEditor Win64 Development 'G:\UE\dancer\dancer.uproject' -NoPCH -NoUBA -NoCache -UBADisableWaitOnMem -MaxParallelActions=2
& 'G:\UE\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'G:\UE\dancer\dancer.uproject' -unattended -nop4 -nosplash -NullRHI -ExecCmds='Automation RunTests RhythmBattle; Quit' -TestExit='Automation Test Queue Empty'
```

Verification results: pending.
