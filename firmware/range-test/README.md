# VL53L0X bench test

ESP32-S3 N16R8 movement controller only. Identify your own board before flashing. USB power. One VL53L0X:
VIN=3V3, GND=GND, SDA=GPIO8, SCL=GPIO9. XSHUT/GPIO unused.

Build with `./build.ps1`. ESP-IDF 5.5.0, espressif32 6.12.0.
The basic DevKitC target uses internal RAM; PSRAM is not enabled or tested.
Flash size is overridden to 16 MB. Artifacts: `.tools/range-build`.

At boot: scan I2C (100 kHz), require address 0x29 and model ID 0xEE,
run ST DataInit/StaticInit, reference SPAD management and reference calibration.
Take 60 continuous-mode samples with 33 ms timing budget and 200 ms read spacing,
then stop ranging. A sample is accepted only when RangeStatus=0.
The readiness wait is bounded to one second; I2C operations to 100 ms.
No motor GPIO, Wi-Fi or audio is initialized by this application.
Reset restarts the test. `capture.py` resets the board and saves UART output.

Before any first write, save and verify all 16 MB of original flash.
Backup for this controller: `.tools/backups/movement-original.bin`;
SHA-256 and successful esptool verification log are stored beside it.
For recovery use esptool write-flash 0 with that complete image on the
verified movement controller only, with motor power disconnected.

Vendor source: ST API 1.0.2, BSD-3-Clause, from
https://github.com/zephyrproject-rtos/hal_st/tree/master/sensor/vl53l0x
Exact fetched revision: vendor/UPSTREAM_COMMIT.txt. Original copyright and
license headers are retained. src/platform.c adapts I2C to ESP-IDF.
Official API documentation:
https://www.st.com/en/embedded-software/stsw-img005.html

## Diagnostic v2

v2 resets the sensor using register 0xBF (low/high, 10 ms delays) before
DataInit; ESP32 reset alone does not remove sensor power. It verifies model
ID again after reset. It uses SINGLE_RANGING, a bounded readiness wait,
and two timing budgets with readback: 20 samples at 33 ms, 20 at 200 ms.
The original continuous-mode application is `.tools/backups/range-v1-app.bin`.
UART capture uses 115200 baud. No limits are disabled to accept bad ranges.
This compares modes within the same ST driver; it is not an independent
driver validation and does not by itself establish a sensor defect.

## Live test v3

Current firmware uses repeated single measurements indefinitely, 50 ms budget
(read back 49990 us), 50 ms spacing (~8 samples/s). Each line includes a
monotonic timestamp. Status 0 emits VALID/mm; any nonzero range status emits
INVALID/mm=NA and retains raw_mm only for diagnosis. It does not reuse a last
good value. Transport errors or readiness timeout exit the loop and stop.
A consumer must treat missing/stale output and FAIL as unknown distance.
This is diagnostic output only, with no movement permission or motor control.

Capture: `capture.py --seconds 10 --no-reset --output <log>` reads an ongoing
run; omit --no-reset to reset the ESP32 and reinitialize the sensor first.
Closing capture does not stop the sensor: v3 keeps running until reset/power-off
(or a transport fault). v2 app/source are saved under `.tools/backups/range-v2-*`.
Dynamic response, absent target and occluded optics require separate observed
physical trials. An absent target may expose a more distant background, and an
occluder may itself produce a valid near return; do not assume either produces
an invalid status.

## Fresh Windows environment

Follow the Python virtualenv installation in ../mic-test/README.md, then run
`./firmware/range-test/build.ps1` from the repository root. It only builds;
it does not flash hardware. Current source is v3; the v1/v2 sections above
record previous experiments. Use your actual serial port with `--port`.
Keep output under `.tools/` (create that folder before capture).
See ../../SECURITY.md before adding network features.
