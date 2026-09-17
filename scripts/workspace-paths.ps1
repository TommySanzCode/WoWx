# Machine-specific paths stay in the environment or ignored local settings.
$wowxPathSettings=@{}
$wowxPathSettingsFile=Join-Path (Split-Path $PSScriptRoot -Parent) 'config\local.paths.json'
if (Test-Path -LiteralPath $wowxPathSettingsFile) {
    $settings=Get-Content -LiteralPath $wowxPathSettingsFile -Raw | ConvertFrom-Json
    foreach ($property in $settings.PSObject.Properties) { $wowxPathSettings[$property.Name]=[string]$property.Value }
}
function Resolve-WowxPath([string]$Name,[string]$Value,[string]$Fallback='') {
    if (!$Value) { $Value=[Environment]::GetEnvironmentVariable($Name) }
    if (!$Value -and $wowxPathSettings.ContainsKey($Name)) { $Value=$wowxPathSettings[$Name] }
    if (!$Value) { $Value=$Fallback }
    if (!$Value) { throw "Set $Name or add it to config/local.paths.json (ignored by Git)." }
    return $Value
}
