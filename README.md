# Digital Level Tradesman — Web App + Firmware

**Version 1.7** · **Firmware version 2.5** · 2026-07-19 · MPU-6050 · Arduino Nano ESP32 · BLE Nordic UART

## What changed in v1.7
One confirmed bug fixed in the Tare function, ported from Digital Level
Tradesman 4 Point v1.1 — see that product's changelog and its
`DL_Tradesman4Point_Tare_Bug_Handoff.md` for the full derivation and
debugging history.

**EMA smoothing accumulator was never reset on Tare** — only the
displayed value was. The accumulator kept holding its stale pre-tare
value, so the display would drift back off-center over the following
packets instead of staying at 0. Fixed: the accumulator now resets to 0
alongside the displayed state.

Also fixed: `sendCommand()` previously had a silent `catch(e){}` — any
BLE write failure produced zero indication. Now returns `true`/`false`
for success/failure and surfaces a toast on any caught write error.

## This product does NOT need the roll sign-convention fix
Confirmed this product never negates roll on receipt (no minus sign on
`parseFloat(m[2])`), unlike Tradesman 4 Point and Pro 4 Point — that bug
does not apply here.

## Confidence level — please read before relying on this
The accumulator fix has not been independently re-verified against real
Tradesman (standard) hardware as of this version. It's ported directly
from a fix confirmed working on Tradesman 4 Point, which shares this
exact mechanism — but worth testing on this product's own hardware
before treating it as equally proven.

## Family naming (current)
- **Digital Level RV** — MPU-6050, 4-point bull's-eye UI, RV market
- **Digital Level Tradesman** (this product) — MPU-6050, standard bubble UI, general trades
- **Digital Level Tradesman 4 Point** — MPU-6050, 4-point bull's-eye UI, general trades / machine leveling
- **Digital Level Pro** — ADXL355, standard bubble UI, precision/millwright
- **Digital Level Pro 4 Point** — ADXL355, 4-point bull's-eye UI, precision/millwright

## Known, not yet done
Digital Level Pro (standard) is confirmed to need this same accumulator
fix. Not yet fixed as of this version.
