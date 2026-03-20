# Subagent Invocation Protocol

This document defines how Atlas spawns and communicates with all subagents.
Every subagent invocation in Harness-Pro follows this protocol — no exceptions.

---

## Execution Model: Who Is Persistent, Who Is Temporary

```
PERSISTENT (one instance, lives for the entire module lifecycle)
└── Atlas
      Holds: approved plan, execution state, session learnings,
             current task queue, accumulated findings

SUBAGENTS (spawned on demand, released after task completes)
├── Explore         — codebase search
├── Librarian       — docs + OSS research
├── Oracle          — architecture advice
├── Frontend        — UI component refactoring
├── Backend-{lang}  — server-side refactoring
├── DBA             — database schema safety
├── hp-verifier     — run verification suite
├── hp-scribe       — update Knowledge Hub docs
└── hp-health-check — re-score the module
```

Atlas is the only persistent agent in the execution layer.
All other agents are **stateless workers** — they receive a task,
complete it, return a structured result, and are released.
They retain no memory between invocations.

---

## Standard Invocation Structure

Every time Atlas spawns a subagent, the invocation contains exactly
three sections — no more, no less:

```
INVOKE {agent-name}

ROLE:
  {contents of .harness/agents/{agent-name}.md}
  This defines the agent's specialty, tools, and output format.

TASK:
  {specific task for this invocation}
  Must be atomic — completable and verifiable in one pass.
  Must include all context the agent needs (no "go figure it out yourself").

CONSTRAINTS:
  - Must not modify files (read-only agents: Explore, Librarian, Oracle)
  - Must not read files outside: {explicit list}
  - Output format: {structured format from agent config}
  - Max output length: {tokens or lines}
  - If uncertain: return "UNCERTAIN: {reason}" rather than guessing
```

### Example: Atlas invokes Explore

```
INVOKE explore

ROLE:
  [contents of .harness/agents/explore.md]

TASK:
  Find all locations in the codebase that call calculateBalance().
  Project root: /src
  Language: TypeScript
  I need: file path, line number, 3 lines of surrounding context for each call site.

CONSTRAINTS:
  - Read-only. Do not modify any files.
  - Search scope: /src only (not /node_modules, not /tests)
  - Output format: structured list (file:line — context)
  - Max results: 30. If more exist, report count and stop.
  - If zero results: return "NOT FOUND" explicitly.
```

### Example: Atlas invokes Oracle

```
INVOKE oracle

ROLE:
  [contents of .harness/agents/oracle.md]

TASK:
  The billing module imports from user-profile (for UserReference type),
  and user-profile imports from billing (for BalanceRecord type).
  This is a circular dependency. I need your recommendation on how to
  break it safely.

  Current module graph (relevant portion):
  billing → user-profile (imports: UserReference at billing/calculator.js:3)
  user-profile → billing (imports: BalanceRecord at user-profile/service.js:8)

  Constraints I cannot change:
  - INV-001: balance precision (integer/cents) must be maintained
  - The external API response format cannot change (downstream consumers)

  Options I have considered:
  A) Extract both types to shared/types.ts — each module imports from shared
  B) Move UserReference into billing itself (it only has 2 fields)

CONSTRAINTS:
  - Advise only. Do not write any code.
  - Output format: structured recommendation (see oracle.md)
  - If neither option is clearly better: say so and explain the trade-off.
```

---

## Parallel Invocation

Atlas can spawn multiple subagents in parallel when their tasks are independent.

**Safe to parallelize:**
```
Explore (searching module A) + Librarian (researching a library)
  → no shared state, no write operations, results are independent
```

**Must be sequential:**
```
Explore finds all call sites
  → THEN Atlas decides scope
  → THEN Backend-TS refactors
  → THEN hp-scribe updates docs
  → THEN hp-verifier validates
```

Atlas declares parallel invocations explicitly:

```
PARALLEL INVOKE:
  [1] explore — find all callers of calculateBalance()
  [2] librarian — research TypeScript integer type best practices

Await both. Merge results before proceeding.
```

---

## Result Contract

Every subagent returns a result in this structure:

```
RESULT from {agent-name}
Status: SUCCESS | UNCERTAIN | FAILED

{structured output per agent's output format}

Learnings:
  {anything Atlas should remember for future tasks in this session}
  (can be empty — "none" is a valid learning)
```

Atlas **always** reads the Learnings field and appends non-empty entries
to `.harness/session-learnings.md` before proceeding to the next task.

### On UNCERTAIN results

If a subagent returns UNCERTAIN, Atlas must not proceed with that task.
Options in order of preference:
1. Provide more context and re-invoke the same agent
2. Invoke a different agent to resolve the uncertainty first
3. If uncertainty is architectural: invoke Oracle
4. If Oracle also returns UNCERTAIN: log as structural alert, skip task, continue elsewhere

### On FAILED results

If a subagent returns FAILED:
1. Retry once with additional context
2. If second attempt fails: log to `.harness/alerts.md`, skip task
3. Never silently ignore a failure

---

## Context Budget Rules

Each agent has a declared `context-budget` in its config. Atlas must respect it.

| Budget Level | What Atlas Injects |
|-------------|-------------------|
| `low` | Task description + direct code snippets only (no full files) |
| `medium` | Task + relevant L1 doc section + code snippets |
| `high` | Task + full L1 doc + session learnings + related invariants |

**The most common mistake:** giving a low-budget agent (Explore, Librarian)
the full Knowledge Hub context. It wastes tokens and degrades output quality.
Explore does not need to know the business invariants. It needs a search query.

---

## Session Learnings Format

`.harness/session-learnings.md` — written by Atlas, read at the start
of every subsequent Atlas context window in this module's lifecycle.

```markdown
## Learning [2024-01-15T14:45:00Z] — from Explore

calculateBalance() is called in 7 locations, including 2 test files.
Changing its return type affects: calculator.js, display.js, settlement.worker.js,
and tests/billing/calculator.test.js, tests/integration/settlement.test.js.
What seemed Micro is actually Small — update scope estimate for similar changes.

---

## Learning [2024-01-15T15:10:00Z] — from Oracle

Circular dep resolution via shared/ extraction is the correct path.
Option B (move UserReference into billing) was rejected because UserReference
is also used by the notification module — moving it would create a new dep.
Shared/ extraction affects 3 modules total, not 2. Adjust Phase 3 task count.

---
```

These learnings are the "memory" of the execution session. They are what
makes Atlas smarter as the refactor progresses, without needing persistent
agent state across all workers.
