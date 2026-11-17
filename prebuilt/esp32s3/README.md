# ESP32-S3 N16R8 / 64×64 HUB75E

- `Mario-S3-64x64.full.bin`: flash at **0x0** for a first installation. The NVS region is erased data.
- `Mario-S3-64x64.app.bin`: flash at **0x10000** to update the application while preserving device settings.
- `SHA256SUMS.txt`: SHA-256 checksums.

Built from source with `esp32s3-mario-usb`. Neither image is a dump from the configured board;
neither contains personal Wi-Fi credentials. Join `Clockwise-Wifi` after first boot to configure your own 2.4GHz network.

See [the wiring and setup guide](../../ESP32S3_MARIO.md).
