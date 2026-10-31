# Dance Battle prototype

Open `dancer.uproject` with Unreal Engine 5.8, then open
`/Game/DanceBattle/Maps/L_DanceBattle` and press Play. The original default map
is preserved. Reopen the editor after the project's first C++ module build.

- Human: left dancer; arrow keys or W/S/A/D for Up/Down/Left/Right.
- Match each moving arrow as it reaches the corresponding circular target.
- Perfect: within 60 ms, 20 points. Good: within 150 ms, 10 points.
- Wrong direction or a missed timing window awards zero. A note scores once.
- The right dancer acts autonomously; no second controller or AI input key.
- R resets the round, chart, scores, seeded AI actions, and feedback.

Edit the game mode Blueprint's `Settings` to configure duration, beat interval,
travel time, timing windows, points and AI. `AIAccuracy` is the probability of
sampling a timing offset inside the Good window. Failed attempts sample outside
that window using `AITimingSpread`; there is no second independent failure roll.
The same seed reproduces the chart and AI decisions after restart.

Gemma requests, responses and integration scripts are retained in `Tools/Gemma`.
Each request uses a fresh context; the loaded model's limit is 8,192 tokens.
See `DanceBattle_Gemma_Worklog.md` for review findings and current validation.

## Validation status

- Final Unreal Editor C++ build passed, including HUD, actors, controller and tests.
- `DanceBattle.Rhythm.Timing` and `DanceBattle.AI.Autonomous` both passed in Unreal;
  final report: `Saved/DanceBattleTestsFinal/index.json` (2 success, 0 failures).
- Genuine editor-generated map, five materials and both Blueprints are saved.
- Connected-editor PIE verified the scene/HUD, autonomous AI scoring, live human
  scoring, timer expiry and the final result. An unattended round ended at 0/550;
  a later live-input round ended at 310/550 with `YOU LOSE`.
- An actual R key event restarted the finished round: Playing, result cleared,
  remaining time 58.08 seconds, AI score 0, player score 10 after new input.
- Gemma directly used Unreal MCP to correct and save lighting. Independent
  property reads and a rendered editor capture confirmed the correction.
- Win/Tie branches and all key mappings were reviewed in C++; exhaustive manual
  testing of every arrow/WASD/Caps Lock combination was not performed.

The module uses a small private CoreMinimal PCH to keep build memory manageable
while the local model is loaded. The validated build used `-MaxParallelActions=1`.
