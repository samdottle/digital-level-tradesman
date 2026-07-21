# Digital Level Tradesman — Web App + Firmware

**Version 1.10 — EXPLORATORY, for evaluation, not confirmed for shipping**
2026-07-19 · MPU-6050 · Arduino Nano ESP32 · BLE Nordic UART

## Background
This hardware has no magnetometer, so it cannot sense yaw (rotation
around the vertical axis). Confirmed this is not a new bug — traced back
through the firmware and app source to the original DL2 release
(v1.4/v2.4) and found byte-for-byte identical, unchanged through every
subsequent rename and version bump. Several automatic-detection
approaches (gyro integration, sign-change inference) were explored and
rejected as unreliable — each had concrete false-positive/false-negative
scenarios, not just abstract uncertainty.

## What this version adds
A manual FRONT-orientation control, tied to the physical FRONT label
already printed on the unit — the same reference the 4-point products
already rely on and are confirmed to work correctly with. The operator
answers once, which side FRONT is on; persisted in `localStorage` since a
given operator likely places the unit the same way every time.

## Implementation notes
- **Correction applied upstream**, at the point raw pitch is parsed —
  not just at the final bubble position — so display, Tare, and
  Calibration all agree with each other and with the LEVEL threshold
  check.
- **Tare and Calibration both negate the value back** before sending to
  the firmware, undoing the app-side correction. Without this, it would
  reintroduce the exact bug found and fixed on Tradesman 4 Point's roll
  axis — an already-corrected value sent into `TAR:`/`CAL:` computes the
  wrong new offset, and the error compounds rather than resolving.
- LEVEL threshold check needed no change — already sign-independent
  (`Math.abs`).
- Demo modes needed no change — neither uses real sensor data.
- Uses a parallel ref alongside the React state so the BLE data handler
  (a stable callback) can read the current value without needing to be
  recreated, which would otherwise require re-registering the BLE
  listener.

## Not yet done
An equivalent phone-rotation-based approach (physically rotating the
viewing device 180° to match FRONT) was also discussed as a zero-UI
alternative, but not built or tested here — its reliability depends on
OS auto-rotate behavior that varies by device and hasn't been verified
on real hardware.

## Status
This is a working prototype for evaluation, not a locked/confirmed
version. Please test before treating it as a shipping candidate.
