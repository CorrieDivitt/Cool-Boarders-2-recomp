# Build the bring-your-own-disc release: a "self-compile kit".
#
# The kit contains the setup host (the runtime built WITHOUT any game code),
# this project's original sources, the PSXRecomp framework tree, its recompiler
# tools and OpenBIOS. On first run the player picks their own disc; the setup
# wizard generates the game C and compiles the game on their machine.
# Nothing from the disc is in the kit, and the IP guard audits the zip.
#
# Prerequisites in this PowerShell session: cmake, ninja and clang on PATH (the
# PSXRecomp toolchain), Python 3, Git for Windows. A built build-recompiler/
# (created by the project setup) must exist.
#
# Usage, from the repository root:
#   powershell -ExecutionPolicy Bypass -File tools\build_selfcompile_kit.ps1 -RuntimeBin C:\path\to\toolchain\bin
param(
    [string]$RuntimeBin = $env:PSXRECOMP_RUNTIME_BIN_DIR,   # folder with the toolchain's runtime DLLs
    [string]$BuildDir = "build-setup"
)
$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
Set-Location $Root

function Step([string]$name, [scriptblock]$cmd) {
    Write-Host "`n=== $name ===" -ForegroundColor Cyan
    $prev = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    & $cmd 2>&1 | ForEach-Object { "$_" }
    $ErrorActionPreference = $prev
    if ($LASTEXITCODE -ne 0) { throw "$name failed (exit $LASTEXITCODE)" }
}

# Git for Windows bash (not the WSL bash.exe in System32).
$bash = $null
foreach ($g in @((Get-Command git -ErrorAction SilentlyContinue | ForEach-Object { Split-Path (Split-Path $_.Source) }),
                 "$env:ProgramFiles\Git", "$env:LOCALAPPDATA\Programs\Git")) {
    if ($g -and (Test-Path "$g\bin\bash.exe")) { $bash = "$g\bin\bash.exe"; break }
}
if (-not $bash) { throw 'Git for Windows bash not found (winget install Git.Git).' }
$py = (Get-Command python -ErrorAction SilentlyContinue).Source
if (-not $py) { $py = (Get-Command py -ErrorAction SilentlyContinue).Source }
if (-not (Test-Path 'build-recompiler')) { throw 'build-recompiler/ not found: run the project setup or build the recompiler first.' }

Step 'Configure setup host (no game code)' {
    cmake -S . -B $BuildDir -G Ninja -DCMAKE_BUILD_TYPE=Release `
          -DPSXRECOMP_FORCE_SETUP_HOST=ON -DBUILD_TESTING=OFF -DPSX_SDL_BACKEND=SDL3
}
Step 'Build setup host' { cmake --build $BuildDir --target psx-runtime }

$env:CHERE_INVOKING = '1'
# Git for Windows has no `zip`; put a Python-backed stand-in on PATH if needed.
& $bash -c 'command -v zip >/dev/null 2>&1'
if ($LASTEXITCODE -ne 0) { $env:Path = "$Root\tools\zipshim;$env:Path" }
if ($RuntimeBin) { $env:PSXRECOMP_RUNTIME_BIN_DIR = $RuntimeBin }
Step 'Package kit' {
    & $bash psxrecomp/tools/package_setup_host.sh `
        --build-dir $BuildDir --artifact windows-x64 --zip-prefix CoolBoarders2Recomp-selfcompile `
        --exe-name CoolBoarders2_Recompiled --display-name 'Cool Boarders 2 Recompiled' `
        --recompiler-build build-recompiler --exclude-dev-mods `
        --disc-hint 'your own legally obtained Cool Boarders 2 (USA, SCUS-94358) disc image: BIN/CUE with all 21 tracks' `
        --project-file CMakeLists.txt --project-file game.toml --project-file VERSION `
        --project-file game_options.toml --project-file codegen_setup.c --project-file codegen_setup.h `
        --project-file symbols.toml --project-file psx_symbols.h --project-file disc_probe.json `
        --project-file catalog_identity.json --project-file README.md --project-file LEGAL.md `
        --project-dir seeds --project-dir src --project-dir assets
}

$zip = Get-ChildItem -Path $Root -Recurse -Filter 'CoolBoarders2Recomp-selfcompile*.zip' -ErrorAction SilentlyContinue |
       Where-Object { $_.FullName -notmatch '\\psxrecomp\\' } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if (-not $zip) { throw 'Packaging finished but no kit zip was found.' }
$stage = Join-Path $Root 'dist\stage-setup-windows-x64'
Step 'Prune kit (framework tests/docs/caches, other consoles'' art)' { & $py tools/kit_prune.py $stage $zip.FullName }
Step "IP audit of $($zip.Name)" { & $py tools/ip_guard.py --release $zip.FullName }
Write-Host "`nKit ready and audited: $($zip.FullName)" -ForegroundColor Green
