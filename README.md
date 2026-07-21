# Digital Level Tradesman — Web App + Firmware

**Proto 1.0 — locked down for shipment**
2026-07-21 · MPU-6050 · Arduino Nano ESP32 · BLE Nordic UART

Built from v1.9 (the last non-experimental baseline), via TEST 1.0
(evaluation build, confirmed working).

## What this implements
FRONT/REAR guidance in place of implied operator-relative left/right,
because the MPU-6050 sensor has no magnetometer and physically cannot
sense yaw (rotation around the vertical axis) — meaning no software fix
can ever make the display correct for both physical orientations if it
depends on the operator's own left/right.

Full background, including everything ruled out before arriving here, is
preserved in this product's orientation bug handoff document
(`DLT_Orientation_Bug_Handoff.md`).

Confirmed correct through worked examples covering both orientations
from the original bug report:

**Orientation A**: FRONT on operator's left, operator raises that end
(raising FRONT). FRONT becomes the physically high end. Text shows
**RAISE REAR** (correct — REAR is low). Bubble sits near the FRONT
label — the end that's actually high. Bubble moves *toward* the raised
end, matching a real level.

**Orientation B**: unit rotated 180°, FRONT now on operator's right.
Operator raises the *same physical end* as before (still under the same
hand) — which, because the bar rotated, is now REAR. REAR becomes the
physically high end. Text shows **RAISE FRONT** (correct — FRONT is
low). Bubble sits near the REAR label — again the end that's actually
high.

In both cases the operator never needs to track their own left/right —
only match the FRONT sticker on the physical unit to the FRONT label on
screen.

## Not verified against real hardware
`POSITIVE_PITCH_MEANS_FRONT_HIGH` (near the top of `index.html`)
determines which physical direction produces which pitch sign — depends
on the PCB's actual mounting orientation inside the housing, which isn't
visible from software. If FRONT/REAR guidance and bubble motion appear
backwards together on a real unit, flip that one boolean — text and
bubble will flip together, staying consistent with each other either
way. This is the one remaining thing to confirm on a physical unit
before this design should be considered fully validated.

## Confirmed by diff against v1.9
Zero changes to Tare, Calibration, pitch computation, EMA smoothing, or
any BLE handling. This is purely a display-layer addition on top of the
existing, unmodified `pitchDeg` value.

## Family naming (current)
- **Digital Level RV** — MPU-6050, 4-point bull's-eye UI, RV market
- **Digital Level Tradesman** (this product) — MPU-6050, FRONT/REAR bubble-mimic UI, general trades
- **Digital Level Tradesman 4 Point** — MPU-6050, 4-point bull's-eye UI, general trades / machine leveling
- **Digital Level Pro** — ADXL355, standard bubble UI, precision/millwright
- **Digital Level Pro 4 Point** — ADXL355, 4-point bull's-eye UI, precision/millwright
