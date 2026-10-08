# Briefcase Player Cap

Player Cap raises the built-in Solo, Duo and Trio player ceilings to 32 on a Windows x64 Deceive Inc. dedicated server. It leaves the server's chosen `MaxPlayers` value in control; it does not force 32 players.

## Install

In Briefcase App, add `https://github.com/EnoPM/Briefcase.PlayerCap/releases/latest/download/catalog.json` as a marketplace source in Settings, then open the server's Mods → Available Mods tab and install Player Cap while the server is stopped. The app enables the mod automatically. Start the server from Briefcase App to apply it.

For manual installation:

1. Install the [latest BriefcaseNative Windows server release](https://github.com/EnoPM/BriefcaseNative/releases/latest) and stop the dedicated server.
2. Download `Briefcase.PlayerCap-windows-x64-<version>.zip` from this mod's latest release.
3. Extract the ZIP directly into the server's `DeceiveInc/Binaries/Win64` directory, beside `DeceiveIncServer-Win64-Shipping.exe`.
4. Open `ue4ss/Mods/mods.txt` and add this line if it is not already present:

```text
briefcaseplayercap : 1
```

5. Start the dedicated server **headless** with Briefcase App. For a manual launch, open PowerShell in Win64 and run:

```powershell
Start-Process -FilePath ".\DeceiveIncServer-Win64-Shipping.exe" -ArgumentList "-unattended -NoSplash -NOCONSOLE -nullrhi -nosound" -WorkingDirectory (Get-Location).Path
```

Player Cap leaves the vanilla `Server Config` window unchanged. Launching Shipping without arguments opens that window and leaves Player Cap inactive for that run. Configure `MaxPlayers` in Briefcase App as described below, then start the server headless.

When upgrading from an older Briefcase mod package, remove the old `Briefcase/Mods/briefcase.player-cap` folder before starting the server, so both versions do not run together. The old `soloLimit` and `duoLimit` configuration does not apply to this UE4SS version; choose the player count through the server's `MaxPlayers` setting instead. You may remove the old `ue4ss/Mods/BriefcasePlayerCap/Data/config.json` if you installed version 0.3.5. On Windows, the new lowercase folder name refers to the same directory as the old mixed-case name.

## Choose the player count

Set `MaxPlayers` to a value from **1 to 32** in Briefcase App or edit `DeceiveInc/Saved/Config/WindowsServer/TripwireServer.ini` while the server is stopped. The setting is under `[/Script/DeceiveInc.TripwireServerSettings]`. Restart after changing it.

The package contains both `ue4ss/Mods/briefcaseplayercap/dlls/main.dll` and `BriefcasePreEntry.dll` in the same mod folder. Both files are required. The second file applies the ceiling before the game reads its player settings on a headless launch.

## Remove

Stop the server, remove `ue4ss/Mods/briefcaseplayercap`, and remove its line from `ue4ss/Mods/mods.txt`. Restart the server.

## License

This mod is licensed under the [MIT License](LICENSE). Third-party components retain their own licenses.
