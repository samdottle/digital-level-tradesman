# Digital Level Tradesman — Web App + Firmware

**Version 1.11 — EXPLORATORY, for evaluation, not confirmed for shipping**
2026-07-20 · MPU-6050 · Arduino Nano ESP32 · BLE Nordic UART

## Alternative to v1.10, proposed by ChatGPT
Built from `v1.9` directly (not combined with v1.10's manual toggle), for
side-by-side comparison.

## Core idea
Stop trying to map pitch onto operator-relative left/right at all — the
one thing this sensor genuinely cannot do without a magnetometer.
Instead, express both the measurement and the instruction in the same
body-fixed terms the sensor actually measures: **RAISE FRONT / RAISE
REAR / LEVEL**, matched by the operator against the physical FRONT label
already printed on the unit. Correct regardless of yaw rotation, because
nothing here claims to know which way the operator is standing.

## Real advantage over v1.10, identified while comparing the two
The manual toggle has a failure mode this doesn't: if the operator
rotates the unit mid-session and forgets to update the toggle, v1.10's
display goes wrong again, silently, with no way to detect it. This
approach has **no stored state to go stale** — the operator checks the
FRONT/REAR label against the physical marking fresh, every time.

## Honest limitation
This does **not** make FRONT irrelevant to the operator — it makes FRONT
the explicit, required reference. That inverts the original design goal
rather than achieving it. Accepted here as the honest tradeoff for what
this sensor can actually deliver, not hidden.

## What changed
- Added `POSITIVE_PITCH_MEANS_FRONT_HIGH`, a single isolated constant
  controlling FRONT/REAR label polarity. **Not verified against real
  hardware** — determining which physical direction corresponds to which
  pitch sign requires seeing the PCB's actual mounting orientation inside
  the housing, which isn't visible from software. If "RAISE FRONT"
  appears backwards on a real unit, flip this one boolean; nothing else
  needs to change.
- Added a prominent RAISE FRONT / RAISE REAR / LEVEL callout below the
  main pitch readout.
- Added FRONT / REAR text labels above the vial ends (screen-layout
  choice only — doesn't correspond to anything physical itself; only the
  label text needs to be matched against the unit).

## Confirmed by diff against v1.9
Zero changes to Tare, Calibration, pitch computation, EMA smoothing, or
any BLE handling. This feature only adds display logic on top of the
existing, unmodified `pitchDeg` value.

## Status
Working prototype for evaluation, not a locked/confirmed version — and
specifically needs the sign-convention constant checked against real
hardware before shipping.
