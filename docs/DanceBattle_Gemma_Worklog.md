# Dance Battle: Gemma task queue

## Execution contract

Gemma implements one small task per fresh API request. Supply only the relevant
interfaces, files, constraints, and previous review findings. Review every diff
before applying it; compile and run relevant checks at integration boundaries.
If a check fails, send Gemma a focused correction task with the actual diagnostics.
Never mark a task verified from the model's claim alone.

## Inspection

- Existing project: dancer, Unreal Engine 5.8, currently Blueprint-only.
- Engine directory: G:/UE/UE_5.8.
- Unreal MCP editor tools respond.
- Unsloth Studio responds at http://127.0.0.1:8888.
- OpenAI-compatible routes: /v1/models and /v1/chat/completions.
- LAN endpoint http://192.168.1.12:8888/v1/models permits access without a key.
- Loaded model: gemma-4-26B-A4B-it-UD-Q4_K_XL, context length 8192.
- Module scaffold, rhythm chart/judgment, scoring state and seeded AI compiled successfully.
- Unreal automation DanceBattle.Rhythm.Timing passed (Saved/DanceBattleTests/index.json).
- Full gameplay, input, primitive actors, HUD and both automation test files compiled.
- Final warning cleanup build passed (Saved/DanceBattleBuild.log).
- The connected editor loaded and saved the genuine map and ran live PIE checks.

## Small implementation tasks

1. Add dancer runtime module, game/editor targets, dependencies, project module registration.
2. Define shared side, direction, judgment, phase, and note data types.
3. Implement chart generation and authoritative clock/reset in rhythm component.
4. Implement nearest-note direction/timing judgment, expiry, and once-only guards.
5. Add focused automation tests for boundaries, wrong direction, expiry, and reset.
6. Implement score/phase/time/winner GameState with change delegates.
7. Implement seeded AI attempt scheduling and documented accuracy distribution.
8. Verify AI uses shared judgment, cancels on reset, and acts without input.
9. Implement primitive dancer geometry and transform-preserving animation.
10. Implement podium and shared stage camera.
11. Implement GameMode orchestration, scoring, round finish, and restart.
12. Implement single-player Enhanced Input, arrow/WASD Started mappings and R.
13. Implement UMG HUD structure, score/time/feedback delegate updates.
14. Implement directional targets, shared-clock moving notes, rhythm pulse, results.
15. Compile all classes; assign isolated fixes from compiler diagnostics.
16. Generate and save real map/assets through Unreal tooling; preserve existing maps.
17. Run editor/PIE checks for autonomous AI, input, timing, UI, finish and restart.
18. Assign focused corrections for failed checks, rerun affected checks, document evidence.

## Review findings and corrections

- Initial request exhausted output budget on reasoning; later requests disable thinking.
- Scaffold: rejected missing header, invalid module macro and invalid ModuleRules settings;
  Gemma corrected them, then Unreal build succeeded.
- Rhythm: corrected nearest matching-note selection, delegate declarations and reset-safe
  event snapshots. Tiny follow-ups corrected loop scopes and type-name typos.
- Scoring: rejected replacement shared types and unconditional Perfect scores; regenerated
  against actual headers and compiled the resulting score/state implementation.
- AI: corrected due-time dispatch, scoped enum, probability clamp and callback cancellation
  guards. AI actions always use shared Attempt, never direct scoring.
- Tests: rejected invented fields and removal-based assumptions; corrected actual timing
  assertions and once-per-note expectation. Compiled and ran the real Unreal test: passed.
- Dancers: rejected duplicate enums and scale-destroying transforms. Corrected common-parent
  animation and distinct silhouettes using shared enum definitions.
- Match/input: corrected sanitized duration use, placed-dancer configuration, real MapKey
  calls, controller-owned HUD construction and direction forwarding.

## Final verification

- Full warning-cleanup build: succeeded in 36 seconds.
- Final Unreal automation run: Rhythm.Timing and AI.Autonomous both succeeded,
  zero warnings/failures; `Saved/DanceBattleTestsFinal/index.json`.
- Map generation created a complete serialized save but initially hit Windows
  sharing violation 32 because another editor had the initial map open. After
  MCP reconnected, the open map was confirmed clean, switched to Entry, restored
  from the genuine editor save temporary file, reloaded and saved through MCP.
  Scene inspection found all 17 authored stage actors.
- The first PIE run confirmed autonomous AI, with player 0 / AI 550 at round end.
- A visual review found overexposure and an unsupported real-time sky capture.
  Gemma task19 directly stopped PIE, changed all four lights and saved the map
  through its own Unsloth MCP tool loop. Independent reads confirmed intensities
  10 / 10 / 0.5 / 0.25 and real-time sky capture false.
- Gemma task20 directly restarted PIE and read live game state. Independent
  rendered review confirmed readable HUD, correct silhouettes/placement and no
  sky-capture warning. Live player input scored; one round ended 310 / 550.
- An actual R key event restarted the finished round. Subsequent MCP state read:
  Playing, remaining 58.08, result empty, AI 0, player 10 after new input.
- The additional generated PIE automation draft (task17 response) was not applied
  or run. PIE verification above used the connected editor. Exhaustive manual
  coverage of every alternate key/Caps Lock combination was not performed.

Every request/response is retained in Tools/Gemma. `invoke_editor_task.py` enables
Unsloth's own tool loop with `enable_tools=true`, `mcp_enabled=true`, and bounded
iterations; each narrow editor request starts with a fresh context.

Build memory investigation found approximately 1.3 GB free RAM against a 2.6 GB
shared UnrealEd PCH. Gemma added a private CoreMinimal PCH and module include path;
the resulting PCH is 196 MB. Full build then completed in 96 seconds, followed by
a warning-free cleanup build in 36 seconds. The final state method is ResetRound.
