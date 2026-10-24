# SRWeave — a dxgi.dll that weaves geo-11 SBS for SR panels

**English** · [한국어](README.ko.md)

This is the source of the `dxgi.dll` (in the root and in `ShaderFixes\SRWeave\`) that ships in the UE4 Universal Fix 2 SR edition
and in the `Unity` and `Preferred` packages (x64/x32) based on the GameBridge release. It has no ReShade code and uses only the SR SDK's `IDX11Weaver1`.
It sits as `dxgi.dll` next to geo-11 (`d3d11.dll`) in the game folder and forwards all 20 exports of the System32 (x64) / SysWOW64 (x86) `dxgi.dll`
with the same ordinals (5 in C++, the other 15 as jump stubs: `src\dxgi_forward.asm` for x64, naked functions in `src\dxgi_forward_x86.cpp` for x86).
In the 32-bit dxgi, the ordinals of `CreateDXGIFactory1/2/` (no suffix) are 10/11/12, which differ from 64-bit, so there is a separate `src\dxgi32.def`.

Hotkeys can be changed with `key_weave`/`key_swap`/`key_lens`/`key_test` in `SRWeave.ini` (Ctrl+Alt is fixed; letters/digits, F1-F24, NUMPAD0-9,
VK_ names, 0x hex, none). An invalid value is logged and the default is used.

## Diagnostic options

`diag_exceptions=1` in `SRWeave.ini` uses a vectored exception handler to catch every C++ exception (0xE06D7363) and every fatal exception (access violation, etc.) in the process when it is first thrown. It logs to `SRWeave.log` the throwing module, the exception type (the RTTI name from ThrowInfo), `what()` and a module-level call stack. The same type at the same place is logged in full only twice; after that it is only counted (3DMigoto routinely throws `out_of_range` from `std::map::at`). This is how the Sine Mora EX crash was found to be a `std::bad_alloc` in the game's CRT (2 GB address space exhausted). `diag_input=1` logs the input messages that the game window receives.

## How it works

1. On the first `CreateDXGIFactory*` call, it creates a factory with the System32 dxgi (no window needed) and hooks the real
   `IDXGIFactory::CreateSwapChain` and `IDXGIFactory2::CreateSwapChainForHwnd` with MinHook.
2. When the game (through geo-11) creates a swap chain, it gets the Present address from the vtable of the object that the real dxgi returned.
   In case that object is a wrapper, there is also a fallback path: among the object's fields, it picks as the real swap chain an object
   whose vtable has at least 12 of its 18 slots pointing into System32 `dxgi.dll` and that answers `QueryInterface(IDXGISwapChain)`.
3. It inline-hooks the real `IDXGISwapChain::Present`/`ResizeBuffers` and `IDXGISwapChain1::Present1` with MinHook.
   When geo-11 calls the real Present after it has finished the SBS frame, the weaving happens inside that call.
4. Weaving: back buffer → copy texture → `setInputViewTexture(srv, width/2, height, format)` → bind the back buffer RTV → `weave()` →
   restore the pipeline state → original Present. D3D calls go only to the real device, so geo-11 does not turn the weaver's drawing into stereo.

The SR initialization order is the same as in SRCapture3D `sr_direct.cpp`. The SR runtime DLLs are delay-loaded and looked up in the SpatialLabs/SR Platform folders.
Without the runtime or the SR Service, only weaving is off and the game runs as usual.

It was forked from `tridef-injector\srweave` (SRWeaveDX11 for TriDef). Differences: no temporary device, swap chain or window (see below),
state reset for a swap chain whose device changed, per-buffer RTVs for the flip model, a warning when the back buffer and window sizes do not match, all dxgi exports.

### sRGB back buffer (2026-10-08, art of rally)

When a Unity game (art of rally) created an `R8G8B8A8_UNORM_SRGB` (29) back buffer in exclusive fullscreen, every frame failed with `CreateShaderResourceView failed 0x80070057`
and there was no weaving. The reason: the copy texture was created with the same typed sRGB format as the back buffer, but the view was created as UNORM (a typed texture only allows views of its own format).
Now the copy texture is created as the TYPELESS format of that format family (`TypelessOf`), which allows both UNORM and UNORM_SRGB views, and the back buffer RTV is created with the back buffer's exact
format. If creating a view fails, the format is logged and that swap chain is skipped (no repeated log every frame); it is retried on ResizeBuffers.
Reproduced and verified with srtest and its `srgb` argument (DISCARD + UNORM_SRGB back buffer).

### sRGB back buffer brightness (2026-10-11, Red Faction Guerrilla Re-MARS-tered)

The art of rally fix above only fixed the view creation failure; it did not get the gamma right. On an sRGB back buffer (29) with `srgb=0` (the default at the time), the input SRV
was UNORM (no decode) and the back buffer RTV was UNORM_SRGB (encode on write), so **the gamma was encoded twice** and the picture looked bright and washed out
(RFG Re-MARS-tered, log `view fmt 28 (back buffer fmt 29)`). The other way round, `srgb=1` on a UNORM back buffer made it dark.
Now `srgb=auto` (default) matches the SRV to the back buffer format (`IsSrgbFormat`), and the back buffer RTV is created with the same format as the SRV (`vf`).
If an RTV in the sibling format is rejected, it is created with the back buffer's exact format and a WARNING is logged. `srgb=0/1` only decides whether weaving happens
in gamma space or in linear space; input and output always match, so the brightness does not change.
Verification: a D3D11 test program (flat gray, the stored value 128 read back from the screen). The DLL before the fix gave 188 (bright) on an sRGB back buffer;
after the fix it gives 128 for x64 and x86, UNORM and sRGB back buffers, and auto/0/1.

### Why it must not create a temporary swap chain (2026-10-03)

The first version, like srweave, created a temporary device + swap chain on a hidden window to get the Present address. geo-11 wraps that temporary swap chain too
and subclasses its window, and after that **keyboard, mouse and controller all stopped working in the game**. Key messages reached the game window's thread,
but the game did not react, and it was the same with `weave=0` (hooks only, no SR). So the cause is not SR but the temporary window.
After switching to factory hooking, the same test passed (↓ in the menu → the selection moves from CONTINUE to NEW GAME).
With `diag_input=1`, the input and activation messages received by the game window's thread and the Raw Input registrations are written to `SRWeave.log`.

## Building

```
powershell -ExecutionPolicy Bypass -File build.ps1            # x64 + x86
powershell -ExecutionPolicy Bypass -File build.ps1 -Arch x86  # one architecture only (x64 | x86)
```

You need the Visual Studio 2026 C++ tools (with MASM for the x64 build) and SR SDK 1.34.10. The default SDK paths are
`simulatedreality-1.34.10-win64-Release (extracted SDK)` (x64) and `simulatedreality-1.34.10-win32-Release (extracted SDK)` (x86) in the folder where the SR runtime installer (or the Leia/SR SDK)
was unpacked. If the SDK is elsewhere, change it with `build.ps1 -Arch x86 -DSR_SDK=<path>`.
The 32-bit SDK comes out when you unpack the runtime installer's `LeiaSR-Runtime-1.34.10-win64\temp\simulatedreality-1.34.10-win32-Release.exe` (NSIS) with 7-Zip
(headers identical to 64-bit, `lib\*32.lib`, `third_party\OpenCV\lib\x86`). The 32-bit runtime DLLs (`SimulatedRealityCore32.dll` etc.) are installed by SR Platform
in `C:\Program Files (x86)\Simulated Reality\Platform\bin`. The output is `bin\x64\dxgi.dll` and `bin\x86\dxgi.dll`. The CRT is static, so no redistributable is needed.
After the build, copy the dll and `SRWeave.ini` to the root and to `ShaderFixes\SRWeave\` of each SR edition folder (in its x64/x32).

Smoke test (without a game): build `srtest` from the scratchpad (a D3D11 window + swap chain, Present for N frames) for x86/x64, put only `dxgi.dll` + `SRWeave.ini`
next to it and run it. `SRWeave.log` should then show hook → SR context → DX11 weaver created → `first weaved frame` (passed on both architectures on 2026-10-08).

MinHook (BSD-2) is in `third_party\minhook`. The SR SDK and runtime belong to Leia/Acer, so they are not included.
