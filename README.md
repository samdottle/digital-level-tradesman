# Digital Level Tradesman — Web App + Firmware

**App version 1.6** · **Firmware version 2.5** · 2026-07-18 · MPU-6050 · Arduino Nano ESP32 · BLE Nordic UART

## What changed in v1.6
Removed the "MPU-6050 · ESP32 · BLE · vX.X" subtitle from the app header —
internal sensor/chip detail that doesn't belong in front of the end user.
Also fixed a real leftover bug from the v1.5 rename: the header subtitle's
version number had been missed and still read "v1.4" even after the title
itself was correctly renamed — moot now that the line is removed entirely,
but worth knowing about in case the same miss happened elsewhere.

## Family naming (current)
- **Digital Level RV** — MPU-6050, 4-point bull's-eye UI, RV market
- **Digital Level Tradesman** (this product) — MPU-6050, standard bubble UI, general trades
- **Digital Level Pro** — ADXL355, standard bubble UI, precision/millwright
- **Digital Level Pro 4 Point** — ADXL355, 4-point bull's-eye UI, precision/millwright
- **Digital Level Tradesman 4 Point** — MPU-6050, 4-point bull's-eye UI, general trades

## Known gap
`icons/icon-192.png` and `icons/icon-512.png` are referenced but not
included — same as every other package in this family.
