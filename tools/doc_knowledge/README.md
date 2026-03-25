# Docling + Qdrant

This workspace now includes a local PDF-to-knowledge toolchain for hardware manuals and datasheets.

## Installed components

- `Docling` in `/.venv-docling`
- `Qdrant` server binary in `/.local/doc_knowledge/qdrant/bin/qdrant`
- `Qdrant` data in `/.local/doc_knowledge/qdrant/data`

## Commands

- Start Qdrant:
  `tools/doc_knowledge/start_qdrant.sh`
- Stop Qdrant:
  `tools/doc_knowledge/stop_qdrant.sh`
- Check versions and health:
  `tools/doc_knowledge/check_stack.sh`
- Convert a PDF to Markdown + JSON with OCR enabled:
  `tools/doc_knowledge/convert_pdf.sh path/to/manual.pdf`

## Notes

- Qdrant listens on `http://127.0.0.1:6333`
- The conversion wrapper writes output to `/.local/doc_knowledge/exports` by default
- For embedded manuals, prefer ingesting chapter-by-chapter or document-by-document, then index chunks with page and section metadata
