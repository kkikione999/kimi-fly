# Knowledge Hub

This directory is the AI-readable hardware knowledge layer for the project.

## Lookup Order

Use this order when changing hardware-related code:

1. `hardware-docs/pinout.md`
2. `knowledge-hub/hardware/<chip>/00-overview.md`
3. `knowledge-hub/hardware/<chip>/10-registers.md`
4. `knowledge-hub/hardware/<chip>/20-bringup-recipes.md`
5. `knowledge-hub/extracted/chunks/<chip>/<document>/*.md`
6. Original PDF in `hardware-docs/`

## Build The Local Knowledge Base

Generate the default chunk set:

```bash
python3 scripts/build_hardware_knowledge.py --clean
```

Also include large manuals and TRMs:

```bash
python3 scripts/build_hardware_knowledge.py --clean --include-large-manuals
```

Only rebuild one chip:

```bash
python3 scripts/build_hardware_knowledge.py --chip icm-42688-p --clean
```

## Search The Generated Chunks

```bash
python3 scripts/search_hardware_knowledge.py "who am i accel config0" --chip icm-42688-p
python3 scripts/search_hardware_knowledge.py "tim1 pwm preload" --chip stm32f411
```

## AnythingLLM UI Path

If you want a browser UI on top of the same materials:

```bash
cd knowledge-hub/platforms/anythingllm
cp .env.example .env
docker compose up -d
```

Then open `http://localhost:3001`.

## Rules

- Project truth wins over old code comments and stale test files.
- Datasheet-derived notes must keep a source PDF name and page number.
- Keep project wiring facts separate from vendor datasheet facts.
- Prefer curated markdown and generated chunks before opening the full PDF.
