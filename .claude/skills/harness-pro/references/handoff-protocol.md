# Handoff Protocol: Planning Layer → Execution Layer

This document defines the contract between hp-planner (Prometheus) and
the execution layer (Atlas). The approved plan file is the **only**
communication channel between these two layers.

---

## The Handoff Artifact

When hp-planner completes, it writes one file:

```
.harness/plans/{YYYY-MM-DD}-{module-name}.md
```

This file is the complete instruction set for Atlas.
Atlas reads this file once at the start of a module's execution lifecycle
and uses it as its primary source of truth for the entire cycle.

**Atlas does not communicate with Prometheus during execution.**
If Atlas encounters something that contradicts the plan, it does not
ask Prometheus — it logs a structural alert and adapts within scope.

---

## Plan File: What Atlas Requires

Atlas will refuse to start execution if any of these fields are missing
or ambiguous in the plan file:

### Required Fields (Atlas validates these on load)

```markdown
---
type: refactor-plan
status: approved              ← must be "approved", not "draft"
module: {module-name}
health-score-baseline: {N}    ← current score before refactoring
health-score-target: {N}      ← target score to reach
accuracy-reviewed: {bool}     ← was Momus loop run?
---
```

### Required Sections

```
1. Objective          — one paragraph, plain language
2. Explicitly Out of Scope — list of what will NOT be touched
3. Success Criteria   — measurable, not vague
4. Risk Notes         — Metis findings, incorporated
5. Task Sequence      — phases with individual atomic tasks
6. Agent Assignments  — which agent handles which phase
7. Rollback Points    — where execution can safely stop and undo
```

If any section is missing: Atlas writes to `.harness/alerts.md` and halts.
It does not attempt to infer missing information.

---

## Task Sequence Contract

The task sequence is the most critical section. Each task must be:

**Atomic** — completable and verifiable in a single subagent invocation.
If a task requires two agents, it must be split into two tasks.

**Labeled with granularity** — every task declares its level:
```
[Nano] Add type annotation to calculateBalance()
[Micro] Extract magic number 100 to CENTS_PER_DOLLAR constant
[Small] Refactor calculateBalance() internal logic (interface unchanged)
[Medium — Oracle required] Break circular dep: billing ↔ user-profile
```

**Assigned to an agent** — every task names who executes it:
```
[Nano] Add type annotation to calculateBalance()
       → Executor: Backend-TS subagent (via Atlas)
       → Verifier: hp-verifier
```

**Has a stable state** — execution can pause after any task and the
codebase is in a working state. No task should leave the code broken
as a required intermediate step.

---

## What Atlas Does on Plan Load

When Atlas first reads an approved plan, it executes this checklist
before spawning any subagents:

```
□ Validate all required frontmatter fields are present
□ Validate all required sections exist
□ Confirm .harness/agents/ directory exists (hp-agent-factory has run)
□ Confirm Knowledge Hub L1 doc exists for this module
□ Confirm hp-verifier baseline snapshot exists
□ Load session-learnings.md (empty on first run — that's fine)
□ Identify which tasks can run in parallel (tasks with no shared files)
□ Set internal task pointer to Phase 1, Task 1
```

Only after all checks pass does Atlas begin execution.

---

## Mid-Execution Plan Amendments

Occasionally Atlas discovers something during execution that contradicts
the plan (e.g., Oracle recommends a different approach than Metis suggested,
or Explore finds the scope is larger than expected).

Atlas handles this as follows:

**Minor amendments** (same scope, different approach):
Atlas updates the plan file directly, logs the change in `refactor-log.md`,
and continues. No need to involve Prometheus.

```markdown
## Plan Amendment [2024-01-15T15:10:00Z]
Task 7 original: Extract UserReference to shared/types.ts
Amendment: Extract UserReference AND BalanceRecord together to shared/types.ts
Reason: Oracle finding — both types are used by notification module,
        separating them would create a new dependency
Approved by: Atlas (within original scope — shared/ extraction strategy unchanged)
```

**Scope-changing discoveries** (more modules affected than planned,
new invariants found, architectural approach needs to change):
Atlas logs a structural alert, pauses execution on the affected tasks,
and continues with unaffected tasks. Human reviews alert and decides.

```markdown
## [ALERT] Plan scope underestimated
Module: billing
Discovery: notification module also imports from billing (not in plan)
           — full circular dep resolution affects 3 modules, not 2
Impact: Phase 3 requires additional task: update notification module imports
Decision needed: authorize scope expansion, or defer notification to next plan?
```

---

## Execution Complete → Handoff Back to Coordinator

When Atlas finishes all tasks in the plan (or all non-blocked tasks),
it writes an execution summary and hands back to the Coordinator:

```markdown
---
type: execution-summary
plan: .harness/plans/2024-01-15-billing.md
completed-at: 2024-01-15T18:30:00Z
---

## Execution Summary — billing module

### Tasks Completed: 9 / 10
✅ Phase 1 (Nano): 3/3 tasks
✅ Phase 2 (Micro): 3/3 tasks
✅ Phase 3 (Small): 2/3 tasks
⏭️ Task 8 skipped: scope expansion alert pending human decision

### Health Score: 38 → 74 (+36)
Target was 80. Gap remaining: 6 points.
Blocked by: Phase 3 Task 8 (notification module scope question)

### PRs Merged: 4
- PR #47: Phase 1 (Nano) — merged ✅
- PR #48: Phase 2 (Micro, Part 1) — merged ✅
- PR #49: Phase 2 (Micro, Part 2) — merged ✅
- PR #50: Phase 3 (Small) — merged ✅

### Session Learnings Accumulated: 6
See: .harness/session-learnings.md

### Coordinator Decision Required:
A) Authorize notification module scope expansion → resume Phase 3 Task 8
B) Accept 74/80 and move to next module (notification module gets its own plan)
C) Close this plan and re-plan billing with expanded scope
```

The Coordinator reads this summary, makes the decision (or escalates to human
if it's a structural alert), and either resumes Atlas or starts the next module.
