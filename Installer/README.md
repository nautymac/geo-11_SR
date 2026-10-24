# geo-11 SR Installer (Geo11SRInstaller.exe)

**English** · [한국어](README_KR.md)

Double-click `Geo11SRInstaller.exe` and a window opens (no console window; a single .NET Framework 4.x WinForms executable; high-DPI aware).
Pick the game's executable and the rest is detected automatically. The config files are changed according to what you choose in the tabs.
ReShade and GameBridge are not needed. SR weaving is done by `dxgi.dll` (SRWeave), which uses the SR SDK's `IDX11Weaver1` directly.

The packages must sit next to this folder:

| Package | Folder |
|---|---|
| Unity (Unity Universal Fix + geo-11) | `..\Geo-11_Unity\{x64,x32}` |
| Generic geo-11 (Preferred) | `..\Geo-11_DX11\{x64,x32}` |
| Unreal Engine 4 (Universal Fix 2, 64-bit only) | `..\Geo-11_UE4\` |

### DirectX 9 games (dgVoodoo2)

geo-11 handles D3D11 only. So for a DX9 game, dgVoodoo2's `D3D9.dll` (D3D9 → D3D11 translation) goes into the game folder, and generic geo-11 (`Geo-11_DX11`) and SRWeave are installed underneath it. When you choose **DirectX 9 game** in the Kind step (selected automatically from the exe's import table or a `d3d9.dll` string inside the file, when the game is not Unity or UE4), you set the dgVoodoo2 version (`Geo-11_DX9\<version>\x86|x64\D3D9.dll`; 2.79.1 default, 2.86.5, 2.87.3) and "Output at the desktop (panel) resolution", and the engine in the Install step is fixed to generic geo-11. The bitness is read from the exe, so 64-bit DX9 works too. On [Finish] the installer copies `D3D9.dll` (an existing D3D9.dll that is not dgVoodoo is kept as `.geo11sr_bak`) and writes a `dgVoodoo.conf` of its own: `OutputAPI = d3d11_fl11_0`, `Resolution = desktop` (or unforced), `VRAM = 4GB` (or 2GB; with dgVoodoo's default of 256 MB the game crashes at panel resolution), `dgVoodooWatermark = false`. An existing dgVoodoo.conf that the installer did not write is not touched. The Engine column of the games list shows `DX9`, and [Uninstall all] also deletes D3D9.dll and the conf. The **4GB patch** in the DX9 section (optional, `Util.SetLargeAddressAware`) can only be turned on for a 32-bit exe without the LAA flag. It is applied on [Finish] after a confirmation box. It stops a 32-bit game from crashing with `std::bad_alloc` when dgVoodoo2 (panel resolution) + geo-11 (both eyes) + SRWeave (a copy) use up its 2 GB of address space (confirmed in Sine Mora EX). The original is kept as `<exe>.geo11sr_bak`, and [Uninstall all] restores it. Known limitation: under dgVoodoo2, geo-11 fails to create its overlay (`Failed to create Overlay`), so the overlay does not show in DX9 games.

### Unreal Engine 4 games

A `<project>\Binaries\Win64\*-Win64-Shipping.exe` layout is detected as UE4 automatically, and the 32-bit choice is disabled. A full install copies the UF2 package (78 MB) into that folder. Then the installer itself does what `0000_Start_GEO11.cmd` used to do: it places the geo-11 driver from `ShaderFixes\Geo11` in the root and in `ShaderFixes\ResetFix`, copies the HUD profile that matches the exe name (`AUTOHUDUICONFIGS`) to `AutoDepthUIHUD.ini`, cleans up `force_stereo=2`, `get_resolution_from` and the `3dvision2sbs` include in d3dx.ini, and runs the common steps (direct_mode=sbs, stereo values, upscaling, overlay, SRWeave). UF2 keeps the stereo control hotkeys in `d3dxdm.ini`, so they are written there. UF2 has no keys for 3D on/off and for saving the current values, so the installer adds `[KeyToggleStereo]`, `[KeySaveSettings]` and `[CommandListSaveSettings]` to `d3dxdm.ini` and then writes the keys (2026-10-09; the save key did not work in SPRAWL). The geo-11 that ships with UF2 is 0.6.40. It does not know the `persist separation = …` syntax that the Unity package uses (the log says Unrecognised entry), and the `separation` value it reads in a command list is wrong too (the square of the default value). So the installer uses **geo-11 0.6.109** for UF2 installs (the same driver as in the Unity package; placed in the package's `ShaderFixes\Geo11` and `ResetFix`, with the original 0.6.40 in `ShaderFixes\Geo11_0.6.40`), and the save list is the same as Unity's: `pre persist separation = separation` / `pre persist convergence = convergence` (+ `overlay_notice`). For a game already installed with 0.6.40, the driver is replaced with the package's 0.6.109 when the hotkeys are applied (d3d11.dll, d3d11_loader.dll, nvapi64.dll; root, Geo11 and ResetFix). 0.6.109 writes `pre persist separation = …` to `d3dx_user.ini` as soon as a value changes. Startup, weaving and saving with 0.6.109 were confirmed in Steel Rats (2026-10-09). Compatibility with UF2's features as a whole still needs more checking in play. In a 3DMigoto ini, a `;` comment after a value causes the whole line to be ignored, so comments go on separate lines only.
The UE4 section of the Install step: **Install the AA/AO improvements** (appends the package's `Engine_additions_AA.txt` and `Scalability_AA.ini` to the end of `Engine.ini` and `Scalability.ini` in `%LOCALAPPDATA%\<project>\Saved\Config\WindowsNoEditor`, keeps the originals as `*_backup.ini` and sets the files read-only; skipped when the folder does not exist because the game has never been run) and the **UE4 settings...** button. This button does in a window, without a console, what Universal Fix 2's console config tool (`00_UE4-UniversalFix-2_Config.cmd`) used to do: install / remove the AA/AO improvements and the AA variant (Keep the game's AA settings = the tool's ENABLE AA / Force AA = DISABLE AA, for broken transparent objects), Force fullscreen (`d3dx.ini` `full_screen`), the on-screen display OSD (`y20`), VSync mode (the `Vertical Sync` NVIDIA profile token) and the HUD profile (`ShaderFixes\AUTOHUDUICONFIGS\<name>` → `AutoDepthUIHUD.ini`). It can also be opened with the **UE4 settings...** button in the Games tab. **-dx11 launch argument**: UF2 needs the game started with `-dx11` (UF2's `0_Start3D.cmd` also does `start "" "<exe>" -dx11`), so [Finish] puts it into the launcher. For a Steam game it adds `-dx11` to `apps/<appid>/LaunchOptions` in `<Steam>\userdata\<account>\config\localconfig.vdf`. A running Steam rewrites that file, so after a confirmation box the installer closes Steam with `-shutdown`, edits the file and starts Steam again (the original is kept as `localconfig.vdf.geo11sr.bak`). For an Epic/GOG (Heroic) game it goes into `launcherArgs` in `%APPDATA%\heroic\GamesConfig\<appName>.json` (the appName is found through install_path in `legendaryConfig\legendary\installed.json` / `gog_store\installed.json`). You can do this again with the **[Put -dx11 into the launcher]** button in the UE4 section of the Install step, and the Launch column of the games list shows the state as `Steam -dx11` / `Heroic -dx11` / `(no -dx11)`. The installer's [Launch] starts the exe directly with `-dx11` when the game is not a Steam game. The installer also writes `z_ExeChoosenState`, so the game can be started with UF2's `0_Start3D.cmd` too. The aim-key convergence scale, shader hunting and auto convergence mode have no files in this package (Win11 build), so they do not work in the original tool either. The link at the bottom of the window can also open the original tool as it is. Removal is done with UF2's `Uninstall.bat`.

## What to install first

**You do not need an SR panel.** With SR weaving off, it works as a plain geo-11 installer (you choose the output mode). SR weaving is only the step that weaves geo-11's SBS image for a lenticular panel.

| Item | Needed? |
|---|---|
| **SR panel + SR runtime** | Needed **only for SR weaving**. If you turn off "Use SR weaving" in the installer's Display step, you can pick geo-11's output mode (sbs / tab / interlaced, etc.) and use it on other 3D displays such as 3D TVs and polarized monitors. In that case the SR runtime is not needed. On Acer SpatialLabs products, installing **SpatialLabs Experience Center** is enough. It installs the 64-bit runtime (`C:\Program Files\Acer\SpatialLabs\Platform\bin`), the 32-bit runtime (`C:\Program Files (x86)\Simulated Reality\Platform\bin`) and the SR Service / Eye Tracker services together. For other Leia SR panels, install that product's SR Platform runtime. |
| Visual C++ redistributable | Not needed for SRWeave or the installer (statically linked). geo-11 (3DMigoto lineage) uses VC++ 2015–2022 x64 (and x86 for 32-bit games). Most games install it, so it is usually already there. |
| .NET Framework 4.x | For the installer. Windows 10/11 include 4.8 by default. |
| GPU | Any. geo-11 works without NVIDIA 3D Vision (the nvapi64.dll in the packages is a stand-in). |
| Build tools (Visual Studio, CMake) | Only needed to build from source yourself. |

Run games at the SR panel's native resolution (for example 3840×2160), fullscreen or in a borderless window, or the pattern will not line up.
For UE4 Universal Fix 2 games, the installer adds the `-dx11` launch argument for Steam (launch options) and Heroic (Epic/GOG) users. If you start the game from the Epic Games Launcher, add `-dx11` to the launcher's launch arguments yourself.

## Status bar links

At the bottom right of the window there are links to the **HelixMod blog** (https://helixmod.blogspot.com/) and **3D Vision Discord**. You can change the addresses with `helixmod=` and `discord=` in `settings.ini` next to the exe (leave one empty to hide it). For Discord, use an invite link that does not expire. The default Discord address is https://discord.gg/zTnnMrA4GE.

## 3D reports and the compatibility list

- **[Report 3D...]** (Games): opens the GitHub report form in the browser. The game name (Steam/Heroic name), the engine and a setup summary (exe name, bits, store, geo-11/dgVoodoo/OGL3D version, SR weaving, upscaling, Sep/Conv, Windows, GPU) are filled in. Pick the result (Works well / Works with issues / Does not work) and submit. A GitHub Action then rebuilds `COMPATIBILITY.md` / `COMPATIBILITY.ko.md` / `compat.json`, grouped by engine and by game. Folder paths are not included.
- **3D report column**: when the installer starts (and on [Refresh]), it downloads `compat.json` and shows Works / Issues / Doesn't work and the number of reports for each game. Games are matched by Steam AppID, then exe name, then game name. The downloaded list is saved as `compat.json` next to the installer, so it also shows offline.
- For a game that is already in the list, the [Report 3D...] button is disabled. A game without an installed fix cannot be reported.
- Changing the addresses (settings.ini): `compat_list=` (the [Compatibility list] link in the status bar), `compat_json=`, `report=`.

## Language

Korean and English are supported. If the Windows display language is Korean, the installer starts in Korean; otherwise it starts in English. You can switch right away with "English로 바꾸기 / Switch to 한국어" at the top right of the window. The choice is saved in `settings.ini` next to the exe (`lang=ko` or `lang=en`). The values written to the config files (ini keys, key spellings) do not depend on the language. Source: `src\Lang.cs` (Korean → English table).

## Workflow (4 steps)

1. In the **Games** tab, **[Add game (start setup)]** → pick the game's executable. It appears in the list as "Needs setup", and step 1 opens.
2. **Step 1 Kind**: either geo-11 (Unity fix / generic Preferred) or Unreal Engine 4 (Universal Fix 2). It is selected automatically from the exe's folder layout and can be changed. The two groups have different packages, hotkey files and extra options, so they are kept as separate groups → [Next]
3. **Step 2 Install** (bits, Unity version and an existing fix are detected automatically; install scope; for the UE4 group the engine / Unity version rows are hidden and the UE4 additional settings are shown) → [Next]
4. **Step 3 Display** (SR weaving, 3D output mode, upscaling) → [Next]
5. **Step 4 Stereo values** (Separation, Convergence, auto convergence) → **[Finish]** writes everything to the game folder and changes the list entry to "Configured".

Each step has [Cancel] [Back] [Next/Finish] at the bottom. If you press [Cancel] partway, the game folder is not changed and the entry stays in the list as "Needs setup". Nothing is applied before [Finish], so you start again from step 1 with [Change settings]. Hotkeys are saved together with [Finish]. To change only the keys of a game that is already set up, press [Change settings] and then [Apply hotkeys only to current game] in the Hotkeys tab.

## Tabs

| Tab | Contents |
|---|---|
| **Games** | Shows the games with their icons and names. Status (Configured / Needs setup / File missing), engine, bits, SR or not, upscaling (when On, the output resolution or desktop), Separation and Convergence are read live from the config files in the game folder. For Sep/Conv, values saved in game with Ctrl+F7 (`d3dx_user.ini`) take priority when present (geo-11 uses them too). Buttons: Add game (start setup), Change settings, Launch (Steam games via `steam://rungameid/<appid>`, otherwise the exe), Open folder, Refresh, Remove from list, Remove SR weaving only, Uninstall all (Uninstall.bat in the game folder), SRWeave.log. Double-click to launch. The list is saved in `games.list` next to the exe (path + whether setup is finished) |
| **Kind** (step 1) | Choose the kind of fix: geo-11 (Unity / generic) or Unreal Engine 4 Universal Fix 2. Shows the detection result and a description of each group |
| **Install** (step 2) | Shows the game picked with [Add game] (name and path) and automatically checks 32/64-bit, Unity and its version, an existing d3d11.dll (fix) and a foreign dxgi.dll (ReShade). Engine (Unity ↔ generic geo-11), Unity version (enabled only for Unity, default 2019 or newer, selected automatically when detected), bits, install scope (full ↔ add SR weaving only) |
| **Display** | Use SR weaving or not, swap eyes, lens on. With SR off you can choose the 3D output mode (direct_mode: sbs/tab/interlaced/…); with SR on it is fixed to sbs. **Upscaling**: the game renders at a lower resolution and geo-11 scales it up to the panel resolution for output (for when a 4K SR panel runs short of frames). The scaling method is either "Compatibility" (default, `upscale_mode=1`: creates a separate output buffer at panel resolution) or "Performance" (`upscale_mode=0`: draws straight into the game's output buffer; fails in many games). Fullscreen handling and output resolution are under "Show advanced settings"; the defaults (Game may switch fullscreen on and off / desktop) are fine. Writes: `upscaling`, `upscale_mode`, `width`/`height`, `ShaderFixes\upscale.ini` include |
| **Stereo values** | Separation (0-100, one decimal place), Convergence (0.01 or more; some fixes use 100 or more), auto convergence |
| **Hotkeys** | Click a box and press the actual key. It shows as "Ctrl + F6" and is converted to the 3DMigoto/SRWeave spelling when saved. geo-11 stereo controls, the overlay (3DMigoto Hunting) mode and its show/hide key, SRWeave keys (always Ctrl+Alt+key). [Clear] empties a key; "Reset hotkeys to defaults" button |
| **Help** | How to use, default hotkeys, files written, requirements, how to remove |
| **Log** | Detection results and what was applied |

When you load a game that is already set up with [Change settings], its current values (direct_mode, upscaling, stereo values, hotkeys, SRWeave settings) are read and shown. Just change the values and press [Finish].

## Files written by install / apply

| File | Entries |
|---|---|
| `d3dxdm.ini` | `direct_mode`, `dm_separation`, `dm_convergence`, `dm_auto_convergence` |
| `d3dx_user.ini` | If `persist separation` / `persist convergence` saved in game with Ctrl+F7 are present, they are set to the same values (at startup geo-11 reads this file after d3dxdm.ini, so it takes priority; without this change the installer's values would be ignored) |
| `ShaderFixesDM\hotkeys.ini` | `Key =` in each section such as `[KeyToggleStereo]` (e.g. `ctrl VK_F6`) |
| `d3dx.ini` | `hunting` (overlay mode), `toggle_hunting` (overlay show/hide key), `reload_config`, `reload_fixes`, `show_original`; upscaling `upscaling`, `upscale_mode`, `width`, `height` (commented out when automatic) and `[Include] include = ShaderFixes\upscale.ini`; the Unity include line through a call to `UnitySwitch.ps1` (hidden window) |
| `SRWeave.ini` | `weave`, `swap_eyes`, `lens`, `key_weave`, `key_swap`, `key_lens`, `key_test` |
| `dxgi.dll` | Copied when SR is used. A foreign dxgi.dll (ReShade) is backed up as `dxgi.dll.reshade_bak`. When SR is turned off, the SRWeave dxgi.dll is deleted and the backup is restored |

`ShaderCache` and `ShaderCacheDM` are cleared only on a full install (they are kept when only SR is added or values change, so the next start is not slow).

**Package copy and time stamps**: package files are copied with the original modification times. After the copy, the times are corrected so that each `*.bin` in `ShaderFixes` (shader cache compiled by 3DMigoto) is not older than its matching `.hlsl`/`.txt`. geo-11 recompiles at startup when a .bin is older than its source. For UF2's overlay geometry shaders (`overlay_hud*.hlsl`) this compile takes tens of minutes, so it looks as if the game window never opens (confirmed in Steel Rats on 2026-10-09). For the same reason, when you copy a package folder by hand, use a method that keeps the time stamps (robocopy, Explorer copy).

## Default hotkeys

| Group | Key | Action |
|---|---|---|
| geo-11 | Ctrl+T | 3D on / off |
| geo-11 | Ctrl+F3 / Ctrl+F4 | Separation down / up |
| geo-11 | Ctrl+F5 / Ctrl+F6 | Convergence down / up |
| geo-11 | Ctrl+F7 | Save current values (persist in d3dxdm.ini) |
| geo-11 | Ctrl+F1 | Show stereo values overlay |
| Overlay | NumPad 0 | Show / hide the overlay (3DMigoto Hunting: shader info and key hints) — when the overlay mode is 1 or 2 |
| Overlay | F10 | Reload settings / fixes |
| Overlay | F9 (held) | Disable the fix temporarily |
| SRWeave | Ctrl+Alt+W | Weaving on / off |
| SRWeave | Ctrl+Alt+S | Swap eyes |
| SRWeave | Ctrl+Alt+L | Lenticular lens on / off |
| SRWeave | Ctrl+Alt+T | Test pattern (left eye red / right eye blue) |

Conversion rules (for reference): geo-11/3DMigoto uses `ctrl`/`alt`/`shift` or `no_modifiers` followed by the key name (letters and digits as they are, other keys as VK_ names such as `VK_F6`, `VK_NUMPAD0`). SRWeave is fixed to Ctrl+Alt, so only the key is stored (letters and digits, `F1`-`F24`, `NUMPAD0`-`9`, VK_ names such as `VK_SPACE`, anything else as `0x` hex, `none`).

## Building

The sources are `src\Geo11SRInstaller.cs` (UI and behavior), `src\Ue4.cs` (UE4 settings window: replaces the file operations of the UF2 config tool with a window) and `src\Lang.cs` (Korean → English string table). UI text is written in Korean in the code and passes through `Lang.T()`, so when you add new text, add its English counterpart to `Lang.cs` as well. Running `src\build.cmd` builds `Geo11SRInstaller.exe` with the Roslyn csc from Visual Studio (or the .NET Framework 4 csc if that is missing) (`src\app.manifest`: DPI awareness, common controls v6; `src\app.ico`: exe and window icon, 256–16px. The source image is `src\app_source.png` ("geo-11" on a navy background + a "3D" glyph with a stereo offset, drawn with `src\icon_variants.py`), preview `src\app_256.png`). No external libraries.
`src\legacy_powershell\` keeps the PowerShell version from before the exe, for reference.

## Notes

- 32-bit games use the 32-bit SRWeave `dxgi.dll` from `x32` (it uses the SR runtime's 32-bit DLLs such as `SimulatedRealityCore32.dll`). When the SR Platform/SpatialLabs runtime is installed, the 32-bit DLLs are also there, in `C:\Program Files (x86)\...\Platform\bin`.
- Run the game on the SR panel at the panel's native resolution, fullscreen or in a borderless window. If the back buffer and the window size differ, `SRWeave.log` records a WARNING and the pattern is misaligned.
- Without the SR runtime, or with the SR Service stopped, only weaving is off and the game runs normally in SBS.
- If you need a command-line method, use `SR_Install.bat` (drag and drop the game exe onto it) / `SR_Remove.bat` in each package.
