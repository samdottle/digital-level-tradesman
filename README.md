# Digital Level Tradesman — Web App + Firmware

**TEST 1.0** — evaluation build, not a numbered version, not for shipping
Built from v1.9 (the last non-experimental baseline). 2026-07-20 · MPU-6050 · Arduino Nano ESP32 · BLE Nordic UART

## What this implements
The FRONT/REAR bubble-mimic guidelines, confirmed correct through worked
examples covering both orientations from the original bug report:

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
screen. This works because the guidance text, and the bubble's own screen
position, are both driven by the same body-fixed pitch signal, which is
invariant under yaw rotation.

## Corrects a defect found in an earlier evaluation build (v1.11)
That build's screen layout (FRONT drawn on the left) caused the bubble to
move *away* from the raised end rather than toward it — backwards
relative to a real bubble level, independent of the yaw-orientation
question entirely. This build swaps which side FRONT is drawn (right,
not left) so bubble motion agrees with the text. Confirmed by tracing
both orientations above with real numbers before considering this
correct, not assumed from the fix alone.

## Not verified against real hardware
`POSITIVE_PITCH_MEANS_FRONT_HIGH` (near the top of `index.html`)
determines which physical direction produces which pitch sign, and that
depends on the PCB's actual mounting orientation inside the housing,
which isn't visible from software. If FRONT/REAR guidance and bubble
motion appear backwards together on a real unit, flip that one boolean —
text and bubble will flip together, staying consistent with each other
either way.

## Confirmed by diff against v1.9
Zero changes to Tare, Calibration, pitch computation, EMA smoothing, or
any BLE handling. This is purely a display-layer addition on top of the
existing, unmodified `pitchDeg` value.

## Status
Evaluation build. Not a shipping candidate until tested on real hardware.
