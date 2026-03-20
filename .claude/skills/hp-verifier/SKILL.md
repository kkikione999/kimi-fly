---
name: hp-verifier
description: >
  Harness-Pro sub-skill: build an observability and verification scaffold for the
  project, then verify every change after it is made. Makes AI changes fully visible
  and provable. Includes baseline establishment, test execution, and actionable
  failure reports that Refactor Agent can act on directly.
---

# hp-verifier — Observability & Verification Agent

## Two Operating Modes

Verifier has two distinct modes:

1. **Scaffold mode** (first run, when project has no verification infrastructure)
2. **Verification mode** (after every refactor, runs the scaffold and reports)

---

## Mode 1: Build the Verification Scaffold

Executed once, after Cartography completes, before any refactoring begins.

### Select Tools Based on Tech Stack

Read the tech stack from the L0 doc, install accordingly:

**Test framework (required)**
```
Node.js/TS  → jest or vitest
Python      → pytest
Go          → go test (built-in)
Java        → JUnit + Maven
```

**Type checking (if language supports it)**
```
TypeScript  → tsc --noEmit
Python      → mypy
Go          → built-in
```

**Linting (required)**
```
JS/TS       → eslint
Python      → ruff
Go          → golangci-lint
```

### Establish the Baseline Snapshot

After installing tools, immediately run a full verification pass to record
the **pre-refactor baseline state**:

```markdown
## Baseline Snapshot — 2024-01-15

**Test status**: 47 passed, 12 failed, 8 skipped (pre-existing failures)
**Type errors**: 23 (pre-existing)
**Lint warnings**: 156 (pre-existing)

⚠️ The above are pre-existing issues — not introduced by Harness-Pro.
Refactoring principle: do not add new failures; eliminate existing ones incrementally.
```

**Pre-existing failures do not block refactoring.** New failures must be
fixed immediately.

### Minimum Viable Verification

If the project has absolutely no tests, Verifier must first establish
**smoke tests** before any refactoring begins:

```
Smoke test requirements (minimum):
□ Project starts successfully (process does not crash)
□ Core entry functions can be called
□ Database connection (if any) can be established
□ Key API endpoints respond with 200
```

Smoke tests passing = basic verification capability ready.

---

## Mode 2: Verify Each Change

Upon receiving a Change Report, execute this sequence:

### Verification Steps

```
Step 1: Run unit tests for the affected module
Step 2: Run type checking (if applicable)
Step 3: Run lint check
Step 4: If Steps 1–3 all pass, run integration tests (if any exist)
Step 5: Output verification report
```

**On failure, stop immediately.** Do not proceed to the next step.
Output the report with failure diagnostics.

### Verification Report Format

```markdown
## Verification Report

**Timestamp**: 2024-01-15T14:35:00Z
**Corresponding Change Report**: billing/calculator — 2024-01-15T14:32:00Z
**Status**: ✅ PASSED / ❌ FAILED

### Test Results
✅ Unit tests: 15 passed, 0 failed (+2 newly passing vs baseline)
✅ Type check: 0 errors (-1 vs baseline — refactor eliminated one type error)
⚠️ Lint: 3 warnings (pre-existing, not new)

### Change Comparison
- Before: calculateBalance() returns float
- After:  calculateBalance() returns integer
- Verified via: 5 unit test cases covering typical scenarios
- Boundary tests: zero balance ✅  negative balance (expected throw) ✅  max value ✅

### Conclusion
✅ Change is safe to merge.
Recommend Scribe update docs. Recommend Health Check re-score.
```

---

## Observability Principle

Verification is not just about pass/fail — it makes the **full impact of every
change visible** to AI:

- **Comparison format**: before vs after, not just the result
- **Coverage scope**: explicitly state what was and was not tested
- **Side effect detection**: did the change affect any module not intended to change?
- **Performance baseline**: for performance-sensitive functions, record execution time delta

---

## Failure Handling

On verification failure, Verifier outputs **localization information**,
not just a raw stack trace:

```markdown
## Verification Failure Report

**Failure point**: tests/billing/calculator.test.js:87
**Failure reason**: expected calculateBalance(100) to return 100 (integer),
                   received 100.0 (float)
**Likely cause**: missed conversion at display.js line 67 (noted in Change Report)
**Recommendation**: fix display.js:67 and re-verify — no rollback needed

**Impact assessment**: failure is in display layer only; core calculation logic is correct
```

## Actionable Feedback to Refactor Agent

Verifier always provides a fix suggestion alongside the error, never just
a description:

```
❌ Type error: verifyToken() expected to return boolean, returns object
💡 Suggestion: add `return result.valid` at auth/middleware.js:42
   OR: update L1-auth.md interface description if the change was intentional
```

This allows Refactor Agent to close the loop immediately, rather than
returning the raw error to the user.
