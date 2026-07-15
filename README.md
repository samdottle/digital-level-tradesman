# Digital Level 2 — Web App + Firmware

**App version 1.4** · **Firmware version 2.5** · 2026-07-15 · MPU-6050 · Arduino Nano ESP32 · BLE Nordic UART

## Files
- `index.html` — the app (v1.4), rebranded from the Digital Level 3 app:
  "ADXL355" → "MPU-6050", "Digital Level 3" → "Digital Level 2" throughout.
  Same tare-fix changelog as the Digital Level 3 release — see the comment
  at the top of the file.
- `manifest.json` — PWA manifest, rebranded (name/short_name/description).
- `sw.js` — service worker, own cache namespace (`dl2-v1.4`) separate from
  Digital Level 3's, so the two can't collide if ever served from the same
  origin.
- `DigitalLevel_Firmware_v2.5.ino` — Arduino sketch. Changelog is in the
  header comment block. v2.5 fixes the standalone buzzer sounding during
  the calibration sequence; both v2.5 and the untouched v2.4 `TAR:` handling
  it inherited are the actual code base this app talks to.

## What changed in v1.4 (app)
Fixed a bug where tapping **Tare to Current Position** while connected would
briefly show 0.00° and then jump to the *negative* of the pre-tare reading —
including the LEVEL badge — instead of settling at zero.

## What changed in v2.5 (firmware)
Fixed the standalone buzzer sounding continuously during the two-position
calibration sequence, caused by the level-check driving the buzzer with no
awareness of calibration state.

## Known gap
`icons/icon-192.png` and `icons/icon-512.png` are referenced by
`manifest.json` and `sw.js` but were never provided, so this package does
not include an `icons/` folder. Add your own before deploying.
