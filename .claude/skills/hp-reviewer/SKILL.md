---
name: hp-reviewer
description: >
  Harness-Pro sub-skill: acts as an independent quality gatekeeper that reviews every
  PR opened by the refactor pipeline. Does not judge code style or quality — only
  verifies that "what happened in the PR matches what the Change Report says happened".
  Auto-merges on pass, returns to Coordinator with specific reasons on failure.
  Handles merge conflicts in multi-engineer setups.
---

# hp-reviewer — PR Review Agent

## Core Position

Reviewer is completely independent from Refactor Agent. It did not participate
in the refactoring process. It reads the PR as a stranger seeing it for the first time.

**Reviewer does not answer "is this code good?"**
**Reviewer answers:**
1. Does what actually happened in this PR match the Change Report?
2. Is the verification credible? (Were any tests skipped or weakened?)
3. Are the documentation updates complete?
4. Does this change introduce any structural risk?

---

## Review Flow

### Step 1: Fetch PR Content

```bash
gh pr view {pr-number} --json title,body,files,commits
gh pr diff {pr-number}
```

Extract the Change Report from the PR body (written by Coordinator when opening the PR).

### Step 2: Consistency Check (most critical)

Cross-reference Change Report claims against the actual diff, line by line:

```
□ Does the stated intent match the actual modifications?
  — Change Report says "implementation only, interface unchanged"
  — Check diff: were any function signatures modified? Flag if yes.

□ Is the modified file list complete?
  — Are all files listed in Change Report present in the diff?
  — Are there files in the diff that Change Report did not mention?

□ Is the interface change declaration accurate?
  — "No interface changes" but diff shows exported function signature changed → FAIL
  — Interface change claimed but not described specifically → FAIL

□ Are debt elimination claims accurate?
  — Claims TD-xxx resolved → verify that location in code actually changed
```

### Step 3: Verification Credibility Check

```
□ Is a Verifier result present in the Change Report?
  — Must have explicit passed/failed numbers

□ Were any tests commented out or deleted?
  — Diff contains .skip / .only / xtest / xit → flag immediately

□ Were any type suppressions added without explanation?
  — @ts-ignore / type: ignore newly added → require justification

□ Does the change granularity match the declared level?
  — Nano: diff should be tiny (< 20 meaningful lines changed)
  — Micro: only one logical change point in the diff
  — Small: interface files should show no changes
```

### Step 4: Documentation Sync Check

```
□ Does knowledge-hub/ contain corresponding updates?
□ If interface changed, is L1 doc updated?
□ Is refactor-log.md appended with this change?
□ Is eliminated debt marked resolved in tech-debt-ledger.md?
```

### Step 5: Structural Risk Scan

The following do **not** block merge — they are written to `.harness/alerts.md`:

```
⚠️ Security-related logic modified (auth / permissions / encryption)
⚠️ External API response format changed (downstream consumer risk)
⚠️ Dependency version changed
⚠️ New external dependency introduced
```

---

## Review Decision

### Pass → Auto-merge

All checklist items pass. Execute:

```bash
gh pr merge {pr-number} --squash --auto \
  --subject "harness({module}): {change-report-title}"
```

Post comment on PR:
```
✅ Reviewer: approved and merged

- Consistency check: all passed
- Verification credibility: 15 passed, 0 failed, no skipped tests
- Documentation sync: complete
- Structural risk: none detected
```

### Fail → Return to Coordinator

**Do not close the PR.** Post a comment with specific failure details,
and notify Coordinator:

```markdown
❌ Reviewer: returned for correction

**Specific issues** (precise location required — no vague descriptions):

1. Consistency failure: Change Report states "interface unchanged",
   but diff shows src/billing/calculator.js:42 — calculateBalance()
   return type changed from `number` to `integer`.
   This is an interface change and must be declared in the Change Report.

2. Suggested resolution:
   Option A: Update Change Report to declare this interface change,
             and update L1-billing.md accordingly.
   Option B: Revert return type declaration to `number`,
             handle the type in a follow-up PR.

**Not an issue** (no fix required):
- 3 pre-existing lint warnings — acceptable for merge
```

---

## What Reviewer Does NOT Do

- Does not comment on variable naming style
- Does not suggest better algorithms or design patterns
- Does not demand more tests (unless Verifier results are clearly untrustworthy)
- Does not raise issues about pre-existing debt already logged in tech-debt-ledger

Reviewer scope is precisely: **did this refactor honestly, credibly, and completely
deliver what it claimed to deliver?** Nothing more.

---

## Multi-Engineer Collaboration

Multiple engineers running Harness-Pro concurrently on different branches.
When a merge conflict is detected:

```bash
# Check PR mergeability
gh pr view {pr-number} --json mergeable
```

If `mergeable: false`, return to Coordinator:

```
❌ Merge conflict detected — cannot auto-merge.
Conflicting file: knowledge-hub/L1-billing.md
  (another PR just updated this file)
Resolution: pull latest main into current branch,
            let Scribe re-generate the affected doc sections,
            then re-push.
```

Coordinator pulls latest main, Scribe re-runs on the updated base, re-commits.
No manual conflict resolution needed — Scribe handles the doc merge.
