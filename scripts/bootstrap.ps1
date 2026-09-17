param([string]$NxdkDir)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'workspace-paths.ps1')
$NxdkDir=Resolve-WowxPath 'NXDK_DIR' $NxdkDir
$root=Split-Path $PSScriptRoot -Parent
$pins=Get-Content -LiteralPath (Join-Path $root 'config\dependencies.json') -Raw | ConvertFrom-Json
$dxtHeader=Join-Path $root $pins.stb_dxt.path
if (!(Test-Path -LiteralPath $dxtHeader) -or (Get-FileHash -LiteralPath $dxtHeader -Algorithm SHA256).Hash.ToLowerInvariant() -ne $pins.stb_dxt.sha256) { throw 'Vendored stb_dxt checksum mismatch.' }
New-Item -ItemType Directory -Force -Path (Join-Path $root 'upstream') | Out-Null
foreach ($name in @('wowee','stormlib','glm','libtommath','vmangos')) {
    $pin=$pins.$name
    if ($pin.commit -notmatch '^[0-9a-f]{40}$') { throw "Invalid revision for $name" }
    $target=Join-Path $root "upstream\$name"
    if (!(Test-Path -LiteralPath (Join-Path $target '.git'))) {
        if (Test-Path -LiteralPath $target) { throw "Existing non-Git directory: $target" }
        & git clone --quiet --filter=blob:none --no-checkout $pin.url $target
        if ($LASTEXITCODE) { throw "Clone failed: $name" }
        if ($name -eq 'wowee') {
            & git -C $target sparse-checkout set src include tools tests docs cmake Data/expansions/classic extern
            if ($LASTEXITCODE) { throw 'WoWee sparse checkout failed.' }
        }
        & git -C $target fetch --quiet --depth 1 origin $pin.commit
        if ($LASTEXITCODE) { throw "Pinned fetch failed: $name" }
        & git -C $target checkout --quiet --detach $pin.commit
        if ($LASTEXITCODE) { throw "Checkout failed: $name" }
    }
    $actual=(& git -C $target rev-parse HEAD).Trim()
    if ($LASTEXITCODE -or $actual -ne $pin.commit) { throw "Revision mismatch: $name" }
    & git -C $target diff --quiet
    if ($LASTEXITCODE) { throw "Tracked upstream files were modified: $name" }
    Write-Output "$name pinned at $actual"
}
$nxdkRevision=(& git -C $NxdkDir rev-parse HEAD).Trim()
if ($LASTEXITCODE -or $nxdkRevision -ne $pins.nxdk.commit) { throw 'nxdk revision does not match the pinned toolchain.' }
New-Item -ItemType Directory -Force -Path (Join-Path $root 'build\xbox') | Out-Null
$downloads=Join-Path $root 'build\downloads'
New-Item -ItemType Directory -Force -Path $downloads | Out-Null
$database=Join-Path $downloads 'vmangos-db.zip'
if (!(Test-Path -LiteralPath $database)) { Invoke-WebRequest -Uri $pins.vmangos_database.url -OutFile $database }
$databaseHash=(Get-FileHash -LiteralPath $database -Algorithm SHA256).Hash.ToLowerInvariant()
if ($databaseHash -ne $pins.vmangos_database.sha256) { throw 'vMaNGOS database snapshot checksum mismatch.' }
Write-Output 'Pinned source check complete. Game assets and nxdk are separate prerequisites.'
