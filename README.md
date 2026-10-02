# Briefcase Player Cap

Player Cap raises the built-in Solo, Duo and Trio player ceilings to 32 on a Windows x64 Deceive Inc. dedicated server. It leaves the server's chosen `MaxPlayers` value in control; it does not force 32 players.

## Install

1. Install the [latest BriefcaseNative Windows server release](https://github.com/EnoPM/BriefcaseNative/releases/latest) and stop the dedicated server.
2. Download `Briefcase.PlayerCap-windows-x64-<version>.zip` from this mod's latest release.
3. Extract the ZIP directly into the server's `DeceiveInc/Binaries/Win64` directory, beside `DeceiveIncServer-Win64-Shipping.exe`.
4. Open `ue4ss/Mods/mods.txt` and add this line if it is not already present:

```text
BriefcasePlayerCap : 1
```

5. Start `DeceiveIncServer-Win64-Shipping.exe` with Win64 as its working directory. The Briefcase `version.dll` loads this mod through UE4SS.

When upgrading from an older Briefcase mod package, remove the old `Briefcase/Mods/briefcase.player-cap` folder before starting the server, so both versions do not run together. The old `soloLimit` and `duoLimit` configuration does not apply to this UE4SS version; choose the player count through the server's `MaxPlayers` setting instead. You may remove the old `ue4ss/Mods/BriefcasePlayerCap/Data/config.json` if you installed version 0.3.5.

## Choose the player count

Set `MaxPlayers` to a value from **1 to 32** in the server configuration. You can use Briefcase Server Manager or edit `DeceiveInc/Saved/Config/WindowsServer/TripwireServer.ini` while the server is stopped. The setting is under `[/Script/DeceiveInc.TripwireServerSettings]`. Restart after changing it.

The package contains both `ue4ss/Mods/BriefcasePlayerCap/dlls/main.dll` and `BriefcasePreEntry.dll` in the same mod folder. Both files are required. The second file applies the ceiling before the game reads its player settings.

## Remove

Stop the server, remove `ue4ss/Mods/BriefcasePlayerCap`, and remove its line from `ue4ss/Mods/mods.txt`. Restart the server.
