# kimi-fly Agent Guide

> **Humans steer. Agents execute.**

This is the entry point for AI agents working on the kimi-fly drone project. This file is a **map**, not an encyclopedia. Follow pointers to dive deeper.

## Project Overview

kimi-fly is a drone/UAV project with two main components:
- **机身/** (Airframe): Mechanical structure, aerodynamics
- **主控/** (Flight Controller): STM32 + ESP32-C3 embedded systems

## Quick Navigation

| Need | Go To |
|------|-------|
| Coding standards | [`CLAUDE.md`](./CLAUDE.md) |
| Hardware architecture | [`docs/HARDWARE.md`](./docs/HARDWARE.md) |
| How to use agents | [`docs/AGENTS-GUIDE.md`](./docs/AGENTS-GUIDE.md) |
| Design principles | [`docs/design-docs/core-beliefs.md`](./docs/design-docs/core-beliefs.md) |
| Active work plans | [`docs/exec-plans/active/`](./docs/exec-plans/active/) |
| Technical debt | [`docs/exec-plans/tech-debt-tracker.md`](./docs/exec-plans/tech-debt-tracker.md) |

## Agent Team

Invoke the right agent for the task:

- **stm32-embedded-engineer**: STM32 firmware, HAL/LL drivers, peripherals
- **esp32-c3-autonomous-engineer**: ESP32-C3 firmware, wireless, automation
- **embedded-test-engineer**: Test cases, HIL testing, validation
- **harness-architect**: Multi-agent orchestration, architecture decisions
- **plan-reviewer**: Review plans before execution

## Golden Principles

1. **Mechanical over manual**: Use linters and automated checks
2. **Parse, don't validate**: Use typed structures at boundaries
3. **Fail fast**: Assertions and early error detection
4. **Document in repo**: Knowledge lives here, not in chat
5. **Small PRs, fast iteration**: Corrections are cheap, waiting is expensive

## Before You Start

1. Read [`CLAUDE.md`](./CLAUDE.md) for coding standards
2. Check [`docs/exec-plans/active/`](./docs/exec-plans/active/) for ongoing work
3. Review [`docs/HARDWARE.md`](./docs/HARDWARE.md) for hardware context
4. When blocked, write to [`docs/harness-question.md`](./docs/harness-question.md)

## Emergency Escalation

If you encounter:
- Hardware defects → Document in `docs/harness-question.md` with scope capture
- Safety-critical bugs → Stop work, escalate immediately
- Architectural conflicts → Consult harness-architect

---

*This is an agent-first repository. All code, tests, docs, and tooling are agent-generated.*
