# Change Report Format Spec

The Change Report is the standard communication protocol between
Refactor Agent → Verifier + Scribe. Format must be strictly followed —
the automated pipeline depends on it.

---

## Full Format

```markdown
---
type: change-report
timestamp: 2024-01-15T14:32:00Z
module: billing/calculator
---

## Change Report

**Timestamp**: 2024-01-15T14:32:00Z
**Module**: billing/calculator
**Change type**: [see type list below]
**Intent**: one sentence in plain language a human can understand

### Modified Files
| File Path | Change Type | Scope |
|-----------|------------|-------|
| src/billing/calculator.js | implementation change | lines 42–87 |
| src/billing/display.js | implementation change | lines 12–15 |
| tests/billing/calculator.test.js | new tests added | lines 89–120 |

### Interface Changes
- **No interface changes** / OR list specifically:
  - `calculateBalance()` return type: float → integer

### Debt Eliminated
- TD-003 (float precision issue) → mark as resolved

### New Issues Discovered
- TD-012 (new): missed conversion at display.js line 67

### Invariant Impact
- INV-001: this change strengthens the invariant (added type annotation)
- No new invariants discovered

### Requested Actions
- [ ] Verifier: run billing module tests
- [ ] Scribe: update L1-billing.md (interface), tech-debt-ledger.md
- [ ] Health Check: re-score billing module
```

## Change Type List

```
- implementation change (interface unchanged)
- interface change (breaking)
- new function / module added
- function / module deleted
- rename / move
- extract duplicated logic (refactor)
- add type annotations
- add tests
- bug fix
```

---

# Agent Tool Templates

Based on the tech stack identified in L0, the Coordinator configures
the appropriate tool set for Refactor and Verifier agents.

## Node.js / TypeScript

```yaml
agent-tools:
  run-tests: "npm test -- --coverage"
  run-tests-module: "npm test -- --testPathPattern={module}"
  type-check: "npx tsc --noEmit"
  lint: "npx eslint src/ --ext .ts,.js"
  lint-fix: "npx eslint src/ --ext .ts,.js --fix"
  start-dev: "npm run dev"
  build: "npm run build"

rules:
  - run type-check after every TypeScript file change
  - run module tests only (not full suite) after each change
  - lint errors block merge; lint warnings do not
```

## Python

```yaml
agent-tools:
  run-tests: "pytest --cov=src --cov-report=term-missing"
  run-tests-module: "pytest tests/{module}/ -v"
  type-check: "mypy src/"
  lint: "ruff check src/"
  lint-fix: "ruff check src/ --fix"
  start-dev: "uvicorn main:app --reload"

rules:
  - prefer pytest fixtures over direct DB initialization in tests
  - mypy errors must be fixed — do not use `type: ignore` without explanation
  - ruff replaces flake8 + black; do not mix linters
```

## Go

```yaml
agent-tools:
  run-tests: "go test ./... -cover"
  run-tests-module: "go test ./{module}/... -v"
  type-check: "go vet ./..."
  lint: "golangci-lint run"
  start-dev: "air"
  build: "go build ./..."

rules:
  - Go type system is strict — run full tests after any interface change
  - never discard error return values with _
  - all new exported functions require godoc comments
```

## React / Next.js

```yaml
agent-tools:
  run-tests: "npx vitest run --coverage"
  run-tests-module: "npx vitest run {module}"
  type-check: "npx tsc --noEmit"
  lint: "npx eslint src/ --ext .tsx,.ts"
  e2e: "npx playwright test"  # CI only
  start-dev: "npm run dev"

rules:
  - use @testing-library/react for component tests, not Enzyme
  - test user behavior, not implementation details
  - run e2e tests only after major feature changes, not every refactor
```

## Java / Spring Boot

```yaml
agent-tools:
  run-tests: "mvn test"
  run-tests-module: "mvn test -Dtest={TestClass}"
  lint: "mvn spotbugs:check"
  build: "mvn package -DskipTests"
  start-dev: "mvn spring-boot:run"

rules:
  - unit tests: JUnit 5 + Mockito
  - integration tests: @SpringBootTest (be aware of startup time)
  - use constructor injection, not @Autowired field injection
```
