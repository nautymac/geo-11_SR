# geo-11 SR

**English** · [한국어](README.ko.md)

geo-11 is a 3DMigoto-based stereo 3D driver that renders games side-by-side (SBS). This project adds **SRWeave**, a small `dxgi.dll` that hooks the game's real `IDXGISwapChain::Present` and weaves that SBS frame for a lenticular (glasses-free) SR panel such as Acer SpatialLabs or a Leia SR display, using the SR SDK weaver directly. No ReShade, no GameBridge. 64-bit and 32-bit games are supported.

It also works **without** an SR panel: switch SR weaving off and it is a plain geo-11 installer for any other 3D display (SBS, top-bottom, interlaced output).

![installer](Installer/screenshot.png)

## Download and install

Download **`geo-11_SR_all-in-one.zip`** from [Releases](../../releases) and unzip it anywhere. It is one zip with the installer and the three fix packages as sub-folders, already in the layout the installer expects:

```
geo-11_SR\
  Installer\Geo11SRInstaller.exe
  Geo-11_Unity\
  Geo-11_DX11\
  Geo-11_UE4\
```

| Folder | Contents |
|---|---|
| `Installer\` | GUI installer (English / Korean) and the Korean manual |
| `Geo-11_Unity\` | Unity Universal Fix + geo-11 0.6.109 + SRWeave (x64, x32) |
| `Geo-11_DX11\` | Generic geo-11 "Preferred" + SRWeave (x64, x32) |
| `Geo-11_UE4\` | Unreal Engine 4 Universal Fix 2 (Win11 build) + SRWeave, 64-bit only. geo-11 updated to 0.6.109; the original 0.6.40 is kept in `ShaderFixes\Geo11_0.6.40` |

Keep the folders side by side: the installer finds the packages by relative path.

## Requirements

| Item | Needed? |
|---|---|
| SR panel + SR runtime | Only for SR weaving. On Acer SpatialLabs devices, **SpatialLabs Experience Center** alone is enough: it installs the 64-bit runtime (`C:\Program Files\Acer\SpatialLabs\Platform\bin`), the 32-bit runtime (`C:\Program Files (x86)\Simulated Reality\Platform\bin`) and the SR Service. Other Leia SR panels: install that product's SR Platform runtime. Without SR weaving nothing SR-related is needed. |
| Visual C++ redistributable | Not for SRWeave or the installer (statically linked). geo-11 itself (3DMigoto lineage) uses VC++ 2015–2022 x64 (and x86 for 32-bit games), which almost every game installs already. |
| .NET Framework 4.x | For the installer. Windows 10/11 ship 4.8. |
| GPU | Any. geo-11 does not need NVIDIA 3D Vision; the `nvapi64.dll` in the packages is a stand-in. |
| Visual Studio, CMake | Only to build from source. |

Run games at the panel's native resolution (for example 3840×2160), fullscreen or borderless, or the lenticular pattern will not line up.

## Using the installer

1. **Games** tab → **Add game** → pick the game's exe.
2. **Kind**: geo-11 (Unity fix / generic) or Unreal Engine 4 Universal Fix 2. Detected from the exe's folder layout; change it if wrong.
3. **Install**: bits, Unity version, existing fix, full install or "SR weaving only".
4. **Display**: SR weaving on/off, eye swap, lens, upscaling; with SR weaving off, the 3D output mode (sbs / tab / interlaced …).
5. **Stereo values**: separation, convergence, auto convergence. **Finish** writes everything to the game folder.

Hotkeys (editable in the **Hotkeys** tab): Ctrl+F3/F4 separation, Ctrl+F5/F6 convergence, Ctrl+F7 save, Ctrl+T 3D on/off, NumPad 0 overlay; SRWeave: Ctrl+Alt+W weaving on/off, Ctrl+Alt+S swap eyes, Ctrl+Alt+L lens, Ctrl+Alt+T test pattern.

UE4 Universal Fix 2 games must start with `-dx11`. The installer writes it into the Steam launch options (Steam is shut down briefly and restarted) or into Heroic's per-game settings (Epic/GOG). With the Epic Games Launcher add `-dx11` to the launch arguments yourself. The **UE4 settings** window replaces the fix's console config tool (AA/AO improvements, forced fullscreen, VSync, HUD profile).

Problems? Look at `SRWeave.log` in the game folder.

## Repository layout

| Folder | Contents |
|---|---|
| [`Installer/`](Installer/) | GUI installer (C# WinForms, no console window, English / Korean). Source `src\*.cs`, build `src\build.cmd` (Roslyn csc). Korean manual: [`README_KR.md`](Installer/README_KR.md) |
| [`SRWeave/`](SRWeave/) | SRWeave `dxgi.dll` source (C++, MinHook, SR SDK). `build.ps1 -Arch all` → `bin\x64\dxgi.dll`, `bin\x86\dxgi.dll`. Notes: [`README.md`](SRWeave/README.md) (Korean) |

The fix packages are not in the repository (third-party work, size); they are release assets only.

## How SRWeave works

1. On the first `CreateDXGIFactory*` call it hooks the real factory's `CreateSwapChain` / `CreateSwapChainForHwnd` with MinHook.
2. When the game creates its swap chain it hooks the real `Present` / `Present1` / `ResizeBuffers`. After geo-11 has drawn the SBS frame, the weaver runs inside Present.
3. The SR DLLs are delay-loaded: without the SR runtime only weaving is disabled. sRGB back buffers (typeless copy) and 32-bit games (different dxgi ordinals, `dxgi32.def`) are handled.
4. Hotkeys and options live in `SRWeave.ini`; the log is `SRWeave.log` next to it.

## Building

- Installer: `Installer\src\build.cmd` (Roslyn csc from Visual Studio, else the .NET Framework 4 csc). No external libraries.
- SRWeave: `SRWeave\build.ps1 -Arch all` (CMake + MSVC). The SR SDK is not included: take it from an SR Platform installation (or the Leia/SR SDK) and point the `SR_SDK` option in `CMakeLists.txt` at it.

## Verified (2026-10-09)

- Unreal Engine 4 (Steel Rats, SPRAWL): gameplay, stereo value save key.
- Unity (art of rally): weaving, including an sRGB back buffer.
- x64 and x86: hook → SR context → weaver → woven frame in a D3D11 test program.

## Credits and license

- **geo-11** — the geo-11 team (DarkStarSword, bo3b, masterotaku and others; 3DMigoto lineage), 3D Vision community / [HelixMod](https://helixmod.blogspot.com/).
- **Unreal Engine 4 Universal Fix 2** — assembled by LOSTI (helixmod community). This project only adds the SRWeave files, a newer geo-11 and a few ini lines (save key, `-dx11`); the original config tool stays in the package.
- **SR SDK / SR Platform** — Leia Inc. / Acer SpatialLabs (not included; loaded from the installed runtime).
- **MinHook** — Tsuda Kageyu, BSD 2-Clause (`SRWeave/third_party/minhook`).

The code in this repository (installer, SRWeave) is MIT licensed, see `LICENSE`. The packages in the releases belong to their authors and are redistributed for the convenience of SR panel users; if you own one of them and want it removed, open an issue.

Community: [3D Vision Discord](https://discord.gg/zTnnMrA4GE) · [HelixMod](https://helixmod.blogspot.com/)
