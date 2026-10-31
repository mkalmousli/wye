# Installs Wye for this user, no administrator rights needed:
#   irm https://raw.githubusercontent.com/mkalmousli/wye/main/install.ps1 | iex
# Remove it again:
#   $env:WYE_UNINSTALL=1; irm https://raw.githubusercontent.com/mkalmousli/wye/main/install.ps1 | iex
$ErrorActionPreference = 'Stop'
& {
    $repo = 'https://github.com/mkalmousli/wye'
    $releases = "$repo/releases/latest/download"
    $raw = 'https://raw.githubusercontent.com/mkalmousli/wye/main'
    $dir = Join-Path $env:LOCALAPPDATA 'Programs\Wye'
    $exe = Join-Path $dir 'Wye.exe'
    $lnk = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Wye.lnk'

    function Say($text) { Write-Host "==> $text" -ForegroundColor Cyan }
    function Ok($text) { Write-Host "  ✓ $text" -ForegroundColor Green }
    function Warn($text) { Write-Host "  ! $text" -ForegroundColor Yellow }
    function Fail($text) { throw "  ✗ wye: $text" }

    Write-Host ''
    Write-Host '  __      ___   _  ___' -ForegroundColor Magenta
    Write-Host '  \ \    / / | | |/ _ \' -ForegroundColor Magenta
    Write-Host '   \ \/\/ /| |_| |  __/' -ForegroundColor Magenta
    Write-Host '    \_/\_/  \__, |\___|' -ForegroundColor Magenta
    Write-Host '            |___/' -ForegroundColor Magenta
    Write-Host '  one keyboard, two computers, three wires' -ForegroundColor DarkGray
    Write-Host ''

    if ($env:WYE_UNINSTALL) {
        Say 'removing Wye'
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $dir, $lnk
        Ok 'removed. Your settings and shared folder are still there.'
        return
    }

    if (-not [Environment]::Is64BitOperatingSystem) { Fail 'Wye needs 64-bit Windows.' }
    $arch = $env:PROCESSOR_ARCHITECTURE
    if ($arch -ne 'AMD64') {
        Fail "no release for Windows on $arch yet. Build from source: $repo#build"
    }

    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Say 'looking for the latest release'
    try {
        $manifest = Invoke-RestMethod "$releases/latest.json"
    } catch {
        Fail "there is no release yet. Build from source: $repo#build"
    }
    $asset = $manifest.assets.'windows-x64'
    if (-not $asset -or -not $asset.sha512) { Fail 'the release has no checksum for this system; not installing.' }
    Ok "found Wye $($manifest.version) for Windows x64 ($($asset.file))"

    $tmp = Join-Path ([IO.Path]::GetTempPath()) "wye-$([guid]::NewGuid()).exe"
    try {
        Say "downloading $($asset.file)"
        $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
        if ($curl) {
            # curl.exe ships with Windows 10+ and draws a real progress bar.
            & $curl.Source -fL --progress-bar $asset.url -o $tmp
            if ($LASTEXITCODE -ne 0) { Fail 'could not download the app' }
        } else {
            Invoke-WebRequest $asset.url -OutFile $tmp -UseBasicParsing
        }

        Say 'checking the download'
        if ((Get-FileHash $tmp -Algorithm SHA512).Hash -ne $asset.sha512.ToUpper()) {
            Fail "the download's checksum does not match; not installing."
        }
        Ok 'SHA-512 matches'

        Say "installing to $dir"
        New-Item -ItemType Directory -Force $dir | Out-Null
        if (Test-Path $exe) { Move-Item -Force $exe "$exe.old" } # works while the old version runs
        Move-Item -Force $tmp $exe
        Ok 'app installed'
    } finally {
        Remove-Item -Force -ErrorAction SilentlyContinue $tmp
    }

    Say 'adding the Start menu entry'
    $shell = New-Object -ComObject WScript.Shell
    $s = $shell.CreateShortcut($lnk)
    $s.TargetPath = $exe
    $s.Description = 'Wye'
    $s.Save()
    Ok 'Start menu entry added'

    Write-Host ''
    Write-Host '  Wye is ready!' -ForegroundColor Green
    Write-Host "  open      Wye from the Start menu, or run: `"$exe`""
    Write-Host "  guide     $repo#get-started" -ForegroundColor DarkGray
    Write-Host "  remove    `$env:WYE_UNINSTALL=1; irm $raw/install.ps1 | iex" -ForegroundColor DarkGray
    Write-Host ''
}
