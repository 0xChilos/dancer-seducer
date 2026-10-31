# Solo Dance Battle — Unreal Engine 5 C++ Implementation Plan

## Goal

Implement a **single-player rhythm/dance battle** inside the **existing Unreal Engine project**, inspired by the supplied screenshot's two-character, two-score, circular-indicator UI composition. **Only the left dancer is player-controlled. The right dancer is an autonomous AI opponent that dances and earns points without human input.** This is a normal dance stage, **not** a volcano: no lava, rocks, cave, or volcanic lighting.

Use **Unreal Engine built-in placeholder meshes** (capsules, spheres, cylinders, cubes) for both characters. Implement gameplay, beat scheduling, AI, scoring and match flow in **C++**. Use UMG and asset Blueprints for presentation/configuration only. No external character models, plugins, network multiplayer or second local player.

> The screenshot establishes the visual style and UI arrangement, not the original game's rules. The mechanics below define a practical playable prototype.

## Core Play Experience

- **Human player (left):** Match moving directional notes (`↑`, `↓`, `←`, `→`) as they enter the target zone. Primary controls are the arrow keys; fallback controls are `W`, `S`, `A`, `D` mapped to Up, Down, Left, Right respectively. Each note is one scoring opportunity.
- **AI opponent (right):** Automatically performs dance moves on beat, with variable accuracy. **Do not require any user input for the AI.** It should animate even when the player does nothing.
- **Both characters:** Have looped idle dance movement and a more expressive, short movement on each attempted beat; success or miss gets distinct feedback.
- **Round:** 60 seconds (configurable), with a 1-second beat interval (configurable).
- **Scoring:** Award 10 points for a Good note and 20 for a Perfect note (configurable); Miss awards zero. Each note can be judged once per side. AI can miss notes; it must not be scripted to win or lose unconditionally.
- **Human timing:** The human must press the matching direction when a moving note overlaps the target zone. Perfect: within ±60 ms of note hit time; Good: within ±150 ms; outside Good window or wrong direction: Miss. Prevent repeated key presses from scoring the same note. Schedule notes in advance so they visibly travel into the target zone.
- **AI timing:** For each AI note, choose an autonomous direction/timing response using a seeded random stream and configurable skill. AI responses are scheduled against the same note hit times and evaluated using the same direction and timing rules as human inputs. Expose difficulty via accuracy and offset-variance settings.
- **Result:** Show player score, AI score, timer, and Win/Lose/Tie at end of round. `R` restarts the match and resets AI/random/beat state; `Esc` may pause if existing project supports it.
- **Match phases:** `Waiting`, `Playing`, `Finished`.

## Player Controls and Timing Rules

| Direction | Primary | Alternative |
|---|---|---|
| Up | `↑` | `W` |
| Down | `↓` | `S` |
| Left | `←` | `A` |
| Right | `→` | `D` |

- Arrows move toward fixed target zones. Press the matching direction as the icon visually aligns with its target; judge timing using scheduled timestamps, not screen coordinates.
- Focusing on the beat/music is helpful, but **the game must work without external music assets**: provide a built-in metronome/pulse cue that stays synchronized with notes. Optional audio can be added later.
- Caps Lock should **not** affect Enhanced Input key mapping; do not require users to disable it. Implement WASD as actual alternative mappings, not as a workaround for broken arrow-key support.
- If multiple notes would overlap, render and judge them independently. Avoid ambiguous note selection and disable autoplay for the human side.

## Visual Target and Level

Create `Content/DanceBattle/Maps/L_DanceBattle.umap`:

1. Two elevated dance podiums side by side on a **simple neutral dance stage**, separated by a small central gap. Use scaled built-in cube/cylinder geometry; create a backdrop with simple panels and stage-light bars.
2. One larger rounded placeholder dancer on the **left**, one slimmer placeholder dancer on the **right**, visually distinguishable by silhouette and basic material colors.
3. Fixed front-facing camera, slightly elevated; both dancers, podium tops, scores and circular UI indicators remain in frame at 16:9.
4. Readable neutral lighting with modest colored accents; no environmental hazards or special external assets.
5. Minimal, functional materials; prioritize gameplay and composition over polish.

**UI similar to screenshot:** Numeric score below each dancer; four glowing turquoise directional target markers and moving arrow notes (per-side note lanes), plus circular rhythm accents by each score; small player and AI identifiers/portraits across the top; centered countdown/round indicator; brief success/miss feedback. Keep the UI legible in Play-in-Editor and at common screen resolutions.

## Architecture (C++)

| Class | Responsibility |
|---|---|
| `ADanceBattleGameMode` | Match start/restart, orchestration, authoritative score rules |
| `ADanceBattleGameState` | Player/AI scores, remaining time, match phase, winner and change delegates |
| `ADanceBattlePlayerController` | **Only one** local controller; Enhanced Input for four human directions, WASD alternatives, and restart |
| `ADancePlaceholderCharacter` | Primitive mesh character, idle dance and beat reaction animation; no skeletal asset required |
| `UDanceRhythmComponent` | Shared note chart/beat timeline, directional hit judgment, and per-side once-per-note guard |
| `UDanceAIOpponentComponent` | Schedules autonomous AI actions per beat, difficulty/accuracy, deterministic optional seed |
| `ADancePodium` | Reusable platform geometry and optional pulse accent |
| `ADanceCamera` | Fixed two-character composition, if an actor class is useful |
| `UDanceBattleWidget` | Event-driven UMG HUD updates and winner display |

Use a single GameMode/GameState pair and **do not create a second PlayerController or attempt to possess the AI character**. The AI component may live on the AI actor or the match manager. Give the player and AI stable `EDanceSide` identifiers rather than pretending both are local human players.

### Shared beat evaluation

- Maintain a single authoritative round start time and beat sequence index.
- Generate or load an ordered directional note chart: `NoteId`, `Side`, `Direction`, `SpawnTime`, `HitTime`. Publish note spawn/judgment events to animate moving icons into fixed directional target zones.
- Human presses are evaluated against the nearest currently eligible pending note in the matching direction for the player lane; wrong-direction presses cannot hit another note.
- AI independently decides **once per assigned note** whether and when to attempt a direction and chooses a timing offset. Schedule the action using the same shared note clock. Its action is validated through the same scoring path as the human; do not increment its score directly from a visual animation function.
- Separate *action attempt* from *successful hit*: AI should still visibly dance on unsuccessful attempts.
- Track each note state (`Pending`, `Perfect`, `Good`, `Miss`) for each side. Never judge a note twice. Expire missed notes after the late window; guard against stale callbacks from prior rounds.
- Never derive rhythm solely from character animation frame rates. Drive timing from a common clock/timer; interpolate visuals independently.

### Suggested editable defaults

| Setting | Default |
|---|---:|
| Round duration | 60 s |
| Beat interval | 1.0 s |
| Perfect timing window | ±0.06 s |
| Good timing window | ±0.15 s |
| Points: Perfect / Good / Miss | 20 / 10 / 0 |
| Arrow travel time before hit | 1.5 s |
| Direction mapping | ↑/W, ↓/S, ←/A, →/D |
| AI hit probability / skill target | 70% (adjustable) |
| AI timing variation | Adjustable, e.g. 0.20 s standard deviation |
| AI difficulty | Easy / Normal / Hard presets |

**Important:** Pick one consistent accuracy mechanism. For instance, sample an AI timing offset and validate it through the shared timing window, tuning the distribution to approximately the desired accuracy; optionally use a separately documented skip probability. Do not apply a 70% success coin flip *and* a large uncalibrated timing error without explaining the resulting effective success rate.

## Agent Execution Phases

### Phase 1 — Inspect and integrate

1. Inspect the `.uproject`, Unreal version, existing source module/`.Build.cs`, plugins, input patterns and content folders.
2. Work **within the existing project** and follow its naming/module conventions. Do not overwrite other maps or default gameplay settings unnecessarily.
3. Add `EnhancedInput`, `UMG`, `Slate` and `SlateCore` dependencies as required.
4. Create `Source/<ExistingModule>/DanceBattle/` and `Content/DanceBattle/` subfolders.

### Phase 2 — Stage and placeholder actors

1. Implement reusable podium and character actors with Unreal's built-in primitive meshes.
2. Assemble and **save** `L_DanceBattle` through supported Unreal Editor tooling; do not fake `.umap` or `.uasset` binary files.
3. Place/spawn left player dancer and right autonomous AI dancer, both stationary on their respective podiums.
4. Set a fixed shared camera, basic stage lights and backdrop.
5. Add procedural C++ animation: idle bob/sway, action bounce/tilt and brief miss feedback. Maintain initial relative transforms to avoid animation drift.

### Phase 3 — Solo input and beat manager

1. Set up Enhanced Input actions `IA_Up`, `IA_Down`, `IA_Left`, `IA_Right`, and `IA_Restart`. Bind both `↑/W`, `↓/S`, `←/A`, `→/D` to their corresponding directional actions and `R` to restart. Use key-down/Started events, not held-key repeat.
2. Implement the GameMode/GameState match phases, timer and score changes.
3. Implement a centralized chart of directional notes; spawn icons about 1.5 seconds ahead of the corresponding hit time, move each icon smoothly toward its directional target zone based on the shared clock, and judge Perfect/Good/Miss using note timestamps (not render frame positions).
4. Bind human inputs to **player side only**. No `L` control for the AI; no split-screen.

### Phase 4 — Autonomous AI dancer

1. Implement `UDanceAIOpponentComponent` with configurable accuracy and optional seed.
2. For each scheduled AI note, automatically choose a direction (normally matching the note), timing offset and optional documented skip/wrong-direction behavior, then queue at most one AI attempt. No tick-based random key spam.
3. Dispatch AI directional attempts into the same judgment logic as human inputs and animate the right character on Perfect, Good or Miss; AI requires no keyboard input.
4. Cancel pending AI actions when round ends/restarts; guard callbacks with match generation/beat IDs.
5. Confirm AI accumulates a realistic mixture of hits/misses and keeps dancing **with zero human interaction**.

### Phase 5 — Screenshot-inspired UI

1. Implement an event-driven `UDanceBattleWidget` with optional Widget Blueprint layout.
2. Place a large left player score and right AI score low in the frame, turquoise circular accents, top identifiers, and a match timer. Add four clearly recognizable arrow target zones for the player and AI and moving arrow icons traveling along the corresponding lanes; show directional prompts with sufficient contrast and accessible feedback.
3. Animate each note from spawn to target using the shared timebase; show Perfect/Good/Miss feedback independently for player and AI and clear judged/expired notes. UI timing must remain stable across frame rates.
4. Show the final Win/Lose/Tie panel with a restart hint. Avoid tick-based text binding if delegates suffice.

### Phase 6 — Play and verify

1. Compile C++ with the project's supported build setup and fix all introduced errors.
2. Open `L_DanceBattle` and validate Play-in-Editor, camera layout, collision and UI.
3. Confirm the AI acts and scores **without any key presses**.
4. Confirm `↑/W`, `↓/S`, `←/A`, `→/D` hit only their matching left-lane notes; wrong-direction, early, late and held inputs cannot farm score.
5. Confirm note icons line up with their target zones at scheduled hit times; Perfect/Good/Miss thresholds, note expiry, and once-per-note scoring behave correctly at different frame rates.
6. Confirm rounds end correctly, results are correct, and `R` fully resets human and AI state.
7. Save all new content assets. Report changed files, how to run, controls and validation results; distinguish verified tests from tests not run.

## Suggested File Layout

```text
Source/<ExistingModule>/DanceBattle/
  DanceBattleGameMode.h/.cpp
  DanceBattleGameState.h/.cpp
  DanceBattlePlayerController.h/.cpp
  DancePlaceholderCharacter.h/.cpp
  DanceRhythmComponent.h/.cpp
  DanceAIOpponentComponent.h/.cpp
  DancePodium.h/.cpp
  DanceCamera.h/.cpp
  DanceBattleWidget.h/.cpp

Content/DanceBattle/
  Maps/L_DanceBattle.umap
  Blueprints/BP_DanceBattleGameMode.uasset
  UI/WBP_DanceBattleHUD.uasset
  Materials/M_DanceStage.uasset
  Materials/M_DanceAccent.uasset
  Input/IA_Up.uasset
  Input/IA_Down.uasset
  Input/IA_Left.uasset
  Input/IA_Right.uasset
  Input/IA_Restart.uasset
  Input/IMC_DanceBattle.uasset
```

Paths are examples; adapt to existing module and editor-generated asset conventions.

## Acceptance Criteria

- [ ] Existing Unreal project compiles with new C++ feature classes.
- [ ] Saved `L_DanceBattle` loads and runs in Play-in-Editor.
- [ ] Stage is a normal dance stage, **no volcanic environment**.
- [ ] Two distinct dancers use **only Unreal placeholder primitive meshes**.
- [ ] **Only one human-controlled dancer (left)**; directional arrow keys and WASD alternatives trigger corresponding notes.
- [ ] **AI controls the right dancer by itself**; it animates and scores with no human interaction.
- [ ] AI has visible hit/miss variation and configurable difficulty.
- [ ] Both scores use one shared authoritative directional note system, once per note per side, with Perfect/Good/Miss judgments.
- [ ] Screenshot-inspired twin-score, turquoise-circle HUD plus moving arrow notes and target zones remain readable.
- [ ] Round clock, winner/loser/tie display and restart all work.
- [ ] New `.umap`/`.uasset` assets are genuine editor-generated, saved project files.
- [ ] Agent documents what it actually compiled/tested, with any remaining issues.

## Copy/Paste Agent Directive

> Implement this plan in the **existing Unreal Engine C++ project**. This is a **solo dance battle against an AI**—not two human players and not a volcano. Use built-in Unreal primitive meshes for both placeholder dancers. Put the human on the left and an autonomous AI dancer on the right, with fixed camera and UI closely resembling the supplied screenshot's two scores and circular beat indicators. Implement a shared C++ directional note chart with moving arrow icons that align with fixed target zones. Player controls are `↑/W`, `↓/S`, `←/A`, `→/D`; judge note direction and Perfect/Good/Miss timing, prevent repeated scoring and expire misses. The AI responds and dances autonomously to its own directional notes with configurable mistakes. Implement score updates, 60-second rounds, Win/Lose/Tie results and restart (`R`). Create a real saved playable map and UMG UI assets using supported Unreal tooling. Do not stop at compiling C++: integrate and test the scene in Play-in-Editor where available. Never claim a test succeeded unless it was actually performed.
