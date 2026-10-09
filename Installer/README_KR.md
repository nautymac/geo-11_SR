# geo-11 SR 설치기 (Geo11SRInstaller.exe)

`Geo11SRInstaller.exe`를 더블클릭하면 창이 뜹니다 (콘솔 창 없음, .NET Framework 4.x WinForms 단일 실행 파일, 고해상도 DPI 대응).
게임 실행 파일을 고르면 나머지는 자동으로 감지되고, 탭에서 고른 대로 설정 파일이 바뀝니다.
ReShade·GameBridge는 필요 없습니다. SR 위빙은 SR SDK의 `IDX11Weaver1`을 직접 쓰는 `dxgi.dll`(SRWeave)이 맡습니다.

이 폴더 옆에 두 패키지가 있어야 합니다:

| 패키지 | 폴더 |
|---|---|
| Unity (Unity Universal Fix + geo-11) | `..\Unity\{x64,x32}` |
| 일반 geo-11 (Preferred) | `..\Preferred\{x64,x32}` |
| Unreal Engine 4 (Universal Fix 2, 64비트 전용) | `..\UE4\` |

### Unreal Engine 4 게임

`<프로젝트>\Binaries\Win64\*-Win64-Shipping.exe` 구조면 UE4로 자동 감지되고 32비트 선택은 꺼집니다. 전체 설치는 UF2 패키지(78 MB)를 그 폴더에 복사한 뒤 `0000_Start_GEO11.cmd`가 하던 일을 설치기가 직접 합니다: `ShaderFixes\Geo11`의 geo-11 드라이버를 루트와 `ShaderFixes\ResetFix`에 배치, 실행 파일 이름에 맞는 HUD 프로필(`AUTOHUDUICONFIGS`)을 `AutoDepthUIHUD.ini`로 복사, d3dx.ini의 `force_stereo=2`·`get_resolution_from`·`3dvision2sbs` include 정리, 그리고 공통 단계(direct_mode=sbs, 입체 값, 업스케일, 오버레이, SRWeave). 입체 조절 단축키는 UF2가 `d3dxdm.ini` 안에 두므로 거기에 씁니다. UF2에는 3D 켜기/끄기와 현재 값 저장 키가 없어서 설치기가 `d3dxdm.ini`에 `[KeyToggleStereo]`, `[KeySaveSettings]`, `[CommandListSaveSettings]`를 추가한 뒤 키를 씁니다(2026-10-09, SPRAWL에서 저장 키가 안 먹던 문제). UF2가 들고 있는 geo-11은 0.6.40이라 Unity 패키지가 쓰는 `persist separation = …` 문법을 모르고(로그에 Unrecognised entry), 명령 리스트에서 읽는 `separation` 값도 엉뚱합니다(기본값의 제곱). 그래서 설치기는 UF2 설치에 **geo-11 0.6.109**(Unity 패키지와 같은 드라이버; 패키지 `ShaderFixes\Geo11`, `ResetFix`에 넣어 둠, 0.6.40 원본은 `ShaderFixes\Geo11_0.6.40`)를 쓰고, 저장 리스트는 Unity와 같은 `pre persist separation = separation` / `pre persist convergence = convergence`(+ `overlay_notice`)입니다. 이미 0.6.40으로 설치된 게임은 단축키를 적용할 때 드라이버를 패키지의 0.6.109로 바꿉니다(d3d11.dll, d3d11_loader.dll, nvapi64.dll; 루트·Geo11·ResetFix). 0.6.109는 값이 바뀌면 바로 `d3dx_user.ini`에 `pre persist separation = …`로 씁니다. Steel Rats에서 0.6.109로 기동·위빙·저장 기록을 확인했고(2026-10-09), UF2 기능 전반의 호환은 플레이로 더 확인이 필요합니다. 3DMigoto ini는 값 뒤에 `;` 주석을 붙이면 그 줄 전체가 무시되므로 주석은 별도 줄에만 둡니다.
설치 단계의 UE4 항목: **AA/AO 개선 설치**(패키지의 `Engine_additions_AA.txt`·`Scalability_AA.ini`를 `%LOCALAPPDATA%\<프로젝트>\Saved\Config\WindowsNoEditor`의 `Engine.ini`·`Scalability.ini` 끝에 붙이고 원본은 `*_backup.ini`로 보관, 읽기 전용 설정; 게임을 아직 실행한 적이 없어 폴더가 없으면 건너뜀)와 **UE4 설정...** 버튼. 이 버튼은 Universal Fix 2의 명령창 설정 도구(`00_UE4-UniversalFix-2_Config.cmd`)가 하던 일을 콘솔 없이 창으로 받는다: AA/AO 개선 설치·제거와 AA 변형(게임 AA 설정 그대로 = 도구의 ENABLE AA / AA 강제 지정 = DISABLE AA, 반투명 물체 깨짐 대응), 강제 전체화면(`d3dx.ini` `full_screen`), 화면 표시 OSD(`y20`), VSync 모드(`Vertical Sync` NVIDIA 프로필 토큰), HUD 프로필(`ShaderFixes\AUTOHUDUICONFIGS\<이름>` → `AutoDepthUIHUD.ini`). 게임 목록 탭의 **UE4 설정...** 버튼으로도 연다. **-dx11 실행 인자**: UF2는 게임을 `-dx11`로 실행해야 하므로(UF2의 `0_Start3D.cmd`도 `start "" "<exe>" -dx11`), [완료] 때 실행기 쪽에 넣는다. Steam 게임은 `<Steam>\userdata\<계정>\config\localconfig.vdf`의 `apps/<appid>/LaunchOptions`에 `-dx11`을 더하는데, Steam이 켜져 있으면 파일을 다시 써 버리므로 확인창 뒤 Steam을 `-shutdown`으로 끝내고 편집한 다음 다시 띄운다(원본은 `localconfig.vdf.geo11sr.bak`). Epic/GOG(Heroic) 게임은 `%APPDATA%\heroic\GamesConfig\<appName>.json`의 `launcherArgs`에 넣는다(appName은 `legendaryConfig\legendary\installed.json` / `gog_store\installed.json`의 install_path로 찾음). 설치 단계 UE4 항목의 **[실행기에 -dx11 넣기]** 버튼으로 다시 할 수 있고, 게임 목록의 실행 열이 `Steam -dx11` / `Heroic -dx11` / `(-dx11 없음)`으로 상태를 보여 준다. 설치기의 [실행]은 Steam이 아니면 exe를 `-dx11`로 직접 띄운다. 설치기는 `z_ExeChoosenState`도 써 두어 UF2의 `0_Start3D.cmd`로도 실행할 수 있다. 조준 키 수렴 배율·셰이더 헌팅·자동 수렴 모드는 이 패키지(Win11판)에 해당 파일이 없어 원래 도구에서도 동작하지 않으며, 창 아래의 링크로 원래 도구를 그대로 열 수도 있다. 제거는 UF2의 `Uninstall.bat`.

## 미리 설치할 것

**SR 패널이 아니어도 됩니다.** SR 위빙을 끄면 일반 geo-11 설치기로 동작합니다(출력 모드 선택). SR 위빙은 geo-11 SBS 화면을 렌티큘러 패널용으로 엮는 단계일 뿐입니다.

| 항목 | 필요 여부 |
|---|---|
| **SR 패널 + SR 런타임** | **SR 위빙을 쓸 때만** 필요합니다. 설치기의 디스플레이 단계에서 "SR 위빙 사용"을 끄면 geo-11의 출력 모드(sbs / tab / 인터레이스 등)를 골라 3D TV·편광 모니터 등 다른 3D 디스플레이에서도 쓸 수 있고, 그때는 SR 런타임이 없어도 됩니다. Acer SpatialLabs 제품은 **SpatialLabs Experience Center**만 설치하면 됩니다. 64비트 런타임(`C:\Program Files\Acer\SpatialLabs\Platform\bin`), 32비트 런타임(`C:\Program Files (x86)\Simulated Reality\Platform\bin`), SR Service / Eye Tracker 서비스가 함께 설치됩니다. 다른 Leia SR 패널은 그 제품의 SR Platform 런타임을 설치하세요. |
| Visual C++ 재배포 패키지 | SRWeave·설치기에는 필요 없습니다(정적 링크). geo-11(3DMigoto 계열)이 VC++ 2015~2022 x64(32비트 게임이면 x86도)를 쓰는데, 게임들이 대부분 깔아 두므로 보통 이미 있습니다. |
| .NET Framework 4.x | 설치기용. Windows 10/11에 4.8이 기본 포함되어 있습니다. |
| GPU | 제한 없음. geo-11은 NVIDIA 3D Vision 없이 동작합니다(패키지의 nvapi64.dll은 대체본). |
| 빌드 도구(Visual Studio, CMake) | 소스를 직접 빌드할 때만 필요합니다. |

게임은 SR 패널의 원래 해상도(예: 3840×2160)로 전체화면 또는 테두리 없는 창에서 실행해야 무늬가 맞습니다.
UE4 Universal Fix 2 게임의 `-dx11` 실행 인자는 Steam(시작 옵션)과 Heroic(Epic/GOG) 사용자는 설치기가 넣어 주고, Epic Games Launcher로 직접 실행하면 런처의 실행 인자에 `-dx11`을 직접 넣어야 합니다.

## 상태 표시줄 링크

창 아래 오른쪽에 **HelixMod 블로그**(https://helixmod.blogspot.com/)와 **3D Vision Discord** 링크가 있습니다. 주소는 exe 옆 `settings.ini`의 `helixmod=`, `discord=`로 바꿀 수 있고(비우면 숨김), Discord는 만료 없는 초대 링크를 넣으세요. 기본 Discord 주소는 https://discord.gg/zTnnMrA4GE 입니다.

## 언어 / Language

한국어와 영어를 지원합니다. Windows 표시 언어가 한국어면 한국어, 아니면 영어로 시작하고, 창 오른쪽 위의 "English로 바꾸기 / Switch to 한국어"로 바로 바꿀 수 있습니다. 선택은 exe 옆 `settings.ini`(`lang=ko` 또는 `lang=en`)에 저장됩니다. 설정 파일에 쓰는 값(ini 키, 키 표기)은 언어와 무관합니다.
Korean and English. The Windows display language picks the start language (Korean → Korean, anything else → English); the link at the top right switches at any time, and the choice is stored in `settings.ini` next to the exe. Values written to the game's config files do not depend on the language. Source: `src\Lang.cs` (Korean → English table).

## 사용 흐름 (4단계)

1. **게임 목록** 탭에서 **[게임 추가 (설정 시작)]** → 게임 실행 파일 선택. 목록에 "재설정 필요"로 올라가고 1단계로 넘어갑니다.
2. **1단계 종류**: geo-11(Unity 픽스 / 일반 Preferred)과 Unreal Engine 4(Universal Fix 2) 중 하나. 실행 파일 구조로 자동 선택되며 바꿀 수 있습니다. 두 그룹은 패키지·단축키 파일·추가 항목이 달라 별도 그룹으로 나뉩니다 → [다음]
3. **2단계 설치** (비트·Unity 버전·기존 픽스 자동 감지, 설치 범위; UE4 그룹은 엔진/Unity 버전 행이 숨고 UE4 추가 설정이 보임) → [다음]
4. **3단계 디스플레이** (SR 위빙, 3D 출력 모드, 업스케일) → [다음]
5. **4단계 입체 값** (Separation, Convergence, 자동 수렴) → **[완료]** 가 게임 폴더에 모두 적용하고 목록을 "설정 완료"로 바꿉니다.

각 단계 아래에 [취소] [이전] [다음/완료]가 있습니다. 중간에 [취소]하면 게임 폴더는 바뀌지 않고 목록에 "재설정 필요"로 남습니다. [완료] 전에는 아무것도 적용되지 않으므로 [설정 바꾸기]로 1단계부터 다시 합니다. 단축키는 [완료] 때 함께 저장되고, 이미 설정한 게임의 키만 바꾸려면 [설정 바꾸기] 뒤 단축키 탭의 [현재 게임에 단축키만 적용]을 누릅니다.

## 탭 구성

| 탭 | 내용 |
|---|---|
| **게임 목록** | 게임을 아이콘·이름과 함께 보여 줍니다. 상태(설정 완료 / 재설정 필요 / 파일 없음), 엔진, 비트, SR 여부, 업스케일(켬이면 출력 해상도 또는 바탕화면), Separation, Convergence는 게임 폴더의 설정 파일에서 그때그때 읽습니다. Sep/Conv는 게임 안에서 Ctrl+F7로 저장한 값(`d3dx_user.ini`)이 있으면 그 값이 우선입니다(geo-11도 그 값을 씁니다). 버튼: 게임 추가(설정 시작), 설정 바꾸기, 실행(Steam 게임은 `steam://rungameid/<appid>`, 아니면 exe), 폴더 열기, 새로 고침, 목록에서 빼기, SR 위빙만 제거, 전체 제거(게임 폴더의 Uninstall.bat), SRWeave.log. 두 번 클릭하면 실행. 목록은 exe 옆 `games.list`(경로 + 완료 여부)에 저장됩니다 |
| **종류** (1단계) | 픽스 종류 선택: geo-11(Unity / 일반) 또는 Unreal Engine 4 Universal Fix 2. 자동 감지 결과와 각 그룹의 설명을 보여 줍니다 |
| **설치** (2단계) | [게임 추가]에서 고른 게임(이름·경로)을 보여 주고 32/64비트, Unity 여부·버전, 기존 d3d11.dll(픽스), 다른 dxgi.dll(ReShade)을 자동 확인. 엔진(Unity ↔ 일반 geo-11), Unity 버전(Unity일 때만 활성, 기본 2019 이상, 감지되면 자동 선택), 비트, 설치 범위(전체 ↔ SR 위빙만 추가) |
| **디스플레이** | SR 위빙 사용 여부, 좌우 눈 바꾸기, 렌즈 켜기. SR을 끄면 3D 출력 모드(direct_mode: sbs/tab/interlaced/…) 선택 가능, 켜면 sbs 고정. **업스케일**: 게임은 낮은 해상도로 그리고 geo-11이 패널 해상도로 키워 출력(4K SR 패널에서 프레임이 모자랄 때). 키우는 방식은 "호환성 우선"(기본, `upscale_mode=1`: 패널 해상도 출력 버퍼를 따로 만듦)과 "성능 우선"(`upscale_mode=0`: 게임 출력 버퍼에 바로 그림, 안 되는 게임 많음) 중 선택. 전체화면 처리와 출력 해상도는 "고급 설정 보기"에 있으며 기본값(게임이 전체화면 전환 가능 / 바탕화면)이면 됩니다. 고치는 항목: `upscaling`, `upscale_mode`, `width`/`height`, `ShaderFixes\upscale.ini` include |
| **입체 값** | Separation(0-100, 소수 1자리), Convergence(0.01 이상, 픽스에 따라 100 이상도 사용), 자동 수렴 |
| **단축키** | 입력란을 클릭하고 실제 키를 누르면 "Ctrl + F6"처럼 표시되고, 저장할 때 3DMigoto/SRWeave 표기로 자동 변환됩니다. geo-11 입체 조절, 오버레이(3DMigoto Hunting) 모드와 표시/숨김 키, SRWeave 키(항상 Ctrl+Alt+키). [지움]은 비움, "단축키 기본값으로" 버튼 |
| **안내** | 사용 순서, 기본 단축키, 고치는 파일, 실행 조건, 제거 방법 |
| **로그** | 감지 결과와 적용 내역 |

이미 설정한 게임을 [설정 바꾸기]로 불러오면 지금 값(direct_mode, 업스케일, 입체 값, 단축키, SRWeave 설정)을 읽어 와서 보여 주므로, 값만 바꿔 [완료]하면 됩니다.

## 설치 / 적용이 고치는 파일

| 파일 | 항목 |
|---|---|
| `d3dxdm.ini` | `direct_mode`, `dm_separation`, `dm_convergence`, `dm_auto_convergence` |
| `d3dx_user.ini` | 게임 안에서 Ctrl+F7로 저장한 `persist separation` / `persist convergence`가 있으면 같은 값으로 바꿈 (geo-11은 시작할 때 이 파일을 d3dxdm.ini보다 나중에 읽어 우선 적용하므로, 안 바꾸면 설치기 값이 무시됨) |
| `ShaderFixesDM\hotkeys.ini` | `[KeyToggleStereo]` 등 각 섹션의 `Key =` (예: `ctrl VK_F6`) |
| `d3dx.ini` | `hunting`(오버레이 모드), `toggle_hunting`(오버레이 표시/숨김 키), `reload_config`, `reload_fixes`, `show_original`; 업스케일 `upscaling`, `upscale_mode`, `width`, `height`(자동이면 주석 처리)와 `[Include] include = ShaderFixes\upscale.ini`; Unity include 줄은 `UnitySwitch.ps1` 호출(숨김 창) |
| `SRWeave.ini` | `weave`, `swap_eyes`, `lens`, `key_weave`, `key_swap`, `key_lens`, `key_test` |
| `dxgi.dll` | SR 사용 시 복사. 다른 dxgi.dll(ReShade)이 있으면 `dxgi.dll.reshade_bak`으로 백업. SR을 끄면 SRWeave dxgi.dll을 지우고 백업을 되돌림 |

`ShaderCache`, `ShaderCacheDM`은 전체 설치 때만 비웁니다(SR만 추가·값 변경 때는 그대로 두어 다음 실행이 느려지지 않게).

**패키지 복사와 타임스탬프**: 패키지 파일은 원본의 수정 시각을 그대로 복사하고, 복사 뒤 `ShaderFixes`의 `*.bin`(3DMigoto가 컴파일해 둔 셰이더 캐시)이 짝이 되는 `.hlsl`/`.txt`보다 오래되지 않도록 시각을 보정합니다. geo-11은 .bin이 원본보다 오래되면 시작할 때 다시 컴파일하는데, UF2의 오버레이 지오메트리 셰이더(`overlay_hud*.hlsl`)는 이 컴파일에 수십 분이 걸려 게임 창이 뜨지 않는 것처럼 보입니다(2026-10-09 Steel Rats에서 확인). 같은 이유로 패키지 폴더를 손으로 복사할 때도 시각이 보존되는 방법(robocopy, 탐색기 복사)을 쓰세요.

## 기본 단축키

| 구분 | 키 | 동작 |
|---|---|---|
| geo-11 | Ctrl+T | 3D 켜기/끄기 |
| geo-11 | Ctrl+F3 / Ctrl+F4 | Separation 감소 / 증가 |
| geo-11 | Ctrl+F5 / Ctrl+F6 | Convergence 감소 / 증가 |
| geo-11 | Ctrl+F7 | 현재 값 저장 (d3dxdm.ini에 persist) |
| geo-11 | Ctrl+F1 | 입체 값 오버레이 표시 |
| 오버레이 | 숫자패드 0 | 오버레이(3DMigoto Hunting: 셰이더 정보·조작 안내) 표시/숨김 — 오버레이 모드가 1 또는 2일 때 |
| 오버레이 | F10 | 설정 / 픽스 다시 읽기 |
| 오버레이 | F9 (누르는 동안) | 픽스 잠시 끄기 |
| SRWeave | Ctrl+Alt+W | 위빙 켜기/끄기 |
| SRWeave | Ctrl+Alt+S | 좌우 눈 바꾸기 |
| SRWeave | Ctrl+Alt+L | 렌티큘러 렌즈 켜기/끄기 |
| SRWeave | Ctrl+Alt+T | 테스트 무늬 (왼눈 빨강 / 오른눈 파랑) |

변환 규칙(참고): geo-11/3DMigoto는 `ctrl`/`alt`/`shift` 또는 `no_modifiers` 뒤에 키 이름(글자·숫자는 그대로, 나머지는 `VK_F6`, `VK_NUMPAD0` 같은 VK_ 이름). SRWeave는 Ctrl+Alt 고정이라 키만 저장(글자·숫자, `F1`-`F24`, `NUMPAD0`-`9`, `VK_SPACE` 같은 VK_ 이름, 그 외 `0x` 16진수, `none`).

## 빌드

소스는 `src\Geo11SRInstaller.cs`(화면·동작), `src\Ue4.cs`(UE4 설정 창: UF2 설정 도구의 파일 작업을 창으로 대체), `src\Lang.cs`(한국어 → 영어 문자열 표)입니다. 화면 글자는 코드에 한국어로 쓰고 `Lang.T()`를 거치므로, 새 글자를 추가하면 `Lang.cs`에 영어 짝을 함께 넣습니다. `src\build.cmd`를 실행하면 Visual Studio의 Roslyn csc(없으면 .NET Framework 4의 csc)로 `Geo11SRInstaller.exe`를 만듭니다(`src\app.manifest`: DPI 인식, 공용 컨트롤 v6; `src\app.ico`: exe·창 아이콘, 256~16px. 원본은 `src\app_source.png`(남색 바탕에 "geo-11" + 입체 어긋남 "3D" 글리프, `src\icon_variants.py`로 그림), 미리보기 `src\app_256.png`). 외부 라이브러리 없음.
`src\legacy_powershell\`에는 exe 이전의 PowerShell 판이 참고용으로 남아 있습니다.

## 참고

- 32비트 게임은 `x32`의 32비트 SRWeave `dxgi.dll`(SR 런타임 32비트 DLL `SimulatedRealityCore32.dll` 등 사용)을 씁니다. SR Platform/SpatialLabs 런타임이 설치돼 있으면 32비트 DLL도 `C:\Program Files (x86)\...\Platform\bin`에 같이 들어 있습니다.
- 게임은 SR 패널에서 패널 원래 해상도로 전체화면 또는 테두리 없는 창으로 실행합니다. 백버퍼와 창 크기가 다르면 `SRWeave.log`에 WARNING이 남고 무늬가 어긋납니다.
- SR 런타임이 없거나 SR Service가 꺼져 있으면 위빙만 꺼지고 게임은 SBS로 정상 실행됩니다.
- 명령줄 방식이 필요하면 각 패키지의 `SR_Install.bat`(게임 exe를 끌어다 놓기) / `SR_Remove.bat`을 쓰면 됩니다.
