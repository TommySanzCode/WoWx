param([string]$NxdkDir,[string]$MsysBash)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'workspace-paths.ps1')
$NxdkDir=Resolve-WowxPath 'NXDK_DIR' $NxdkDir
$MsysBash=Resolve-WowxPath 'WOWX_MSYS_BASH' $MsysBash
$env:MSYSTEM='MINGW64'
$env:CHERE_INVOKING='1'
$env:NXDK_DIR=$NxdkDir
Push-Location (Split-Path $PSScriptRoot -Parent)
try {
    if (Test-Path -LiteralPath 'build\xbox\PTTEST.BIN') { throw 'Offline portrait markers belong only in isolated fixture discs.' }
    if (!(Test-Path -LiteralPath 'build\xbox\input.rpl') -or !(Test-Path -LiteralPath 'build\xbox\scenario.bin')) { python scripts/prepare-replay.py }
    $entropy=New-Object byte[] 4112
    [System.Security.Cryptography.RandomNumberGenerator]::Fill($entropy)
    [System.IO.File]::WriteAllBytes((Join-Path (Get-Location) 'build\xbox\entropy.bin'),$entropy)
    & $MsysBash -lc 'export NXDK_DIR="$(cygpath -u "$NXDK_DIR")"; exec make -j4'
    if ($LASTEXITCODE) { throw 'Xbox build failed; inspect the compiler output.' }
} finally { Pop-Location }
