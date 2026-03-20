---
name: hp-cartography
description: >
  Harness-Pro sub-skill: scan a codebase and build a layered Knowledge Hub (L0/L1/L2).
  Automatically adapts to project size — from single-pass for small projects to
  chunked resumable scanning for large ones. Trigger when an unfamiliar codebase
  needs to be understood, or a source of truth needs to be established before
  AI-driven refactoring. Always the first step in the Harness-Pro pipeline.
  If a previous scan was interrupted, resumes from where it left off.
---

# hp-cartography — Code Scanning & Knowledge Hub Construction

## Step 0: Size Probe (always runs first, takes 2 seconds)

Before doing anything else, measure the project:

```bash
# Total file count (excluding noise)
find . -type f \
  ! -path '*/.git/*' ! -path '*/node_modules/*' \
  ! -path '*/dist/*' ! -path '*/.next/*' ! -path '*/build/*' \
  ! -path '*/__pycache__/*' ! -path '*/.venv/*' ! -path '*/vendor/*' \
  | wc -l

# Top-level module count
find . -maxdepth 1 -type d ! -name '.' ! -name '.*' | wc -l
```

Use the result to select a scan strategy:

| File Count | Strategy | L1 Approach | Expected L0 Time |
|-----------|----------|-------------|-----------------|
| < 100 | **Single-pass** | All L1s in one run | < 30s |
| 100–1000 | **Module-by-module** | L1 per module, sequential | < 60s for L0 |
| 1000–5000 | **Chunked** | L1 strictly on demand | < 30s for L0 |
| > 5000 | **Chunked + checkpoint** | L1 on demand only | < 30s for L0 |

Write the result to `.harness/cartography-state.md` before proceeding:

```markdown
---
scan-strategy: chunked        # single-pass | module-by-module | chunked | chunked+checkpoint
total-files: 2847
module-count: 12
l0-status: pending            # pending | complete
l1-pending: []                # filled after L0 completes
l1-complete: []
last-updated: 2024-01-15T14:00:00Z
---
```

**If `.harness/cartography-state.md` already exists → resume mode.**
Read the state file, skip completed work, continue from where it stopped.

---

## Step 1: L0 Scan — filesystem commands only, no file reading

Regardless of project size, L0 always uses the same fast approach.
**Do not open any file except manifest files.**

```bash
# 1. Directory structure
find . -maxdepth 2 -type d \
  ! -path '*/.git/*' ! -path '*/node_modules/*' \
  ! -path '*/dist/*' ! -path '*/.next/*' ! -path '*/build/*' \
  ! -path '*/__pycache__/*' ! -path '*/.venv/*' ! -path '*/vendor/*' \
  | sort

# 2. File count per top-level directory
find . -maxdepth 1 -type d ! -name '.' | while read d; do
  count=$(find "$d" -type f ! -path '*/.git/*' | wc -l)
  echo "$count $d"
done | sort -rn

# 3. Locate manifest files
find . -maxdepth 2 -type f \( \
  -name "package.json" -o -name "requirements.txt" \
  -o -name "pyproject.toml" -o -name "go.mod" \
  -o -name "Cargo.toml" -o -name "pom.xml" \
  -o -name "Dockerfile" -o -name "docker-compose.yml" \
\) ! -path '*/node_modules/*'

# 4. Read manifest files only (the one allowed file read in L0)
cat package.json 2>/dev/null | head -80
cat requirements.txt 2>/dev/null | head -40

# 5. Entry points
find . -maxdepth 3 -type f \( \
  -name "main.*" -o -name "index.*" -o -name "app.*" -o -name "server.*" \
\) ! -path '*/node_modules/*' ! -path '*/dist/*' ! -path '*/.venv/*'

# 6. Module relationship signal
grep -rl "from \.\." . --include="*.ts" 2>/dev/null | sed 's|/[^/]*$||' | sort | uniq -c | sort -rn | head -15
```

Write `knowledge-hub/L0-project-map.md`.
Update `cartography-state.md`: set `l0-status: complete`, populate `l1-pending` list.

---

## Step 2: L1 Scan — strategy depends on project size

### Single-pass (< 100 files)

Generate all L1 docs in one run. For small projects, this is fast and gives
the agent team full context immediately.

```bash
# For each top-level module directory:
for module in src/*/; do
  echo "=== $module ==="
  # Read only signatures — first 30 lines of each file
  find "$module" -type f \( -name "*.ts" -o -name "*.py" -o -name "*.go" \) \
    ! -name "*.test.*" ! -name "*.spec.*" \
    -exec head -30 {} \; 2>/dev/null
done
```

Write all `knowledge-hub/L1-{module}.md` files.

### Module-by-module (100–1000 files)

Process one module at a time, write its L1, update state, proceed to next.
If interrupted: state file records which modules are done.

```bash
# Per module — read only public-facing files
find ./src/{module} -maxdepth 1 -type f \
  \( -name "index.*" -o -name "*.interface.*" -o -name "types.*" \) \
  -exec head -50 {} \;

# Then spot-check implementation files for signatures only
grep -n "^export\|^def \|^func \|^class \|^public " \
  ./src/{module}/*.ts 2>/dev/null | head -40
```

After each module: update `l1-complete` in state file.

### Chunked (> 1000 files)

L1 is **never batch-generated**. Only generate L1 for a module when:
- Refactor Agent is about to work on it, OR
- Health Check needs to score it

Update state file each time a new L1 is written.

---

## Step 3: Resuming an Interrupted Scan

At startup, always check for an existing state file:

```bash
cat .harness/cartography-state.md 2>/dev/null
```

**If found:**
```
Resume logic:
- l0-status: pending → restart L0 (fast, always safe to redo)
- l0-status: complete, l1-pending not empty → continue L1 from next pending module
- All complete → report "Knowledge Hub is current, nothing to do"
```

**If not found:** fresh scan, start from Step 0.

This means Cartography is always **idempotent** — safe to re-run at any time.
Re-running never destroys existing work, only fills in gaps.

---

## L2: Implementation Detail — always on demand, any project size

Generated only when Refactor Agent needs a specific function.
Never batch-generated regardless of project size.

```bash
# Locate the function
grep -n "function {name}\|def {name}\|func {name}" src/{module}/{file}

# Read only that function's body
sed -n '{start_line},{end_line}p' src/{module}/{file}
```

---

## Business Invariant Extraction

Run during L1 scanning (not L0). Fast grep, no full file reads:

```bash
grep -rn "DO NOT CHANGE\|CRITICAL\|INVARIANT\|NEVER\|ALWAYS" \
  ./src --include="*.ts" --include="*.py" --include="*.go" \
  ! -path '*/node_modules/*' 2>/dev/null | head -50
```

---

## Output Checklist

```
.harness/
└── cartography-state.md     ← scan progress (always written first)

knowledge-hub/
├── L0-project-map.md        ← always complete before L1 starts
├── L1-{module}.md           ← one per module, generated per strategy
├── invariants.md            ← populated during L1 scans
├── tech-debt-ledger.md      ← populated during L1 scans
└── refactor-log.md          ← empty at this stage
```

---

## Rules (apply at all project sizes)

- **Size probe first, always**: never start scanning without knowing the scale
- **L0 uses commands only**: `find`, `grep -l`, `head`, `wc` — no file reads except manifests
- **Write state before scanning**: cartography-state.md is the checkpoint
- **Idempotent**: re-running never breaks existing Knowledge Hub docs
- **L2 never pre-generated**: always on demand, for any project size
- **Intent unclear**: annotate `⚠️ intent unclear`, do not guess
- **Bugs found**: log to tech-debt-ledger, do not fix during scan
