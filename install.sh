#!/bin/sh
# Installs Wye for this user, no root needed:
#   curl -fsSL https://raw.githubusercontent.com/mkalmousli/wye/main/install.sh | sh
# Remove it again:
#   curl -fsSL https://raw.githubusercontent.com/mkalmousli/wye/main/install.sh | sh -s -- --uninstall
#
# Downloads the latest release and checks its SHA-512. Where there is no release
# for this system yet, it builds from source if the build tools are there.
#
# Environment: WYE_PREFIX (default ~/.local), WYE_REPO, WYE_RELEASES, WYE_RAW.

main() {
    set -eu
    repo=${WYE_REPO:-https://github.com/mkalmousli/wye}
    releases=${WYE_RELEASES:-$repo/releases/latest/download}
    raw=${WYE_RAW:-https://raw.githubusercontent.com/mkalmousli/wye/main}
    prefix=${WYE_PREFIX:-$HOME/.local}
    bin=$prefix/bin/wye
    desktop=$prefix/share/applications/wye.desktop
    svg=$prefix/share/icons/hicolor/scalable/apps/wye.svg
    png=$prefix/share/icons/hicolor/256x256/apps/wye.png

    case ${1:-} in
    --uninstall) uninstall; return ;;
    "") ;;
    *) die "unknown option: $1 (the only option is --uninstall)" ;;
    esac

    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"' EXIT

    os=$(uname -s)
    arch=$(uname -m)
    if [ "$os" = Linux ] && [ "$arch" = x86_64 ] && from_release; then
        :
    else
        say "No release for $os $arch yet, so building from source."
        from_source
    fi
    finish
}

say() { printf 'wye: %s\n' "$1"; }
die() { printf 'wye: %s\n' "$1" >&2; exit 1; }

fetch() { # url file
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL "$1" -o "$2"
    elif command -v wget >/dev/null 2>&1; then
        wget -qO "$2" "$1"
    else
        die "needs curl or wget"
    fi
}

# The asset's SHA-512 from latest.json, which release.yml writes one field per line.
expected_hash() {
    awk '/"linux-x64"/ { on = 1 } on && /"sha512"/ { gsub(/[" ,]/, ""); sub(/^sha512:/, ""); print; exit }' "$1"
}

from_release() {
    fetch "$releases/latest.json" "$tmp/latest.json" 2>/dev/null || return 1
    fetch "$releases/wye-linux-x64" "$tmp/wye" || die "could not download the app"
    want=$(expected_hash "$tmp/latest.json")
    [ -n "$want" ] || die "the release has no checksum for this system; not installing"
    if command -v sha512sum >/dev/null 2>&1; then
        got=$(sha512sum "$tmp/wye" | cut -d' ' -f1)
    else
        got=$(shasum -a 512 "$tmp/wye" | cut -d' ' -f1)
    fi
    [ "$got" = "$want" ] || die "the download's checksum does not match; not installing"
    mkdir -p "$(dirname "$bin")" "$(dirname "$desktop")" "$(dirname "$svg")" "$(dirname "$png")"
    cp "$tmp/wye" "$bin.new"
    chmod 755 "$bin.new"
    mv -f "$bin.new" "$bin" # also works while the old version runs
    fetch "$raw/app/resources/icons/wye.svg" "$svg"
    fetch "$raw/app/resources/linux/wye.png" "$png"
    cat >"$desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Wye
GenericName=Keyboard and Mouse Switch
Comment=Drive a second computer with this keyboard and mouse, over a cable
Exec="$bin"
Icon=wye
Terminal=false
Categories=Utility;System;
Keywords=kvm;keyboard;mouse;switch;remote;pico;
StartupWMClass=Wye
StartupNotify=true
EOF
    command -v update-desktop-database >/dev/null 2>&1 && update-desktop-database "$(dirname "$desktop")" 2>/dev/null || true
    say "installed the latest release"
}

need_tools() {
    missing=
    for t in "$@"; do
        command -v "$t" >/dev/null 2>&1 || missing="$missing $t"
    done
    [ -z "$missing" ] && return 0
    printf 'wye: to build from source, first install:%s\n' "$missing" >&2
    if [ "$(uname -s)" = Darwin ]; then
        printf 'wye:   xcode-select --install && brew install cmake ninja && brew install --cask gcc-arm-embedded\n' >&2
    else
        printf 'wye: on Debian or Ubuntu:\n' >&2
        printf 'wye:   sudo apt-get install git build-essential cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib libx11-dev libxrandr-dev libxi-dev libxinerama-dev libxcursor-dev libgl-dev\n' >&2
    fi
    exit 1
}

from_source() {
    need_tools git cc cmake ninja arm-none-eabi-gcc
    say "downloading the source"
    git clone -q --depth 1 "$repo.git" "$tmp/wye-src"
    sdk=${PICO_SDK_PATH:-}
    if [ -z "$sdk" ]; then
        say "downloading the Pico SDK, which takes a few minutes"
        sdk=$tmp/pico-sdk
        git -c advice.detachedHead=false clone -q --depth 1 --branch 2.1.1 --recurse-submodules --shallow-submodules \
            https://github.com/raspberrypi/pico-sdk.git "$sdk"
    fi
    cd "$tmp/wye-src"
    say "building the firmware and the app"
    ./build.c firmware --pico-sdk="$sdk" >"$tmp/build.log" 2>&1 || { tail -20 "$tmp/build.log" >&2; die "the firmware build failed"; }
    case $(uname -s) in
    Linux) ./build.c install --prefix="$prefix" >"$tmp/build.log" 2>&1 ;;
    *) bin=$HOME/Applications/Wye.app/Contents/MacOS/Wye; ./build.c install >"$tmp/build.log" 2>&1 ;;
    esac || { tail -20 "$tmp/build.log" >&2; die "the app build failed"; }
    cd /
    say "installed from source"
}

finish() {
    say "version: $("$bin" --version)"
    case :${PATH:-}: in
    *:"$(dirname "$bin")":*) say "open Wye from your app menu, or run: wye" ;;
    *) say "open Wye from your app menu, or run: $bin" ;;
    esac
}

uninstall() {
    rm -f "$bin" "$desktop" "$svg" "$png" "$HOME/.config/autostart/wye.desktop"
    if [ "$(uname -s)" = Darwin ]; then
        rm -rf "$HOME/Applications/Wye.app"
    fi
    say "removed. Your settings and shared folder are still there."
}

main "$@"
