#!/usr/bin/env python3
"""Write dist/latest.json (signed later) and the release notes for a tag.

Run from the repo root by .github/workflows/release.yml:
    TAG=v1.2.3 REPO=owner/name RUNNER_TEMP=... python3 tools/release/manifest.py
"""
import hashlib
import json
import os
import subprocess

tag = os.environ["TAG"]
repo = os.environ["REPO"]
notes_path = os.path.join(os.environ.get("RUNNER_TEMP", "/tmp"), "RELEASE_NOTES.md")
version = tag.lstrip("v")
base = f"https://github.com/{repo}/releases/download/{tag}/"
site = f"https://github.com/{repo}"

# asset key used by the install scripts -> file name in dist/
PLATFORMS = {
    "linux-x64": "wye-linux-x64",
    "linux-arm64": "wye-linux-arm64",
    "windows-x64": "Wye-windows-x64.exe",
}


def git(*args):
    return subprocess.run(["git", *args], capture_output=True, text=True).stdout.strip()


def sha512(path):
    with open(path, "rb") as f:
        return hashlib.sha512(f.read()).hexdigest()


assets = {}
for key, name in PLATFORMS.items():
    path = os.path.join("dist", name)
    if os.path.exists(path):
        assets[key] = {"file": name, "url": base + name, "sha512": sha512(path),
                       "size": os.path.getsize(path)}

subject = git("tag", "-l", "--format=%(contents:subject)", tag)
body = git("tag", "-l", "--format=%(contents:body)", tag)
prev = git("describe", "--tags", "--abbrev=0", f"{tag}^")
log = git("log", "--no-merges", "--pretty=- %s", f"{prev}..{tag}" if prev else tag)

with open(os.path.join("dist", "latest.json"), "w") as f:
    json.dump({"version": version, "notes": subject, "assets": assets}, f, indent=2)

rows = [f"| {key} | `{a['file']}` | {a['size'] // 1024} KiB | `{a['sha512'][:16]}…` |"
        for key, a in assets.items()]
lines = [
    f"# Wye {version}",
    "",
    subject,
    "",
    body,
    "",
    "## Download",
    "",
    "| System | File | Size | SHA-512 (first 16) |",
    "| --- | --- | --- | --- |",
    *rows,
    "| Pico firmware | `wye_bridge.uf2`, `wye_target.uf2` | | the app flashes both Picos for you |",
    "",
    "## Install",
    "",
    f"- **Windows:** `irm https://raw.githubusercontent.com/{repo}/main/install.ps1 | iex`",
    f"- **Linux:** `curl -fsSL https://raw.githubusercontent.com/{repo}/main/install.sh | sh`",
    "- **Manual:** download the file for your system and check its SHA-512 against the table.",
    "",
    f"Guide: [Get started]({site}#get-started) · [API]({site}/blob/main/docs/API.md) · "
    f"[Build from source]({site}#build)",
    "",
    f"## Changes since {prev or 'the beginning'}",
    "",
    log or "- (no changes recorded)",
    "",
]
with open(notes_path, "w") as f:
    f.write("\n".join(lines))
print(f"wrote dist/latest.json ({len(assets)} assets) and {notes_path}")
