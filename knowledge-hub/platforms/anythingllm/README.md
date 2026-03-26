# AnythingLLM Integration

This directory gives the ready-to-run open-source UI path for the hardware knowledge base.

## What This Provides

- A local `docker-compose.yml` for `AnythingLLM`
- Read-only mounts for:
  - `hardware-docs/`
  - `knowledge-hub/`
- Persistent app storage under `artifacts/anythingllm-storage/`

## First Run

```bash
cd knowledge-hub/platforms/anythingllm
cp .env.example .env
docker compose up -d
```

Open `http://localhost:3001`.

## Recommended Workspace Layout In AnythingLLM

Create workspaces like:

- `hardware-stm32`
- `hardware-imu`
- `hardware-barometer`
- `hardware-wifi-bridge`

Import both kinds of material:

- project truth docs from `knowledge-hub/hardware/`
- original PDFs from `hardware-docs/`

## Import Order

1. `hardware-docs/pinout.md`
2. chip overview / register / bring-up markdown
3. generated chunks under `knowledge-hub/extracted/chunks/`
4. original PDF only as fallback

## Notes

- The compose file uses `mintplexlabs/anythingllm:latest`.
- Telemetry is disabled by default in `.env.example`.
- This UI path is useful for browsing and question answering, but repository development should still prefer the local markdown/chunk workflow first.
