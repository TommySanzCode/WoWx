param([string]$MsysBash)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'workspace-paths.ps1')
$MsysBash=Resolve-WowxPath 'WOWX_MSYS_BASH' $MsysBash
$env:MSYSTEM = 'MINGW64'
$env:CHERE_INVOKING = '1'
Push-Location (Split-Path $PSScriptRoot -Parent)
try { & $MsysBash scripts/build-tools.sh; if ($LASTEXITCODE) { throw 'Host tool build failed' } }
finally { Pop-Location }
