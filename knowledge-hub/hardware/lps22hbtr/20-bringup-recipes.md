# LPS22HBTR Bring-Up Lookup Guide

1. Confirm the SPI3 wiring in `hardware-docs/pinout.md`.
2. Search generated datasheet chunks for SPI timing, WHO_AM_I, and pressure output registers.
3. Validate `PB9` interrupt behavior only after polling reads work.
