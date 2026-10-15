# 3D 호환 목록

사용자 보고로 자동으로 만들어지는 목록입니다. 엔진별, 게임별로 정리합니다. 보고는 설치기의 **[3D 동작 보고...]** 버튼이나 [보고 양식](https://github.com/nautymac/geo-11_SR/issues/new?template=compat_report.yml)으로 합니다.

✅ 잘 됨 · ⚠️ 문제 있음 · ❌ 안 됨. 상태는 보고의 다수결이고, 같으면 최근 보고를 따릅니다.

게임 10개 · 보고 10건 · English: [COMPATIBILITY.md](COMPATIBILITY.md)

## Unity (Universal Fix)

| 게임 | 상태 | 보고 | 비트 | 최근 메모 | 이슈 |
|---|---|---|---|---|---|
| art of rally | ✅ 잘 됨 | ✅ 1 | 64 | Unity Universal Fix; weaving and brightness correct with its sRGB back buffer in exclusive fullscreen. | [#12](https://github.com/nautymac/geo-11_SR/issues/12) |

## Unreal Engine 4 (Universal Fix 2)

| 게임 | 상태 | 보고 | 비트 | 최근 메모 | 이슈 |
|---|---|---|---|---|---|
| SPRAWL | ✅ 잘 됨 | ✅ 1 | 64 | Universal Fix 2 with geo-11 0.6.109; separation/convergence save key (Ctrl+F7) works. Starts with -dx11. | [#10](https://github.com/nautymac/geo-11_SR/issues/10) |
| Steel Rats | ✅ 잘 됨 | ✅ 1 | 64 | Universal Fix 2 with geo-11 0.6.109; starts with -dx11 (the installer sets the Steam launch option). | [#9](https://github.com/nautymac/geo-11_SR/issues/9) |

## DirectX 9 (dgVoodoo2 + geo-11)

| 게임 | 상태 | 보고 | 비트 | 최근 메모 | 이슈 |
|---|---|---|---|---|---|
| Sine Mora EX | ✅ 잘 됨 | ✅ 1 | 32 | Needs VRAM 4GB and the 4GB patch (else crashes). No NumPad 0 overlay under dgVoodoo; hotkeys work. | [#14](https://github.com/nautymac/geo-11_SR/issues/14) |

## OpenGL (OGL3D)

| 게임 | 상태 | 보고 | 비트 | 최근 메모 | 이슈 |
|---|---|---|---|---|---|
| Amnesia: The Dark Descent | ✅ 잘 됨 | ✅ 1 | 32 | Upscale on for readable UI at 4K, auto convergence. Quit from the in-game menu. | [#4](https://github.com/nautymac/geo-11_SR/issues/4) |
| Broken Age | ✅ 잘 됨 | ✅ 1 | 32 | Needs convergence = auto (depth scale changes per shot). No flicker with OGL3D 1.0.0. | [#5](https://github.com/nautymac/geo-11_SR/issues/5) |
| Curse of the Dead Gods | ✅ 잘 됨 | ✅ 1 | 64 | Depth and HUD depth good, no shader breakage. Manual convergence. | [#3](https://github.com/nautymac/geo-11_SR/issues/3) |
| Darkest Dungeon® | ✅ 잘 됨 | ✅ 1 | 64 | Intro cinematic is flat; in-game layers have real depth. Auto convergence. | [#6](https://github.com/nautymac/geo-11_SR/issues/6) |

## DirectX 11 (일반 geo-11)

| 게임 | 상태 | 보고 | 비트 | 최근 메모 | 이슈 |
|---|---|---|---|---|---|
| Mega Man 11 | ✅ 잘 됨 | ✅ 1 | 64 | Generic geo-11 (Preferred) + SR weaving. Normal brightness (UNORM back buffer). | [#11](https://github.com/nautymac/geo-11_SR/issues/11) |
| Red Faction Guerrilla Re-Mars-tered | ✅ 잘 됨 | ✅ 1 | 32 | 32-bit; correct brightness with its sRGB back buffer since v0.1.1. | [#13](https://github.com/nautymac/geo-11_SR/issues/13) |
