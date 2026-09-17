param([string]$Xemu, [string]$Controller='keyboard', [switch]$Replay, [switch]$SceneReplay, [switch]$QuestReplay, [switch]$CombatReplay, [switch]$TurninReplay, [switch]$InventoryReplay, [switch]$KoboldReplay, [switch]$BoundaryReplay, [switch]$KoboldTurninReplay, [switch]$HearthstoneReplay, [switch]$DeathReplay, [switch]$VendorReplay, [switch]$AppearanceReplay, [switch]$CameraReplay, [switch]$CharacterReplay, [switch]$CharacterScreensReplay, [switch]$StarterQuestReplay, [switch]$CampQuestReplay, [string]$CampReturn, [switch]$SpellbookReplay, [switch]$SpellbookScreensReplay, [switch]$AvatarSwitchReplay, [switch]$ActionbarReplay, [switch]$UtilityReplay, [switch]$QuestLoopReplay, [switch]$WalkReplay, [string]$TestCharacter, [switch]$LegacyText, [switch]$MapReplay, [switch]$CorpseReplay, [switch]$SoakReplay, [int]$SoakSeconds=3600, [switch]$AutoLogin, [switch]$LoginReplay, [switch]$PreviewReplay)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'workspace-paths.ps1')
$Xemu=Resolve-WowxPath 'WOWX_XEMU' $Xemu 'xemu.exe'
$bootrom=Resolve-WowxPath 'WOWX_MCPX' ''
$flashrom=Resolve-WowxPath 'WOWX_BIOS' ''
$harddisk=Resolve-WowxPath 'WOWX_HDD' ''
$root=Split-Path $PSScriptRoot -Parent
$config=Join-Path $root 'config\local.xemu.toml'
$iso=Join-Path $root 'build\wowx.iso'
if (!(Test-Path -LiteralPath $iso)) { throw 'Build the XISO first.' }
if (Get-NetTCPConnection -LocalPort 4444 -State Listen -ErrorAction SilentlyContinue) { throw 'QMP port 4444 is already in use.' }
# Each disposable-disk run gets a new entropy pool, even when the XBE is unchanged.
if (([int]$PreviewReplay.IsPresent+[int]$LoginReplay.IsPresent+[int]$SoakReplay.IsPresent+[int]$CorpseReplay.IsPresent+[int]$MapReplay.IsPresent+[int]$WalkReplay.IsPresent+[int]$QuestLoopReplay.IsPresent+[int]$UtilityReplay.IsPresent+[int]$ActionbarReplay.IsPresent+[int]$AvatarSwitchReplay.IsPresent+[int]$Replay.IsPresent+[int]$SceneReplay.IsPresent+[int]$QuestReplay.IsPresent+[int]$CombatReplay.IsPresent+[int]$TurninReplay.IsPresent+[int]$InventoryReplay.IsPresent+[int]$KoboldReplay.IsPresent+[int]$BoundaryReplay.IsPresent+[int]$KoboldTurninReplay.IsPresent+[int]$HearthstoneReplay.IsPresent+[int]$DeathReplay.IsPresent+[int]$VendorReplay.IsPresent+[int]$AppearanceReplay.IsPresent+[int]$CameraReplay.IsPresent+[int]$CharacterReplay.IsPresent+[int]$CharacterScreensReplay.IsPresent+[int]$StarterQuestReplay.IsPresent+[int]$CampQuestReplay.IsPresent+[int]$SpellbookReplay.IsPresent+[int]$SpellbookScreensReplay.IsPresent) -gt 1) { throw 'Select one replay scenario.' }
if ($CampReturn -and !$CampQuestReplay) { throw 'CampReturn requires CampQuestReplay.' }
if ($TestCharacter -and !($SoakReplay -or $CorpseReplay -or $WalkReplay -or $QuestLoopReplay -or $CampQuestReplay -or $StarterQuestReplay -or $CharacterReplay -or $AvatarSwitchReplay)) { throw 'TestCharacter requires a character/journey replay.' }
if ($AutoLogin -and $LoginReplay) { throw 'AutoLogin and LoginReplay are mutually exclusive.' }
$textArgs=@(); if ($LegacyText) { $textArgs+=@('--legacy-text') }; if ($AutoLogin) { $textArgs+=@('--auto-login') }
$characterArgs=@(); if ($TestCharacter) { $characterArgs=@('--test-character',$TestCharacter) }
if ($PreviewReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --preview }
elseif ($LoginReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --login-replay }
elseif ($SoakReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --soak --soak-seconds $SoakSeconds @characterArgs }
elseif ($CorpseReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --corpse @characterArgs }
elseif ($MapReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --map }
elseif ($WalkReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --walk @characterArgs }
elseif ($QuestLoopReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --quest-loop @characterArgs }
elseif ($UtilityReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --utility }
elseif ($ActionbarReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --actionbar }
elseif ($AvatarSwitchReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --avatar-switch @characterArgs }
elseif ($SpellbookScreensReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --spellbook-screens }
elseif ($SpellbookReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --spellbook }
elseif ($CampQuestReplay) {
    if ($CampReturn) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --camp-quest --camp-return $CampReturn @characterArgs }
    else { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --camp-quest @characterArgs }
}
elseif ($StarterQuestReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --starter-quest @characterArgs }
elseif ($CharacterScreensReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --character-screens }
elseif ($CharacterReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --character @characterArgs }
elseif ($CameraReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --camera }
elseif ($AppearanceReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --appearance }
elseif ($VendorReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --vendor }
elseif ($DeathReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --death }
elseif ($HearthstoneReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --hearthstone }
elseif ($KoboldTurninReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --kobold-turnin }
elseif ($BoundaryReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --boundary }
elseif ($KoboldReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --kobolds }
elseif ($InventoryReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --inventory }
elseif ($TurninReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --turnin }
elseif ($CombatReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --combat }
elseif ($QuestReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --quest }
elseif ($SceneReplay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --scene }
elseif ($Replay) { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs --enabled }
else { python (Join-Path $PSScriptRoot 'prepare-replay.py') @textArgs }
if ($LASTEXITCODE) { throw 'Replay preparation failed.' }
& (Join-Path $PSScriptRoot 'build-xbox.ps1')
python (Join-Path $PSScriptRoot 'write-build-receipt.py')
$emulatorDir=Join-Path $root 'build\emulator'
New-Item -ItemType Directory -Force -Path $emulatorDir | Out-Null
$eeprom=Join-Path $emulatorDir 'eeprom.bin'
if (!(Test-Path -LiteralPath $eeprom)) {
    $eepromSource=Resolve-WowxPath 'WOWX_EEPROM' '' (Join-Path $env:APPDATA 'xemu\xemu\eeprom.bin')
    Copy-Item -LiteralPath $eepromSource -Destination $eeprom
}
@'
[general]
show_welcome = false
[input.bindings]
port1_driver = 'usb-xbox-gamepad'
port1 = __CONTROLLER__
[display]
renderer = 'VULKAN'
[display.quality]
surface_scale = 1
[display.window]
fullscreen_on_startup = false
[audio]
use_dsp = true
[net]
enable = true
[sys.files]
bootrom_path = __BOOTROM__
flashrom_path = __FLASHROM__
eeprom_path = __EEPROM__
hdd_path = __HARDDISK__
'@.Replace('__CONTROLLER__',($Controller | ConvertTo-Json -Compress)).Replace('__EEPROM__',($eeprom | ConvertTo-Json -Compress)).Replace('__BOOTROM__',($bootrom | ConvertTo-Json -Compress)).Replace('__FLASHROM__',($flashrom | ConvertTo-Json -Compress)).Replace('__HARDDISK__',($harddisk | ConvertTo-Json -Compress)) | Set-Content -LiteralPath $config -Encoding utf8
$argsList=@('-config_path',('"'+$config+'"'),'-dvd_path',('"'+$iso+'"'),'-snapshot','-m','64','-qmp','tcp:127.0.0.1:4444,server,nowait')
$proc=Start-Process -FilePath $Xemu -ArgumentList $argsList -PassThru -WindowStyle Hidden -RedirectStandardOutput (Join-Path $root 'build\xemu-stdout.log') -RedirectStandardError (Join-Path $root 'build\xemu-stderr.log')
$proc.Id | Set-Content -LiteralPath (Join-Path $root 'build\xemu.pid')
Write-Output "WOWX xemu PID $($proc.Id); QMP localhost:4444; disposable HDD writes enabled"
