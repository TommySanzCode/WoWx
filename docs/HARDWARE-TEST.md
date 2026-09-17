# First original Xbox test — XBMC / FTP

**September 17 update:** use [the latest hardware build guide](HARDWARE-20260917.md).
The instructions below describe the preserved September 16 checkpoint.

This package uses the unchanged xemu-verified executable from checkpoint
`20260916-1212`. It is a limited playable development client, not the completed
Vanilla port. Hardware graphics, timing and controller transport are unverified.

## 1. Transfer the prepared folder

Your Xbox: **192.168.50.85**. Your PC's Ethernet address: **192.168.50.156**.
Keep both on the same home network; connect the Xbox by Ethernet.

Use your existing FTP client and Xbox login. Copy this entire PC folder:

`.\dist\WOWX-hardware-20260916\WOWX`

to a new application folder on the Xbox:

`E:\Games\WOWX`

Use `F:\Games\WOWX` instead if that drive has more free space. Allow 450 MiB
free. The transfer contains **108 files / 414,119,256 bytes** (about 395 MiB).
Check the FTP queue for failed transfers. `default.xbe`, `world.wxp`, all other
prepared assets and configuration files belong together in that folder.
Launch this XBE directly; the XISO is not needed for the HDD test. Keep the
existing XBMC installation and dashboard boot files intact.

The console package has live controller input, manual login, a fresh entropy
pool, and the PC LAN address prefilled. It contains no account password. It
does not include the newer, still-unverified starter-outfit work.

## 2. Enable the PC game server for this Xbox

Open **PowerShell as administrator** on the PC and run:

```powershell
# Run from the repository root.
.\scripts\hardware-network.ps1
```

Windows requires administrator rights for the firewall rule and TCP forwarding.
The script reuses/starts the workspace server, forwards the PC's LAN ports
3725 and 8086 to its existing loopback server, and updates the realm's advertised
address. The firewall rule permits only **192.168.50.85** on the Private LAN.
It saves the original realm address for restoration and does not reset accounts
or characters. Wait **25 seconds** for the realm-list cache before logging in.
Keep the PC awake while playing. This setup persists until explicitly restored.

This follows Windows' supported port forwarding mechanism for WSL services:
[Microsoft WSL networking](https://learn.microsoft.com/en-us/windows/wsl/networking).
The local server and script preflight passed; a connection originating from the
physical Xbox still needs verification.

## 3. Launch and log in

1. Connect an original Xbox controller to port 1.
2. In **XBMC → File Manager**, browse to `E:\Games\WOWX` (or your chosen drive).
3. Select **default.xbe**. Allow the first load to finish.
4. The server field should show **192.168.50.156:3725**. The local account name
   is prefilled. Read the `username` and `password` entries in
   `.\server\local-credentials.json` on your PC; keep that file private.
5. D-pad to Password, press **A**, then use the on-screen keyboard. **A** enters
   a key, **X** erases, **Y** switches the keyboard page, and **Start** finishes.
6. Press **Start** on the account form to connect. Select **WOWX Local** with A.
7. Select **Xboxer** and press A to enter at his saved position. **Xboxdawn** is
   another saved Human warrior near Northshire Abbey.

Boot and controller navigation can be checked before server setup, but entering
the world requires the PC server. This client reads the Xbox's saved network
configuration; XBMC FTP connectivity alone does not prove the game has the same
settings. If necessary, check the console's saved network settings and DHCP.

## 4. First test: about five minutes

Start with movement, camera and menus in the supplied Human test area.

| Control | Action |
|---|---|
| Left / right stick | Move / camera |
| A | Jump; confirm |
| Y or D-pad left/right | Cycle targets |
| X | Interact, melee or loot |
| B | Cancel; stop attack / clear target |
| LT + A / LT + B | Default attack / Heroic Strike bindings |
| Tap / hold Black | Bags / utility wheel |
| Back | Map |
| Start | Settings/menu |
| Click right stick | Show timing and free-memory diagnostics |
| Start, then Y | Log out to character selection |

Record whether the title appears, the controller responds, login completes and
the character/terrain render. With diagnostics visible, note frame time and
free KiB during movement and combat. The target is **30 fps (~33 ms/frame)**
with **at least 8192 KiB free**. These are acceptance targets, not yet measured
hardware results. A photo or short video of any failure and the diagnostics is
useful; keep passwords off camera.

Terrain currently covers a small prepared area, not every zone. Some character
backgrounds, customization, audio and much of the game/UI remain unfinished.
Do not interpret an unavailable-area message as unrestricted-world support.
Normal gameplay saves progress on the PC server. Allow logout to finish before
powering the console off. Settings and entropy usage are written to `E:\WOWX`.

## Troubleshooting

- **XBE fails to launch / black screen:** note Xbox revision if known, BIOS or
  softmod, video cable/mode, and whether any text appeared. Check the 108-file FTP
  transfer before retrying. No BIOS flashing is part of this test.
- **Assets missing:** confirm all files are next to `default.xbe`, without an
  extra nested `WOWX` directory. A dashboard drive mapping may need investigation
  if the files are present but the client cannot read `D:\`.
- **Login times out:** check the PC remains `192.168.50.156`, the network command
  succeeded, and both devices share the LAN. NordLynx was active on the PC during
  preparation; if LAN connections fail, check its local-network access settings.
- **Realm selection stalls:** the world port is **8086**; the realm must advertise
  the PC LAN address, not xemu's **10.0.2.2**. Wait 25 seconds after setup.
- **Password rejected:** use the disposable WoW credentials, not the FTP login.
- **Authentication eventually stops after many attempts:** the development
  entropy pool is finite and consumption persists on hardware. Prepare a new
  package/pool; do not delete consumption records to reuse old entropy.

The existing telemetry sender targets xemu's private host address. For this
unchanged checkpoint, use the on-screen diagnostics and PC server logs; automatic
hardware telemetry capture still needs a client update.

## End the hardware test

After logging out, run in administrator PowerShell:

```powershell
# Run from the repository root.
.\scripts\hardware-network.ps1 -Mode Restore
```

This restores the previous realm address and removes this test's two forwarding
entries and firewall rule. It leaves the server, all character progress and the
Xbox files in place. Wait 25 seconds before reconnecting in xemu.
