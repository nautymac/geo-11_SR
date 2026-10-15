# 3D compatibility list

Built automatically from user reports, grouped by engine and then by game. Report a game with the installer's **[Report 3D...]** button or the [report form](https://github.com/nautymac/geo-11_SR/issues/new?template=compat_report.yml).

✅ works well · ⚠️ works with issues · ❌ does not work. The status is the majority of the reports; a tie goes to the newest report.

9 games · 9 reports · 한국어: [COMPATIBILITY.ko.md](COMPATIBILITY.ko.md)

## Unity (Universal Fix)

| Game | Status | Reports | Bits | Latest note | Issues |
|---|---|---|---|---|---|
| art of rally | ✅ Works well | ✅ 1 | 64 | Unity Universal Fix; weaving and brightness correct with its sRGB back buffer in exclusive fullscreen. | [#12](https://github.com/nautymac/geo-11_SR/issues/12) |

## Unreal Engine 4 (Universal Fix 2)

| Game | Status | Reports | Bits | Latest note | Issues |
|---|---|---|---|---|---|
| SPRAWL | ✅ Works well | ✅ 1 | 64 | Universal Fix 2 with geo-11 0.6.109; separation/convergence save key (Ctrl+F7) works. Starts with -dx11. | [#10](https://github.com/nautymac/geo-11_SR/issues/10) |
| Steel Rats | ✅ Works well | ✅ 1 | 64 | Universal Fix 2 with geo-11 0.6.109; starts with -dx11 (the installer sets the Steam launch option). | [#9](https://github.com/nautymac/geo-11_SR/issues/9) |

## DirectX 9 (dgVoodoo2 + geo-11)

_No reports yet._

## OpenGL (OGL3D)

| Game | Status | Reports | Bits | Latest note | Issues |
|---|---|---|---|---|---|
| Amnesia: The Dark Descent | ✅ Works well | ✅ 1 | 32 | Upscale on for readable UI at 4K, auto convergence. Quit from the in-game menu. | [#4](https://github.com/nautymac/geo-11_SR/issues/4) |
| Broken Age | ✅ Works well | ✅ 1 | 32 | Needs convergence = auto (depth scale changes per shot). No flicker with OGL3D 1.0.0. | [#5](https://github.com/nautymac/geo-11_SR/issues/5) |
| Curse of the Dead Gods | ✅ Works well | ✅ 1 | 64 | Depth and HUD depth good, no shader breakage. Manual convergence. | [#3](https://github.com/nautymac/geo-11_SR/issues/3) |
| Darkest Dungeon® | ✅ Works well | ✅ 1 | 64 | Intro cinematic is flat; in-game layers have real depth. Auto convergence. | [#6](https://github.com/nautymac/geo-11_SR/issues/6) |

## DirectX 11 (generic geo-11)

| Game | Status | Reports | Bits | Latest note | Issues |
|---|---|---|---|---|---|
| Mega Man 11 | ✅ Works well | ✅ 1 | 64 | Generic geo-11 (Preferred) + SR weaving. Normal brightness (UNORM back buffer). | [#11](https://github.com/nautymac/geo-11_SR/issues/11) |
| Red Faction Guerrilla Re-Mars-tered | ✅ Works well | ✅ 1 | 32 | 32-bit; correct brightness with its sRGB back buffer since v0.1.1. | [#13](https://github.com/nautymac/geo-11_SR/issues/13) |
