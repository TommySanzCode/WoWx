$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$linuxRoot=& wsl.exe -d Ubuntu -u root -- wslpath -a $root.Replace('\','/')
if ($LASTEXITCODE -or !$linuxRoot) { throw 'Cannot resolve this workspace inside WSL Ubuntu.' }
$linuxRoot=$linuxRoot.Trim()
& wsl.exe -d Ubuntu -u root -- python3 "$linuxRoot/scripts/start-server.py"
if ($LASTEXITCODE) { throw 'Local test server startup failed; inspect server/logs.' }
