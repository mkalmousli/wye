#!/bin/sh
# Installs Wye for this user, no root needed:
#   curl -fsSL https://raw.githubusercontent.com/mkalmousli/wye/main/install.sh | sh
# Remove it again:
#   curl -fsSL https://raw.githubusercontent.com/mkalmousli/wye/main/install.sh | sh -s -- --uninstall
#
# Picks the release file for this system, shows its download progress, checks
# its SHA-512 and installs it. With no release for this system yet, it builds
# from source when the build tools are there.
#
# Environment: WYE_PREFIX (default ~/.local), WYE_REPO, WYE_RELEASES, WYE_RAW,
#              NO_COLOR (turns colours off).

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
    setup_colors

    case ${1:-} in
    --uninstall) uninstall; return ;;
    "") ;;
    *) die "unknown option: $1 (the only option is --uninstall)" ;;
    esac

    tmp=$(mktemp -d)
    trap 'rm -rf "$tmp"' EXIT

    os=$(uname -s)
    arch=$(uname -m)
    case $os-$arch in
    Linux-x86_64 | Linux-amd64) key=linux-x64 ;;
    Linux-aarch64 | Linux-arm64) key=linux-arm64 ;;
    Darwin-arm64) key=macos-arm64 ;;
    Darwin-x86_64) key=macos-x64 ;;
    *) key= ;;
    esac

    banner
    if [ -n "$key" ] && from_release "$key"; then
        :
    else
        warn "no release for $os $arch yet, so building from source"
        from_source
    fi
    finish
}

setup_colors() {
    if [ -t 1 ] && [ -z "${NO_COLOR:-}" ] && [ "${TERM:-dumb}" != dumb ]; then
        c_reset=$(printf '\033[0m'); c_bold=$(printf '\033[1m'); c_dim=$(printf '\033[2m')
        c_red=$(printf '\033[31m'); c_green=$(printf '\033[32m'); c_yellow=$(printf '\033[33m')
        c_blue=$(printf '\033[34m'); c_magenta=$(printf '\033[35m'); c_cyan=$(printf '\033[36m')
    else
        c_reset= c_bold= c_dim= c_red= c_green= c_yellow= c_blue= c_magenta= c_cyan=
    fi
}

banner() {
    printf '%s' "$c_magenta$c_bold"
    cat <<'ART'
  __      ___   _  ___
  \ \    / / | | |/ _ \
   \ \/\/ /| |_| |  __/
    \_/\_/  \__, |\___|
            |___/
ART
    printf '%s  one keyboard, two computers, three wires%s\n\n' "$c_reset$c_dim" "$c_reset"
}

say() { printf '%s==>%s %s\n' "$c_cyan$c_bold" "$c_reset$c_bold" "$1$c_reset"; }
ok() { printf '  %s✓%s %s\n' "$c_green$c_bold" "$c_reset" "$1"; }
warn() { printf '  %s!%s %s\n' "$c_yellow$c_bold" "$c_reset" "$1" >&2; }
die() { printf '  %s✗%s %s\n' "$c_red$c_bold" "$c_reset" "$1" >&2; exit 1; }

# Downloads $1 to $2. Shows a progress bar on a terminal, stays quiet otherwise.
fetch() {
    if command -v curl >/dev/null 2>&1; then
        if [ -t 2 ]; then
            curl -fL --progress-bar "$1" -o "$2"
        else
            curl -fsSL "$1" -o "$2"
        fi
    elif command -v wget >/dev/null 2>&1; then
        wget -qO "$2" "$1"
    else
        die "needs curl or wget"
    fi
}

# Reads a field of one asset from latest.json: field KEY NAME FILE.
asset_field() {
    awk -v key="\"$1\"" -v name="\"$2\"" '
        index($0, key) { on = 1 }
        on && index($0, name) { sub(/^[^:]*: */, ""); gsub(/[",]/, ""); print; exit }' "$3"
}

hash_of() {
    if command -v sha512sum >/dev/null 2>&1; then
        sha512sum "$1" | cut -d' ' -f1
    else
        shasum -a 512 "$1" | cut -d' ' -f1
    fi
}

from_release() { # key
    say "looking for the latest release"
    fetch "$releases/latest.json" "$tmp/latest.json" 2>/dev/null || return 1
    url=$(asset_field "$1" url "$tmp/latest.json")
    want=$(asset_field "$1" sha512 "$tmp/latest.json")
    file=$(asset_field "$1" file "$tmp/latest.json")
    version=$(asset_field version version "$tmp/latest.json")
    [ -n "$url" ] || return 1
    ok "found Wye $version for $1 ($file)"

    say "downloading $file"
    fetch "$url" "$tmp/wye" || die "could not download the app"

    say "checking the download"
    [ -n "$want" ] || die "the release has no checksum for this system; not installing"
    [ "$(hash_of "$tmp/wye")" = "$want" ] || die "the download's checksum does not match; not installing"
    ok "SHA-512 matches"

    say "installing to $bin"
    mkdir -p "$(dirname "$bin")" "$(dirname "$desktop")" "$(dirname "$svg")" "$(dirname "$png")"
    cp "$tmp/wye" "$bin.new"
    chmod 755 "$bin.new"
    mv -f "$bin.new" "$bin" # also works while the old version runs
    ok "app installed"

    if [ "$os" != Darwin ]; then
    say "adding the icon and the app menu entry"
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
    ok "app menu entry added"
    fi
}

need_tools() {
    missing=
    for t in "$@"; do
        command -v "$t" >/dev/null 2>&1 || missing="$missing $t"
    done
    [ -z "$missing" ] && return 0
    warn "to build from source, first install:$missing"
    if [ "$(uname -s)" = Darwin ]; then
        printf '      xcode-select --install && brew install cmake ninja && brew install --cask gcc-arm-embedded\n' >&2
    else
        printf '      sudo apt-get install git build-essential cmake ninja-build gcc-arm-none-eabi libnewlib-arm-none-eabi libstdc++-arm-none-eabi-newlib libx11-dev libxrandr-dev libxi-dev libxinerama-dev libxcursor-dev libgl-dev\n' >&2
    fi
    exit 1
}

from_source() {
    need_tools git cc cmake ninja arm-none-eabi-gcc
    say "downloading the source"
    git clone -q --depth 1 "$repo.git" "$tmp/wye-src"
    sdk=${PICO_SDK_PATH:-}
    if [ -z "$sdk" ]; then
        say "downloading the Pico SDK (a few minutes)"
        sdk=$tmp/pico-sdk
        git -c advice.detachedHead=false clone -q --depth 1 --branch 2.1.1 --recurse-submodules --shallow-submodules \
            https://github.com/raspberrypi/pico-sdk.git "$sdk"
    fi
    cd "$tmp/wye-src"
    say "building the firmware"
    ./build.c firmware --pico-sdk="$sdk" >"$tmp/build.log" 2>&1 || { tail -20 "$tmp/build.log" >&2; die "the firmware build failed"; }
    ok "firmware built"
    say "building and installing the app"
    case $(uname -s) in
    Linux) ./build.c install --prefix="$prefix" >"$tmp/build.log" 2>&1 ;;
    *) ./build.c install >"$tmp/build.log" 2>&1 ;;
    esac || { tail -20 "$tmp/build.log" >&2; die "the app build failed"; }
    cd /
    ok "installed from source"
}

finish() {
    printf '\n%s%s  Wye is ready!%s\n' "$c_green" "$c_bold" "$c_reset"
    printf '  %sversion%s   %s\n' "$c_dim" "$c_reset" "$("$bin" --version 2>/dev/null || echo unknown)"
    case :${PATH:-}: in
    *:"$(dirname "$bin")":*) printf '  %sopen%s      Wye from your app menu, or run: %swye%s\n' "$c_dim" "$c_reset" "$c_bold" "$c_reset" ;;
    *) printf '  %sopen%s      Wye from your app menu, or run: %s%s%s\n' "$c_dim" "$c_reset" "$c_bold" "$bin" "$c_reset" ;;
    esac
    printf '  %sguide%s     %s#get-started\n' "$c_dim" "$c_reset" "$repo"
    printf '  %sremove%s    curl -fsSL %s/install.sh | sh -s -- --uninstall\n\n' "$c_dim" "$c_reset" "$raw"
}

uninstall() {
    say "removing Wye"
    rm -f "$bin" "$desktop" "$svg" "$png" "$HOME/.config/autostart/wye.desktop"
    if [ "$(uname -s)" = Darwin ]; then
        rm -rf "$HOME/Applications/Wye.app"
    fi
    ok "removed. Your settings and shared folder are still there."
}

main "$@"
