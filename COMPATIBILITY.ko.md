# 3D 호환 목록

사용자 보고로 자동으로 만들어지는 목록입니다. 엔진별, 게임별로 정리합니다. 보고는 설치기의 **[3D 동작 보고...]** 버튼이나 [보고 양식](https://github.com/nautymac/geo-11_SR/issues/new?template=compat_report.yml)으로 합니다.

✅ 잘 됨 · ⚠️ 문제 있음 · ❌ 안 됨. 상태는 보고의 다수결이고, 같으면 최근 보고를 따릅니다.

게임 4개 · 보고 4건 · English: [COMPATIBILITY.md](COMPATIBILITY.md)

## Unity (Universal Fix)

_아직 보고가 없습니다._

## Unreal Engine 4 (Universal Fix 2)

_아직 보고가 없습니다._

## DirectX 9 (dgVoodoo2 + geo-11)

_아직 보고가 없습니다._

## OpenGL (OGL3D)

| 게임 | 상태 | 보고 | 비트 | 최근 메모 | 이슈 |
|---|---|---|---|---|---|
| Amnesia: The Dark Descent | ✅ 잘 됨 | ✅ 1 | 32 | Upscale on for readable UI at 4K, auto convergence. Quit from the in-game menu. | [#4](https://github.com/nautymac/geo-11_SR/issues/4) |
| Broken Age | ✅ 잘 됨 | ✅ 1 | 32 | Needs convergence = auto (depth scale changes per shot). No flicker with OGL3D 1.0.0. | [#5](https://github.com/nautymac/geo-11_SR/issues/5) |
| Curse of the Dead Gods | ✅ 잘 됨 | ✅ 1 | 64 | Depth and HUD depth good, no shader breakage. Manual convergence. | [#3](https://github.com/nautymac/geo-11_SR/issues/3) |
| Darkest Dungeon® | ✅ 잘 됨 | ✅ 1 | 64 | Intro cinematic is flat; in-game layers have real depth. Auto convergence. | [#6](https://github.com/nautymac/geo-11_SR/issues/6) |

## DirectX 11 (일반 geo-11)

_아직 보고가 없습니다._
