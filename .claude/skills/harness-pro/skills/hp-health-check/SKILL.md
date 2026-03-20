---
name: hp-health-check
description: >
  Harness-Pro sub-skill: quantitatively measure refactoring progress using a Health
  Score. Establishes a baseline before refactoring starts, tracks score changes after
  each module is completed, determines whether a module has reached "AI-maintainable"
  status, and pushes results to the human observation layer.
---

# hp-health-check — Quantitative Health Scoring

## Core Idea

The condition for stopping refactoring cannot be "feels about right." It must be
measurable. Health Check provides a scoring system so the Coordinator has an
objective basis for deciding "is this module done?"

---

## Health Score Calculation

Total: 100 points across 5 dimensions.

### 1. Documentation Coverage (25 pts)

```
Measured by:
- L1 docs covering public functions / total public functions
- Proportion of docs that include an intent description (not empty)
- All entries in invariants.md have a status marker

Full score conditions:
- All public interfaces have L1 documentation
- All docs have intent descriptions
- All invariants are marked (🟢 Confirmed or 🔴 Unconfirmed)
```

### 2. Test Coverage (25 pts)

```
Measured (data from Verifier):
- Line coverage percentage
- Whether critical paths have explicit tests

Scoring:
≥ 80% line coverage     → 25 pts
60–79%                  → 18 pts
40–59%                  → 10 pts
Smoke tests only         → 5 pts
No tests at all          → 0 pts
```

### 3. Type Safety (20 pts)

```
Scoring:
0 type errors (tsc / mypy)     → 20 pts
1–5 type errors                → 14 pts
6–20 errors                    → 7 pts
> 20 errors or untyped dynamic → 3 pts
```

### 4. Module Cohesion (15 pts)

```
Evaluated via code analysis:
- Do functions have single responsibility? (sample check)
- Are there circular dependencies?
- Is cross-module coupling through documented interfaces?

Full score conditions:
- 80%+ of sampled functions are single-responsibility
- No circular dependencies
- Cross-module calls go through L1-documented interfaces
```

### 5. AI Readability (15 pts)

```
Self-evaluated: AI attempts to read a section of code and scores:
- Can the intent of a function be understood in 5 seconds?
- Are variable/function names self-explanatory?
- Have magic numbers been eliminated?
- Are side effects explicitly annotated?

Full score conditions:
- Most functions are understandable without reading comments
- No magic numbers
- Side effects annotated
```

---

## Score Report Format

```markdown
## Health Score Report

**Project**: my-ecommerce-app
**Evaluated**: 2024-01-15
**Scope**: billing module

### Total: 72 / 100

| Dimension | Score | Max | Notes |
|-----------|-------|-----|-------|
| Doc coverage | 20 | 25 | 3 functions missing intent description |
| Test coverage | 18 | 25 | 68% line coverage |
| Type safety | 20 | 20 | 0 type errors ✅ |
| Module cohesion | 9 | 15 | 1 circular dependency found (TD-015) |
| AI readability | 5 | 15 | Multiple magic numbers remain |

### vs Baseline
Baseline (pre-refactor): 38 pts
Current: 72 pts
Delta: +34 ↑

### Gap to Target
Target score: 80 pts (set by Coordinator)
Gap: -8 pts
Recommended next steps:
  → Eliminate magic numbers (estimated +6 pts, low effort)
  → Complete 3 missing function docs (estimated +4 pts, low effort)
```

---

## Target Score Guidelines

Coordinator sets the target score at project start. Recommended standards:

| Scenario | Target |
|---------|--------|
| Minimum viable improvement | 60 pts |
| AI can maintain stably | 75 pts |
| AI can extend independently | 85 pts |
| High-quality production system | 90 pts |

**Default target: 75 pts** (AI-maintainable)

---

## Recommendation to Coordinator

Health Check must end with a clear **continue / done** recommendation:

```markdown
## Recommendation

✅ billing module has reached target score (72 ≥ 70, target lowered for legacy core)
→ Mark this module complete. Proceed to next module.

or

⏳ billing module has not reached target (72 < 80)
→ Priority action: eliminate magic numbers (est. +6 pts, low risk)
→ Defer: resolve circular dependency (est. +6 pts, higher risk — next cycle)
```

---

## Write to Observation Layer

After every scoring run, append a machine-readable entry to
`.harness/observation-log.jsonl` (consumed by the human dashboard):

```json
{
  "timestamp": "2024-01-15T16:00:00Z",
  "event": "health_check",
  "module": "billing",
  "score_before": 38,
  "score_after": 72,
  "delta": 34,
  "target": 80,
  "reached_target": false,
  "next_recommended_action": "eliminate magic numbers (est. +6 pts)"
}
```

Humans can build a real-time dashboard from this file alone, without
touching the codebase.

---

## Structural Alerts

The following go to `.harness/alerts.md` in addition to the observation log:

```markdown
## [ALERT-2024-01-15] Health Score Stalled

**Module**: user-profile
**Pattern**: 3 consecutive refactors — 41 → 43 → 42 → 43, no meaningful progress
**Analysis**: TD-015 (circular dependency) is a structural blocker preventing
              improvement on all other dimensions
**Human decision needed**: authorize Medium-level refactor (split module)?
**Impact**: splitting will affect interfaces in billing and notification modules
```

This is the most important signal the system sends to humans. Everything else
AI handles autonomously. Only directional blockers like this escalate.
