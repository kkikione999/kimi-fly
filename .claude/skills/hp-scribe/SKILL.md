---
name: hp-scribe
description: >
  Harness-Pro sub-skill: automatically update the Knowledge Hub after every code
  change. Triggered by Refactor Agent completing a change — reads the Change Report
  to determine which documents to update. Ensures docs and code are always committed
  atomically. Never allows documentation to fall behind the code.
---

# hp-scribe — Automatic Documentation Agent

## Core Responsibility

Scribe does not scan code proactively.
It does one thing: **read the Change Report and update exactly the affected documents.**

This design guarantees two properties:
- **Precision**: only updates what actually changed, never rewrites unaffected content
- **Efficiency**: no redundant code scanning, derives all information from Change Report

---

## Trigger Condition

Every time Refactor Agent completes a change, it must:
1. Output a Change Report (format: `references/change-report-schema.md`)
2. Immediately trigger Scribe to process that report

**Scribe should never be triggered manually to "batch update docs"** — if that's
needed, it means previous refactors didn't produce correct Change Reports. Fix the
process first.

---

## Processing Flow

### Step 1: Parse the Change Report

Read the Change Report and extract:
```
- Which files were modified (path list)
- Type of change (new function / deleted function / interface change / rename / move)
- Change intent (one-sentence description written by Refactor Agent)
- Does it affect public interfaces? (yes = L1 doc update required)
- Does it eliminate known technical debt? (yes = tech-debt-ledger update required)
```

### Step 2: Determine Which Docs to Update

| Change Type | Docs to Update |
|------------|---------------|
| Implementation changed (interface unchanged) | L2 doc (if it exists) |
| Function signature changed | L1 doc + L2 doc |
| New module added | L0 (module map) + new L1 |
| Module deleted | L0 + mark that L1 as deprecated |
| Technical debt eliminated | tech-debt-ledger (mark resolved) |
| New business invariant discovered | invariants.md |
| Any change | refactor-log.md (append entry) |

### Step 3: Execute Updates

**Only modify the affected sections of each document — never rewrite the whole file.**

For each document that needs updating:
1. Locate the specific section that needs to change
2. Apply the modification (old content → new content)
3. Update the `last_updated` timestamp in the document frontmatter
4. Append a record to `refactor-log.md`

**Critical: Scribe's output is never committed independently.**
After Scribe finishes updating all documents, it notifies the Coordinator.
Coordinator then packages code changes + doc updates into a **single `git commit`**.
This is the atomicity guarantee — it is impossible for code to merge without
its documentation update.

```bash
# Executed by Coordinator — includes both Refactor and Scribe outputs
git add src/ knowledge-hub/
git commit -m "harness(billing): convert balance storage from float to integer

- Change type: Micro
- Debt eliminated: TD-003 (float precision issue)
- Verifier: 15 passed, 0 failed
- Docs: L1-billing.md, tech-debt-ledger.md synced"
```

### Step 4: Consistency Check

After updates are complete, verify:
```
□ Module list in L0 matches actual directory structure
□ Interface descriptions in L1 match actual code interfaces
□ Code location references in invariants.md are still valid
□ "Resolved" markers in tech-debt-ledger are accurate
```

If inconsistency is found, append a `⚠️ Doc Drift Warning` to `.harness/alerts.md`
and report to Coordinator for a decision.

---

## refactor-log.md Entry Format

Append one entry per refactor:

```markdown
## [2024-01-15 14:32] Module: billing/calculator

**Change**: Convert balance calculation from float to integer (cents)
**Affected files**: billing/calculator.js, billing/display.js
**Docs updated**: L1-billing.md (interface), tech-debt-ledger.md (TD-003 resolved)
**Refactored by**: hp-refactor
**Verification**: ✅ passed (confirmed by hp-verifier)

---
```

---

## Doc Drift Detection

If Scribe detects any of the following, it must report to Coordinator
and **not attempt to self-resolve**:

- A function exists in code but has no documentation anywhere (likely missed)
- A documented interface does not match the actual code interface (drift)
- A location reference in `invariants.md` points to deleted code

Report format:
```markdown
⚠️ Doc Drift Warning
Detected: 2024-01-15
Location: src/auth/middleware.js
Issue: L1-auth.md states verifyToken() returns boolean,
       but actual code returns {valid: boolean, userId: string}
Recommendation: Cartographer re-scan auth module L1
```
