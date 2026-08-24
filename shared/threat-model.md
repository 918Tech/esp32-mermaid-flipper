# Threat model

- Gateway credentials stay off the ESP32 boards and Flipper.
- Commands are normalized and allowlisted before approval.
- The approval console never auto-approves stale or malformed proposals.
- UART frames are bounded and CRC-protected.
