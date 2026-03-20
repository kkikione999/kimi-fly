---
name: hp-planner
description: >
  Harness-Pro sub-skill: the planning layer that transforms a user's refactoring
  intent into a precise, executable plan. Runs before any code is touched.
  Contains three internal agents — Prometheus (planner/interviewer), Metis
  (gap analyst), and Momus (high-accuracy reviewer). Trigger when a user
  describes what they want to improve in a project, before any refactoring
  begins. Also trigger when an existing plan needs to be revised.
---

# hp-planner — Planning Layer

## Three Internal Agents

| Agent | Role | When Active |
|-------|------|------------|
| **Prometheus** | Planner & Interviewer — drives the whole planning session | Always |
| **Metis** | Consultant — mandatory gap analysis before plan is written | Before every plan draft |
| **Momus** | Reviewer — high-accuracy loop, approves or rejects the plan | When user requests high accuracy |

These are not separate processes. They are **three reasoning modes** that
Prometheus moves through during the planning session.

---

## The Planning State Machine

```
User describes work
      ↓
[INTERVIEW MODE — Prometheus]
  Launch Explore + Librarian in background to gather codebase context
  Ask clarifying questions — one at a time, not all at once
  After each user response → run Clearance Check
      ↓
[CLEARANCE CHECK]
  □ Core objective defined?          (what is the end state?)
  □ Scope boundaries established?    (what is explicitly OUT of scope?)
  □ No critical ambiguities?         (no "it depends" left unresolved)
  □ Technical approach decided?      (which strategy, not just what)
  □ Test strategy confirmed?         (how will we know it worked?)
  
  All 5 clear? → proceed to METIS CONSULT
  Any unclear? → back to INTERVIEW
      ↓
[METIS CONSULT — mandatory, always runs]
  Gap analysis on the gathered requirements:
  - What assumptions are we making that haven't been verified?
  - What risks has the interview not surfaced?
  - What dependencies between tasks could cause ordering problems?
  - Is the scope realistic given the codebase's current Health Score?
  Metis findings are incorporated into the plan draft.
      ↓
[PLAN GENERATION — Prometheus writes the plan]
  Output: .harness/plans/{YYYY-MM-DD}-{module-or-feature}.md
  Present plan to user.
      ↓
[USER CHOICE]
  "Looks good" → DONE — guide user to /start-work
  "I want high accuracy" → MOMUS LOOP
      ↓
[MOMUS LOOP]
  Momus reviews the plan against strict quality criteria
  OKAY → DONE
  REJECT → Prometheus revises → Momus reviews again
  Max 3 iterations. If still REJECT after 3: escalate specific issues to user.
```

---

## Prometheus — Interview Guidelines

### Goal of the Interview

Not to gather every possible detail. To reach **minimum viable clarity** —
the 5 Clearance Check conditions. Stop interviewing the moment all 5 are met.

### Parallel Research During Interview

While interviewing, immediately launch background tasks:

```
Launch Explore agent: "Find all modules related to {user's stated goal}"
Launch Librarian agent: "Research best practices for {refactor type} in {tech stack}"
```

These run in parallel with the interview. Explore and Librarian results
are injected into Prometheus's context as they arrive, enriching the questions
and reducing the need for the user to explain technical context.

### Interview Principles

- **One question at a time.** Never ask multiple questions in one message.
- **Most important question first.** If the user can only answer one thing, make it count.
- **Use what you already know.** Cartography scanned the codebase. Don't ask the user
  to describe the code — ask about intent, priorities, and constraints.
- **Offer concrete options.** Instead of "what approach do you prefer?",
  say "should we go with approach A (safer, slower) or B (riskier, faster)?"

### Good Interview, Bad Interview

```
❌ Bad: "Can you tell me more about what you want to improve?"
✅ Good: "The billing module has 3 open issues: TD-003 (float precision),
          TD-015 (circular dep with user-profile), and no type coverage.
          Which is most urgent to you?"

❌ Bad: "What's the scope? What are the constraints? What's the timeline? 
         What's the test strategy?"
✅ Good: "I can see billing is the highest-debt module. Should we limit
          this refactor to billing only, or also fix the circular dep
          which would require touching user-profile?"
```

---

## Metis — Gap Analysis Protocol

Metis runs **once**, after Clearance Check passes, before the plan is written.
It is not optional — even if the interview went smoothly.

### Metis's Seven Questions

Metis systematically interrogates the gathered requirements:

```
1. ASSUMPTIONS
   "We are assuming {X}. Is this verified or just implied?"
   Flag every assumption not explicitly confirmed by the user.

2. HIDDEN DEPENDENCIES
   "Task B cannot start until Task A completes because..."
   Identify non-obvious ordering constraints between planned tasks.

3. SCOPE CREEP RISKS
   "Achieving goal X will likely surface Y, which will tempt expansion."
   Name the most likely distractions before they happen.

4. REVERSIBILITY
   "If this goes wrong at step N, how do we roll back?"
   Every irreversible step (schema migrations, API contract changes) needs
   an explicit rollback plan in the output.

5. HEALTH SCORE REALISM
   "The current Health Score baseline is {N}. The target is {M}.
    Given the debt profile, is this achievable in one planning cycle?"
   Flag if the gap is too large — suggest phasing.

6. AGENT TEAM ADEQUACY
   "Given this plan, does the generated agent team have the right specialists?"
   If the plan requires DBA work but no DBA agent was generated, flag it.

7. KNOWN UNKNOWNS
   "There are parts of the codebase marked ⚠️ intent unclear in Cartography.
    Does this plan depend on any of them?"
   Flag unclear-intent code that sits on the critical path.
```

Metis output is a short structured list of findings — not prose, not suggestions.
Each finding is either BLOCKING (must be resolved before plan is written)
or ADVISORY (incorporated into the plan as a risk note).

---

## Plan Format

Plans are saved to `.harness/plans/` as Markdown files.

```markdown
---
type: refactor-plan
created: 2024-01-15T09:00:00Z
module: billing
status: approved  # draft | approved | in-progress | done
accuracy-reviewed: true  # was Momus loop run?
health-score-target: 80
---

# Refactor Plan — billing module

## Objective
Convert billing module from float-based to integer-based balance storage,
eliminate circular dependency with user-profile, and bring type coverage to 100%.

## Explicitly Out of Scope
- Changing the billing API's external response format
- Migrating historical balance data (separate plan required)
- user-profile module changes beyond what's needed to break the circular dep

## Success Criteria
- Health Score ≥ 80 (currently 38)
- 0 type errors in billing module
- Circular dependency with user-profile eliminated
- All existing tests pass + new tests for float→integer conversion

## Risk Notes (from Metis)
- ⚠️ display.js has ⚠️ intent unclear annotation — verify before touching
- ⚠️ worker/settlement.js depends on billing; its tests must be included in Verifier scope
- ⚠️ Scope creep risk: fixing circular dep will reveal notification module also needs updating

## Task Sequence

### Phase 1: Nano (no risk, do first)
1. Add type annotations to all public functions in billing/calculator.js
2. Extract magic numbers to named constants in billing/calculator.js
3. Add intent comments to the 3 unclear functions in billing/display.js

### Phase 2: Micro (single logic changes)
4. Convert calculateBalance() from float to integer return type
5. Update display.js to handle integer input (conversion to dollars)
6. Add error handling to deductBalance() for negative amounts

### Phase 3: Small (requires L1 doc update)
7. Extract shared UserReference type to shared/ module
   (prerequisite for breaking circular dep)
8. Update billing to import UserReference from shared/ instead of user-profile
9. Verify user-profile no longer needs to import from billing

### Phase 4: Verify & Close
10. Full Verifier run across billing + user-profile + worker/settlement
11. Health Check re-score
12. Update L1 docs for all affected modules

## Agent Assignments
- Phase 1–2: hp-refactor (via Atlas + Backend-TS)
- Phase 3: Oracle consult required before execution (architectural change)
- Phase 4: hp-verifier + hp-health-check

## Rollback Points
After Phase 2: if Verifier fails, revert calculator.js only (display.js is independent)
After Phase 3: if circular dep fix breaks tests, revert shared/ extraction
               (billing can temporarily re-import from user-profile)
```

---

## Momus — High-Accuracy Review Loop

Momus is activated when the user says they want high accuracy.
It reviews the plan draft against strict criteria.

### Momus Quality Criteria

```
STRUCTURE
□ Every task is atomic — can be done and verified independently
□ Tasks are ordered by dependency (no task requires an incomplete predecessor)
□ Each phase is clearly separated — no Micro task hiding in a Nano phase

COMPLETENESS
□ All Metis BLOCKING findings are resolved in the plan
□ All Metis ADVISORY findings appear as risk notes
□ Success criteria are measurable, not vague ("tests pass" not "quality improves")
□ Rollback points exist for every irreversible step

SCOPE INTEGRITY
□ Every task in the plan maps directly to the stated objective
□ Nothing in the plan is "while we're here" expansion
□ Out-of-scope section explicitly names the temptations

AGENT READINESS
□ Every task has an agent assignment
□ No task requires a capability not present in the generated agent team
□ Oracle consults are explicitly marked where architectural judgment is needed

REALISM
□ Phase 1 (Nano) contains at least 2 tasks
   (if a plan jumps straight to Micro, it skipped the trust-building step)
□ No single phase contains more than 5 tasks
   (too many tasks in one phase = insufficient decomposition)
□ Health Score target is within 30 points of baseline
   (larger gaps require phasing into multiple plans)
```

### Momus Decision

**OKAY**: All criteria met. Momus outputs:
```
✅ OKAY — plan approved
No blocking issues found.
Advisory: [any notes worth surfacing to the user]
```

**REJECT**: One or more criteria failed. Momus outputs:
```
❌ REJECT — {N} issues found

Issue 1 [STRUCTURE]: Task 4 and Task 5 are not atomic — they must both
  be done to restore a working state. Solution: combine into one task
  or add an intermediate stable state.

Issue 2 [COMPLETENESS]: Metis flagged worker/settlement.js as in Verifier
  scope, but Phase 4 only mentions billing + user-profile in the Verifier run.

Prometheus: please revise and resubmit.
```

Prometheus revises the plan addressing each REJECT reason.
Momus reviews again. Maximum 3 iterations.

If after 3 iterations the plan still has REJECT items, Momus surfaces them
to the user directly — these are genuinely ambiguous decisions that require
human judgment, not just better planning.

---

## Output

When planning is complete (user accepts or Momus approves):

```
Plan saved: .harness/plans/2024-01-15-billing.md
Status: approved

Next step: run /start-work to hand this plan to Atlas.
Atlas will read this plan and begin coordinating the agent team.
```

The plan file is the **handoff document** from the Planning Layer to
the Execution Layer (Atlas). Atlas reads it as its primary instruction source.
