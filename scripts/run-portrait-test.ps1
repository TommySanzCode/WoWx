param([Parameter(Mandatory=$true)][string]$Fixture,[Parameter(Mandatory=$true)][string]$Capture,[int]$Seconds=300,[string]$Xemu)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'workspace-paths.ps1')
$Xemu=Resolve-WowxPath 'WOWX_XEMU' $Xemu 'xemu.exe'
$root=Split-Path $PSScriptRoot -Parent
$folder=(Resolve-Path -LiteralPath $Fixture).Path
$buildRoot=(Resolve-Path -LiteralPath (Join-Path $root 'build')).Path+'\'
if (!$folder.StartsWith($buildRoot,[StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture must be inside this workspace build directory.' }
if (Get-Process xemu -ErrorAction SilentlyContinue) { throw 'An emulator is already active. Preserve it before starting a fixture.' }
if (Get-NetTCPConnection -LocalPort 4444 -State Listen -ErrorAction SilentlyContinue) { throw 'QMP port 4444 is occupied.' }
if (!(Test-Path -LiteralPath (Join-Path $folder 'portrait.iso'))) { throw 'Prepare the isolated fixture first.' }
if ((Get-Content -LiteralPath (Join-Path $folder 'disc\PTTEST.BIN') -Raw) -cnotin @('WXPF0001','WXPF0002','WXPF0003','WXPF0004','WXPF0005','WXPF0006','WXPF0007','WXPF0008','WXPF0009','WXPF0010','WXPF0011','WXPF0012','WXPF0013')) { throw 'Offline marker is missing or invalid.' }
if (Test-Path -LiteralPath (Join-Path $folder 'disc\testauth.bin')) { throw 'Offline fixture must not contain credentials.' }
if (Test-Path -LiteralPath $Capture) { throw 'Use a new capture path to preserve existing evidence.' }
$configText=Get-Content -LiteralPath (Join-Path $root 'config\local.xemu.toml') -Raw
$eepromLine='eeprom_path = '+((Join-Path $folder 'eeprom.bin') | ConvertTo-Json -Compress)
$configText=[regex]::Replace($configText,'(?m)^eeprom_path\s*=.*$',{param($match) $eepromLine})
if ($configText -match '\[display.quality\]') { throw 'Review the configured rendering scale before preparing this fixture config.' }
$configText += "`n[display.quality]`nsurface_scale = 1`n"
if (!(Test-Path -LiteralPath (Join-Path $folder 'eeprom.bin'))) { Copy-Item -LiteralPath (Join-Path $root 'build\emulator\eeprom.bin') -Destination (Join-Path $folder 'eeprom.bin') }
$configText | Set-Content -LiteralPath (Join-Path $folder 'xemu.toml') -Encoding utf8
$launchArgs=@('-config_path',('"'+$folder+'\xemu.toml"'),'-dvd_path',('"'+$folder+'\portrait.iso"'),'-snapshot','-m','64','-qmp','tcp:127.0.0.1:4444,server,nowait')
$proc=Start-Process -FilePath $Xemu -ArgumentList $launchArgs -PassThru -WindowStyle Hidden -RedirectStandardOutput (Join-Path $folder 'stdout.log') -RedirectStandardError (Join-Path $folder 'stderr.log')
$proc.Id | Set-Content -LiteralPath (Join-Path $folder 'xemu.pid')
Write-Output "Offline portrait fixture PID $($proc.Id); native scale, 64 MiB, disposable HDD writes."
python (Join-Path $root 'tools\record_telemetry.py') --seconds $Seconds --output $Capture --guest-pid-file (Join-Path $folder 'xemu.pid')
if ($LASTEXITCODE) { throw 'Capture failed. Preserve the emulator and mounted fixture for diagnosis.' }
