param(
    [ValidateSet('Enable','Restore','Check')][string]$Mode='Enable',
    [string]$PcIp='192.168.50.156',
    [string]$XboxIp='192.168.50.85'
)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$statePath=Join-Path $root 'server\hardware-network.json'
$ruleName='WOWX-Hardware-Test-TCP'
$registry='HKLM:\SYSTEM\CurrentControlSet\Services\PortProxy\v4tov4\tcp'
function Assert-IPv4([string]$Value) {
    $parsed=$null
    if (!($Value -match '^\d{1,3}(\.\d{1,3}){3}$') -or ![Net.IPAddress]::TryParse($Value,[ref]$parsed)) { throw "Invalid IPv4 address: $Value" }
}
function Read-Realm {
    $line=& wsl.exe -d Ubuntu -u root --exec mariadb --batch --raw --skip-column-names -e 'SELECT address,localAddress,localSubnetMask,port FROM wowx_logon.realmlist WHERE id=1;'
    if ($LASTEXITCODE -or !$line -or @($line).Count -ne 1) { throw 'Cannot read the existing WOWX realm.' }
    $fields=$line.Split("`t")
    if ($fields.Count -ne 4) { throw 'Unexpected realm schema.' }
    for ($i=0;$i -lt 3;$i++) { Assert-IPv4 $fields[$i] }
    if ($fields[3] -ne '8086') { throw 'Unexpected realm port; configuration was left unchanged.' }
    return @{ address=$fields[0]; localAddress=$fields[1]; mask=$fields[2]; port=8086 }
}
function Write-Realm($Realm) {
    foreach ($value in @($Realm.address,$Realm.localAddress,$Realm.mask)) { Assert-IPv4 $value }
    $sql="UPDATE wowx_logon.realmlist SET address='$($Realm.address)',localAddress='$($Realm.localAddress)',localSubnetMask='$($Realm.mask)' WHERE id=1 AND port=8086;"
    & wsl.exe -d Ubuntu -u root --exec mariadb -e $sql
    if ($LASTEXITCODE) { throw 'Realm address update failed.' }
}
function Proxy-Value([string]$Address,[int]$Port) {
    $key=Get-Item -LiteralPath $registry -ErrorAction SilentlyContinue
    if ($key) { return $key.GetValue("$Address/$Port") }
    return $null
}
function Require-Admin {
    $principal=[Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent())
    if (!$principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw 'Run this command in PowerShell opened with Run as administrator. Windows requires it for the scoped firewall rule and TCP forwarding.'
    }
}
if ($Mode -eq 'Restore') {
    Require-Admin
    if (!(Test-Path -LiteralPath $statePath)) { Write-Output 'No WOWX hardware network setup to restore.'; return }
    $state=Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    Assert-IPv4 $state.pc_ip
    $current=Read-Realm
    if ($current.address -ne $state.pc_ip -and $current.address -ne $state.original.address) {
        throw 'Realm address changed outside this setup; refusing to overwrite it.'
    }
    foreach ($port in 3725,8086) {
        $value=Proxy-Value $state.pc_ip $port
        if ($value -and $value -ne "127.0.0.1/$port") { throw 'Forwarding changed outside this setup; refusing to overwrite it.' }
    }
    Write-Realm $state.original
    foreach ($port in 3725,8086) {
        if (Proxy-Value $state.pc_ip $port) {
            & netsh.exe interface portproxy delete v4tov4 "listenaddress=$($state.pc_ip)" "listenport=$port" | Out-Null
            if ($LASTEXITCODE) { throw 'Could not remove the WOWX forwarding rule.' }
        }
    }
    Get-NetFirewallRule -Name $ruleName -ErrorAction SilentlyContinue | Remove-NetFirewallRule
    Remove-Item -LiteralPath $statePath
    Write-Output 'Original realm address restored; WOWX hardware forwarding/firewall rule removed. Allow 25 seconds for the realm cache.'
    return
}
Assert-IPv4 $PcIp
Assert-IPv4 $XboxIp
if ($PcIp -eq $XboxIp) { throw 'The PC and Xbox must have different addresses.' }
$interface=Get-NetIPAddress -AddressFamily IPv4 -IPAddress $PcIp -ErrorAction Stop
$profile=Get-NetConnectionProfile -InterfaceIndex $interface.InterfaceIndex
if ($profile.NetworkCategory -ne 'Private') { throw 'Use a trusted Private LAN connection; this script will not change the network profile.' }
& (Join-Path $PSScriptRoot 'start-server.ps1')
foreach ($port in 3725,8086) {
    $probe=[Net.Sockets.TcpClient]::new()
    try { $attempt=$probe.ConnectAsync('127.0.0.1',$port); if (!$attempt.Wait(3000) -or !$probe.Connected) { throw "Local server port $port is unavailable." } }
    finally { $probe.Dispose() }
}
$original=Read-Realm
if ($Mode -eq 'Check') {
    Write-Output "Local server ports 3725/8086 reachable; PC $PcIp on Private LAN; Xbox target $XboxIp."
    Write-Output "Current realm endpoint: $($original.address):8086. No network settings changed."
    return
}
Require-Admin
if (Test-Path -LiteralPath $statePath) {
    $state=Get-Content -LiteralPath $statePath -Raw | ConvertFrom-Json
    if ($state.pc_ip -ne $PcIp -or $state.xbox_ip -ne $XboxIp) { throw 'Restore the previous hardware setup before selecting different addresses.' }
    $original=$state.original
} else {
    if (Get-NetFirewallRule -Name $ruleName -ErrorAction SilentlyContinue) { throw 'A firewall rule already uses this name; refusing to replace it.' }
    foreach ($port in 3725,8086) { if (Proxy-Value $PcIp $port) { throw "A forwarding rule already owns ${PcIp}:$port." } }
    @{pc_ip=$PcIp; xbox_ip=$XboxIp; original=$original} | ConvertTo-Json | Set-Content -LiteralPath $statePath -Encoding utf8
}
foreach ($port in 3725,8086) {
    $value=Proxy-Value $PcIp $port
    if ($value -and $value -ne "127.0.0.1/$port") { throw 'Forwarding changed outside this setup; refusing to replace it.' }
    if (!$value) {
        & netsh.exe interface portproxy add v4tov4 "listenaddress=$PcIp" "listenport=$port" connectaddress=127.0.0.1 "connectport=$port" | Out-Null
        if ($LASTEXITCODE) { throw 'Forwarding failed. Use -Mode Restore to undo this setup.' }
    }
}
if (!(Get-NetFirewallRule -Name $ruleName -ErrorAction SilentlyContinue)) {
    New-NetFirewallRule -Name $ruleName -DisplayName 'WOWX hardware test from this Xbox only' -Direction Inbound -Action Allow -Profile Private -Protocol TCP -LocalAddress $PcIp -LocalPort 3725,8086 -RemoteAddress $XboxIp | Out-Null
}
Start-Service iphlpsvc
Write-Realm @{address=$PcIp;localAddress=$PcIp;mask='255.255.255.255'}
Write-Output "Hardware server ready at ${PcIp}:3725, world port 8086; firewall permits only $XboxIp on the Private LAN."
Write-Output 'Wait 25 seconds before login. Keep this PC awake; the setup persists until -Mode Restore.'
