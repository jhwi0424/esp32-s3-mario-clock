"""PlatformIO pre-build hook: prepare the pinned upstream Mario clockface."""
import os
from pathlib import Path
import subprocess

Import("env")

project = Path(env.subst("$PROJECT_DIR")).resolve()
clockface = project / "clockfaces" / "cw-cf-0x01"
font = clockface / "gfx" / "Super_Mario_Bros__24pt7b.h"
if not font.is_file():
    raise RuntimeError(
        "Mario submodule is missing. From the repository root run: "
        "git submodule update --init firmware/clockfaces/cw-cf-0x01"
    )

# The pinned upstream font recognizes only the classic ESP32 board macro.
# Apply the one-line S3 compatibility patch without changing font bitmaps.
original = "#ifdef ARDUINO_ESP32_DEV"
compatible = "#if defined(ARDUINO_ESP32_DEV) || defined(CW_BOARD_ANIMATED_PIXEL_S3)"
content = font.read_text(encoding="utf-8")
if original in content:
    font.write_text(content.replace(original, compatible, 1), encoding="utf-8")
elif compatible not in content:
    raise RuntimeError("Unexpected Mario font version; cannot apply the S3 compatibility patch")

link = project / "lib" / "cw-cf-0x01"
if link.exists():
    if link.resolve() != clockface.resolve():
        raise RuntimeError(f"{link} already exists and does not point to the selected clockface")
elif os.name == "nt":
    subprocess.run(
        ["cmd.exe", "/d", "/c", "mklink", "/J", str(link), str(clockface)],
        check=True, capture_output=True, text=True,
    )
else:
    link.symlink_to(os.path.relpath(clockface, link.parent), target_is_directory=True)
