---
name: ardubot-firmware
description: Implement ArduBot ESP32 motion firmware and ESP32-S3 audio/camera firmware with ESP-IDF. Use for robot command protocols, watchdogs, telemetry and recoverable OTA.
---

Read PROJECT.md for accepted architecture. Motion runs on a controller separate from audio/camera. Start with a documented ESP-IDF-supported board; verify its exact pinout, PSRAM configuration and peripheral resources before writing wiring instructions.

Motion starts disabled after boot, reset, reconnect and OTA. Define bounded speed, acceleration and command duration. Reject invalid or expired commands, prevent replay after reconnect, and stop locally on command timeout or fault. Network tasks and AI must not bypass these checks. Choose timeout values with measured stopping distance and latency; do not claim a universal safe timeout.

Use a versioned command schema and explicit operating state. Publish faults, supply measurements if available, firmware version and connection status. Keep secrets out of logs and source control. A spoken stop is supplemental to physical motor power disconnection.

Start audio with push-to-talk and half-duplex playback at rest. Validate sample format and I2S pin/resource allocation before adding camera streaming or wake words. Benchmark Russian recognition and speech latency on the available server before promising real-time offline conversation.

OTA requires motors disabled, image integrity/authenticity checks and a recovery strategy supported by the selected flash layout. Never reuse an old motion command after reboot. Verify loss of network, server restart, malformed commands and device reboot on a stationary or lifted-wheel test setup before floor trials.
