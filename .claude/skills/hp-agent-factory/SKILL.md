---
name: hp-agent-factory
description: >
  Harness-Pro sub-skill: reads hp-cartography's L0 output and generates
  a project-specific agent team. Produces one config file per agent in
  .harness/agents/. Atlas is always generated as the persistent orchestrator.
  All other agents are generated as subagent templates — stateless workers
  that Atlas spawns, uses, and releases per the subagent protocol.
  Run once after Cartography. Re-run if tech stack changes significantly.
---

# hp-agent-factory — Agent Team Generator

## Execution Model (read this first)

Harness-Pro uses a **mixed execution model**:

```
PERSISTENT (one instance, lives for the full module lifecycle)
└── Atlas — the only persistent agent in the execution layer
      Holds all state: plan progress, task queue, session learnings

SUBAGENTS (spawned per task, stateless, released after completion)
├── Explore, Librarian, Oracle   — advisory workers
├── Backend-{lang}, Frontend, DBA — specialist code workers
└── hp-refactor, hp-scribe, hp-verifier, hp-health-check — pipeline workers
```

hp-agent-factory generates **config files**, not running processes.
Each config is a role specification that Atlas loads when spawning that subagent.
See `references/subagent-protocol.md` for the invocation protocol.

---

## Input

Reads from Knowledge Hub (must exist before running):
- `knowledge-hub/L0-project-map.md` — tech stack, module count, entry points
- `knowledge-hub/tech-debt-ledger.md` — debt type and volume
- `knowledge-hub/invariants.md` — presence and complexity of invariants

---

## Output Structure

```
.harness/agents/
├── _team-manifest.md     ← always: summary of all active agents
├── atlas.md              ← always: persistent orchestrator config
├── explore.md            ← always: codebase search subagent
├── librarian.md          ← always: docs/OSS research subagent
├── oracle.md             ← conditional: architecture advisor
├── backend-{lang}.md     ← conditional: per detected backend language
├── frontend.md           ← conditional: if frontend framework detected
├── dba.md                ← conditional: if migrations directory found
└── devops.md             ← conditional: if CI/CD config found
```

---

## Atlas Config (always generated)

Atlas config is the most detailed — it defines the persistent orchestrator.

```markdown
---
agent: atlas
execution-model: persistent
lifespan: module-lifecycle
context-budget: high
parallel: false
reads: .harness/plans/*.md, knowledge-hub/, .harness/session-learnings.md
writes: .harness/session-learnings.md, .harness/alerts.md
subagent-protocol: references/subagent-protocol.md
---

# Atlas — Execution Orchestrator

## Role
Atlas does not write code. Atlas coordinates the agents that write code.
Atlas holds the full execution state for one module's lifecycle.

## On Startup (plan load)
1. Validate plan file (see references/handoff-protocol.md for checklist)
2. Load session-learnings.md (empty on first run — acceptable)
3. Confirm .harness/agents/ contains required subagent configs
4. Confirm Knowledge Hub L1 doc exists for target module
5. Identify parallelizable tasks in Phase 1

## Task Delegation Rules
[Generated from L0 — which agents are available for this project]
| Task Type                    | Subagent          |
|------------------------------|-------------------|
| Find usages / search code    | Explore           |
| Research library / OSS       | Librarian         |
| Architectural decision       | Oracle            |
| Backend code change          | Backend-{lang}    |
| Frontend component change    | Frontend          |
| Schema / migration safety    | DBA               |
| Write the actual code change | hp-refactor       |
| Update documentation         | hp-scribe         |
| Run verification suite       | hp-verifier       |
| Score the module             | hp-health-check   |

## Wisdom Accumulation
After every subagent returns a result, Atlas reads the Learnings field.
Non-empty learnings are appended to .harness/session-learnings.md immediately.
Session learnings are prepended to every subsequent Atlas context window.

## Completion
When all plan tasks are done (or non-blocked), write Execution Summary
to .harness/execution-summary-{module}.md and notify Coordinator.
Format: see references/handoff-protocol.md
```

---

## Agent Selection Logic

### Always Generated

| Agent | Reason |
|-------|--------|
| Atlas | Every project needs an orchestrator |
| Explore | Every refactor needs code search |
| Librarian | Every refactor needs reference research |

### Conditionally Generated

| Condition in L0 | Agent Generated |
|----------------|----------------|
| Any backend language detected | `backend-{language}.md` |
| Frontend framework detected (React/Vue/Angular/Svelte) | `frontend.md` |
| `tech-debt-ledger` has > 10 entries OR circular deps present | `oracle.md` |
| `/migrations/` or similar directory found | `dba.md` |
| `.github/workflows/` or `Dockerfile` found | `devops.md` |
| Total codebase > 50k lines | Add Navigator capability to Atlas config |

### Oracle Decision (most expensive — generate only when justified)

```
Generate Oracle if ANY of:
□ Circular dependencies detected in L0 module graph
□ tech-debt-ledger has entries with "impact: high"
□ Module count > 15
□ Multiple languages in the same repo
□ Unconfirmed invariants (🔴) exist in invariants.md
```

---

## Subagent Config Template Fields

Every generated subagent config contains these fields.
Values are filled from L0 data — shown here with placeholders:

```markdown
---
agent: {name}
execution-model: subagent
lifespan: single-task
context-budget: {low|medium|high}
parallel: {true|false}
---

# {Name} — {Role Title}

## Specialty
{2-3 sentences describing what this agent is uniquely good at}
{Project-specific context from L0 injected here}

## When Atlas Calls This Agent
{Concrete trigger scenarios — not abstract, drawn from this project's L0}

## Tools
{Specific commands available for this tech stack}

## Context Loading Strategy
{What Atlas should inject when spawning — keeps context-budget honest}

## Output Format
{Exact structure Atlas should expect back}

## Constraints
{What this agent must never do — read-only, scope limits, etc.}
```

Full base templates for each agent type:
see `skills/hp-agent-factory/references/agent-templates.md`

---

## Team Manifest Format

After all configs are generated, write `_team-manifest.md`:

```markdown
# Agent Team Manifest — {project-name}
Generated: {timestamp}

## Active Agents

| Agent | Config | Model Budget | When to Use |
|-------|--------|-------------|-------------|
| Atlas | atlas.md | high (persistent) | Always — orchestrator |
| Explore | explore.md | low | Finding code patterns and usages |
| Librarian | librarian.md | low | Researching libraries and best practices |
| Oracle | oracle.md | high | Architecture decisions (use sparingly) |
| Backend-TS | backend-typescript.md | medium | TypeScript/Node.js code changes |
| DBA | dba.md | medium | Schema changes and migration safety |

## Not Generated
- Frontend: no frontend framework detected in project
- DevOps: no CI/CD config found

## Rationale
Oracle: tech-debt-ledger has 4 high-impact entries including circular dependency
DBA: /migrations/ directory found with 23 migration files
```

---

## Regeneration

Re-run hp-agent-factory (partial) when:
- A new module is added that requires a new specialist agent
- An agent config is consistently producing poor results — rewrite from template
- Tech stack migration changes which agents are needed

Re-run hp-agent-factory (full) when:
- Major tech stack change (e.g., Python → TypeScript migration)
