# Installs Wye for this user, no administrator rights needed:
#   irm https://raw.githubusercontent.com/mkalmousli/wye/main/install.ps1 | iex
# Remove it again:
#   $env:WYE_UNINSTALL=1; irm https://raw.githubusercontent.com/mkalmousli/wye/main/install.ps1 | iex
$ErrorActionPreference = 'Stop'
& {
    $releases = 'https://github.com/mkalmousli/wye/releases/latest/download'
    $dir = Join-Path $env:LOCALAPPDATA 'Programs\Wye'
    $exe = Join-Path $dir 'Wye.exe'
    $lnk = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Wye.lnk'

    if ($env:WYE_UNINSTALL) {
        Remove-Item -Recurse -Force -ErrorAction SilentlyContinue $dir, $lnk
        Write-Host 'wye: removed. Your settings and shared folder are still there.'
        return
    }

    $ProgressPreference = 'SilentlyContinue'
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    try {
        $manifest = Invoke-RestMethod "$releases/latest.json"
    } catch {
        throw 'wye: there is no release yet. See https://github.com/mkalmousli/wye#build to build from source.'
    }
    $want = $manifest.assets.'windows-x64'.sha512
    if (-not $want) { throw 'wye: the release has no checksum for this system; not installing.' }

    $tmp = Join-Path ([IO.Path]::GetTempPath()) "wye-$([guid]::NewGuid()).exe"
    try {
        Invoke-WebRequest "$releases/Wye-windows-x64.exe" -OutFile $tmp -UseBasicParsing
        if ((Get-FileHash $tmp -Algorithm SHA512).Hash -ne $want.ToUpper()) {
            throw "wye: the download's checksum does not match; not installing."
        }
        New-Item -ItemType Directory -Force $dir | Out-Null
        if (Test-Path $exe) { Move-Item -Force $exe "$exe.old" } # works while the old version runs
        Move-Item -Force $tmp $exe
    } finally {
        Remove-Item -Force -ErrorAction SilentlyContinue $tmp
    }

    $shell = New-Object -ComObject WScript.Shell
    $s = $shell.CreateShortcut($lnk)
    $s.TargetPath = $exe
    $s.Description = 'Wye'
    $s.Save()
    Write-Host "wye: installed. Open Wye from the Start menu, or run: `"$exe`""
}
