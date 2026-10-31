# Local Gemma Agent Orchestration — RhythmBattle

## Purpose

Deliver the plan in `IMPLEMENTATION_PLAN.md` with a **local** Gemma model served by Unsloth, context limit 8,192 tokens. Agents are *logical roles*: run independent C++ writers in isolated worktrees if the local machine can support them, or schedule them sequentially through the same model. **Do not let multiple agents issue overlapping Unreal MCP calls.** The UE 5.8 MCP docs explicitly require serialized editor calls because tools execute on the game thread.

This file is a runbook / prompt specification, not a claim that agents have been started from ChatGPT.

## Verified setup (UE 5.8)

1. Enable **Unreal MCP** and **All Toolsets** under Edit > Plugins; restart editor.
2. Under Editor Preferences > Model Context Protocol, enable Auto Start Server, or run editor console `ModelContextProtocol.StartServer`.
3. Default editor MCP endpoint: `http://127.0.0.1:8000/mcp` (localhost only). Generate client config with `ModelContextProtocol.GenerateClientConfig All`. Use supported client from project root.
4. With MCP tool search enabled (default), discover `list_toolsets`, then `describe_toolset`, then `call_tool` for the needed tool only; do not dump hundreds of schemas into Gemma's context.
5. Start Unsloth, load a code-capable Gemma variant, configure its OpenAI-compatible API or supported local agent CLI. Verify model supports tool calls before trusting it to use UE MCP. Unsloth docs describe running local agents and `unsloth start claude`; actual client and tool reliability must be tested with this Gemma build.
6. Protect source tree with git branch / worktrees. Unreal's MCP is experimental and has no authentication beyond loopback behavior: do not expose its listener to the LAN/Internet.

## Coordinator contract

Coordinator owns project discovery, task assignment, file-ownership locks, review, merge, compile/test gates, and the *editor MCP call queue*. Coordinator cannot mark a task done without file evidence and a test result. Each worker must return a summary of ≤350 tokens with changed paths, tests, caveats, and one next step.

### Roles

| Role | Assigned files | Allowed tools | Acceptance output |
|---|---|---|---|
| Core / clock worker | types, chart/config, clock, scheduler, judge, score | filesystem / compiler / tests; MCP read-only during discovery | deterministic notes and tests |
| UI worker | BattleWidget, LaneWidget, materials | code + UI editor calls via queued MCP | functioning 2-side HUD screenshot |
| Events worker | EventAction, ModifierManager, pause handling | code + queued MCP for sample BPs | Blueprint configurable event demos |
| Editor/Assets worker | Data Assets, UMG BP, input assets, demo level integration | **exclusive** editor MCP lease | compile-clean assets and wiring |
| QA reviewer | Tests and reports only; no direct product-file edits | test tools, editor read-only through queue | PASS/FAIL with exact reproduction |
| Coordinator | no ownership of feature files except targeted integration fixes | task dispatch, git, build/test, serialized MCP | merged and validated vertical slice |

**Dependency schedule:** Phase P1 core first; P2 timeline next; UI worker may work on a fake-note renderer contract parallel with P2. Events worker may independently design schemas once contracts freeze. Editor/Assets integrates only after relevant C++ classes build. QA starts at P1, continues each gate.

## Context budget per agent call (max 8,192 tokens)

- System + role + hard rules: ≤700.
- Task and acceptance criteria: ≤900.
- Contracts and precise interface excerpts: ≤1,600.
- Relevant source code (not whole repo): ≤2,100.
- Needed MCP tool schemas: ≤850.
- Tool output / errors: ≤1,150.
- Handoff and safety margin: ≥892.

When approaching the limit, do NOT continue carrying long conversation history. Write task status to `notes/tasks/<task-id>.md`; start a new model call with just a compact handoff and relevant excerpts. Keep MCP results selective and summarize tool call responses immediately. Set each worker to one independently verifiable change per call.

## Tool and conflict rules

- Agents NEVER invoke MCP editor tools concurrently. Coordinator grants a named exclusive lease: one agent, one editor, one tool call at a time. Release after each tool result and save.
- Independent C++ code writing may run concurrently in separate git worktrees; no shared Build.cs or public type headers without coordinator's lock.
- MCP editor action must be preceded by a `discover/inspect` action and verified afterward (compile Asset, inspect property, capture screenshot where applicable). No speculative tool names.
- For Blueprint-graph changes not exposed by stock toolsets, use UE 5.8 Python BlueprintGraphEditor / BlueprintEditorLibrary through an audited custom toolset rather than hallucinating an MCP feature. If unsupported, record manual fallback and continue with C++/data-assets-first implementation.
- No unsupervised mass delete, engine settings changes outside scope, downloads, or git history rewriting.
- When tests fail: report precise failure, change only within task ownership, and rerun related tests.

## Reusable system prompt for one worker

> You are a UE 5.8 C++ implementation worker operating on the RhythmBattle plugin. Context window: 8,192 tokens. Follow `IMPLEMENTATION_PLAN.md`. C++ owns authoritative song time, judgment and scoring; Blueprint owns authorable events/UI. You have a narrow task and assigned files. Inspect existing code before editing, never guess UE APIs, never use an MCP tool without discovering its schema. Your output must include changed paths, concise explanation, test commands and results, and remaining issues. The Unreal MCP editor is exclusive: use it only after the coordinator grants an editor lease. Do not mark completed work without successful verification.

## Initial coordinator prompt

> Implement the UE 5.8 RhythmBattle system in incremental phases from `IMPLEMENTATION_PLAN.md`. Inspect the current Unreal project first. Create a feature branch, confirm installed UE version, check game module dependencies, confirm local Unsloth API/tool calling, and connect to the UE MCP endpoint. Dispatch `TASKS.json` in dependency order. Spawn logical specialist agents where tasks do not share files. Serialize all Unreal MCP requests. Each agent has 8,192 context tokens; pass only local task scope, current public interfaces and selected code snippets, not the entire repository. Compile and run targeted tests at every phase gate. Stop and report any required manual editor operation rather than fabricating tool success.

## Suggested task-specific prompts

**Core:** Implement chart/config and deterministic note hit judge with pure testable inputs. Return only interface changes, test evidence, and risks. Do not touch widgets or BPs.

**UI:** Implement screen-space straight-line lane renderer using linear interpolation between start and hit target with decorative dots and fixed circular targets. Consume score/note snapshots; never compute hits. Test aspect ratio and reduce draw allocations.

**Events:** Implement the Blueprint event action base, an event cue dispatcher, reason-token pauses, and timed modifiers. Provide sample `Taunt`, `SpeedBoost` and `ScoreBoost` workflows; include nested pause tests.

**Editor/Assets:** Using a serialized UE MCP connection, create 2+2 demo lane layout, chart/config assets, input assets, WBP and test level bindings. Discover actual toolsets before each operation; keep screenshots / asset inspection evidence.

**QA:** Run automation and PIE smoke test; validate timing boundaries, pause/resume, active speed change, duplicate scoring, modifier cleanup, and 12-lane renderer performance. Do not accept a compile-only claim as proof of gameplay.

## Definition of done for coordinator

- Code compiles in the actual UE 5.8 project.
- Playable 30-second demo with two lanes per side and correct scores.
- Designers can modify dot speed, score tiers, hit windows, lane geometry, and event timeline in Data Assets or Blueprints.
- Example event pauses music/notes, waits for an animation callback, then resumes without lost song sync.
- At least one event temporarily changes scoring and one changes visual speed without moving hit times.
- No editor MCP or Unsloth dependency in packaged runtime.
- Automated tests and screenshot review artifacts are saved.
