#!/usr/bin/env python3
"""Replay USB enumeration and the bridge handshake through real TinyUSB classes."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("sdk", nargs="?", default=os.environ.get("PICO_SDK_PATH"))
args = parser.parse_args()
if not args.sdk:
    parser.error("pass the Pico SDK directory or set PICO_SDK_PATH")
root = Path(__file__).resolve().parents[1]
src = Path(args.sdk).resolve() / "lib/tinyusb/src"
if not (src / "tusb.c").exists():
    parser.error("SDK TinyUSB submodule is missing")
firmware = root / "firmware"
files = [firmware / "tests/usb_composite.c", firmware / "bridge/usb_desc.c",
         firmware / "shared/usb_audio.c", root / "devcore/audio_queue.c"]
files += [root / "devcore" / f for f in
          ("bridge.c", "node.c", "node_maint.c", "node_ident.c", "fwupdate.c", "fwupdate_commit.c")]
files += list((root / "proto").glob("*.c"))
files += [src / f for f in ("tusb.c", "device/usbd.c", "device/usbd_control.c",
                           "class/cdc/cdc_device.c", "class/audio/audio_device.c", "common/tusb_fifo.c")]
with tempfile.TemporaryDirectory(prefix="wye-usb-") as tmp:
    exe = Path(tmp) / "usb-test"
    cmd = [os.environ.get("CC", "cc"), "-std=c11", "-g", "-ffunction-sections", "-fdata-sections",
           '-DCFG_TUSB_CONFIG_FILE="' + str(firmware / "tests/tusb_config.h") + '"']
    for inc in (root, root / "proto", root / "devcore", firmware / "shared",
                firmware / "tests/include", src):
        cmd.append("-I" + str(inc))
    cmd += list(map(str, files)) + ["-Wl,--gc-sections", "-o", str(exe)]
    subprocess.run(cmd, check=True)
    subprocess.run([str(exe)], check=True, timeout=5)
