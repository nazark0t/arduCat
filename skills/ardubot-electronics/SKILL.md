---
name: ardubot-electronics
description: Design and troubleshoot ArduBot LEGO-compatible motor wiring, power distribution, sensors and printed mounts. Use for ArduBot hardware selection, assembly and measurements.
---

Read PROJECT.md in the ArduBot workspace for inventory and raw measurements. Treat clone connector pinouts as unknown until measured; photographs identify geometry, not electrical compatibility. Preserve measurement polarity and connector viewing direction. Resolve contradictory readings before assigning power pins.

Keep the 15000 RUB prototype budget and existing LEGO-compatible mechanics. Select motor drivers using supply range, 3.3 V logic compatibility, measured operating current and documented startup/stall allowance. Do not select from advertised peak current alone. Do not instruct prolonged stall measurements on battery power.

Use a physical motor power disconnect independent of software. Keep initial logic power on USB. Design common grounding deliberately; do not parallel regulated supplies or connect motor supply to ESP32 logic rails. Fuse wiring appropriately and provide motor noise suppression and decoupling based on the actual layout.

Before purchase, verify exact breakout variant, schematic, connectors, populated components and usable pins. Distinguish a bare ESP32 module from a USB development board. Check camera/PSRAM/I2S pin conflicts before combining peripherals. Printed mounts must allow cable strain relief and access to power disconnects.

For a measurement, specify probe sockets, meter mode, lead polarity, powered/unpowered state and a short interpretation. Never use resistance/continuity on a powered circuit or place an ammeter across a supply. Mark unknown electrical values explicitly.
