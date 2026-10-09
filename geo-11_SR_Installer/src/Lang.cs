// UI language: every user-visible string is written in Korean in the code and passed through Lang.T(),
// which returns the English text from the table below when English is active. Unknown strings fall back
// to the Korean original. The choice is kept in settings.ini next to the exe ("lang=ko" / "lang=en");
// without it the Windows display language decides (Korean -> Korean, anything else -> English).
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;

namespace Geo11SR
{
    static class Lang
    {
        public static bool English;
        static string settingsFile;
        // other "key=value" lines of settings.ini (helixmod=, discord=: the community links in the status bar)
        static readonly Dictionary<string, string> settings = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        public static void Load(string exeDir)
        {
            settingsFile = Path.Combine(exeDir, "settings.ini");
            English = CultureInfo.CurrentUICulture.TwoLetterISOLanguageName != "ko";
            settings.Clear();
            try
            {
                if (File.Exists(settingsFile))
                    foreach (var l in File.ReadAllLines(settingsFile, Encoding.UTF8))
                    {
                        var t = l.Trim();
                        int eq = t.IndexOf('=');
                        if (eq <= 0 || t.StartsWith(";") || t.StartsWith("#")) continue;
                        string k = t.Substring(0, eq).Trim(), v = t.Substring(eq + 1).Trim();
                        if (k.Equals("lang", StringComparison.OrdinalIgnoreCase)) English = v.ToLowerInvariant() != "ko";
                        else settings[k] = v;
                    }
            }
            catch { }
        }

        public static string Setting(string key, string fallback)
        {
            string v;
            return settings.TryGetValue(key, out v) && v.Length > 0 ? v : fallback;
        }

        public static void Save()
        {
            try
            {
                var sb = new StringBuilder();
                sb.AppendLine("lang=" + (English ? "en" : "ko"));
                sb.AppendLine("; links shown at the bottom right (leave empty to hide): helixmod=<url>, discord=<invite url>");
                foreach (var kv in settings) sb.AppendLine(kv.Key + "=" + kv.Value);
                File.WriteAllText(settingsFile, sb.ToString(), Encoding.UTF8);
            }
            catch { }
        }

        public static string T(string ko)
        {
            if (!English || ko == null) return ko;
            string v;
            return en.TryGetValue(ko, out v) ? v : ko;
        }

        static readonly Dictionary<string, string> en = new Dictionary<string, string> {
            // key names
            { "숫자패드 *", "NumPad *" }, { "숫자패드 +", "NumPad +" }, { "숫자패드 -", "NumPad -" }, { "숫자패드 .", "NumPad ." }, { "숫자패드 /", "NumPad /" }, { "숫자패드 ", "NumPad " },
            // Unity versions
            { "5.5 이하", "5.5 or older" }, { "2019 이상", "2019 or newer" },
            // hotkey labels
            { "3D 켜기 / 끄기", "3D on / off" },
            { "Separation(입체감) 증가", "Separation up" }, { "Separation(입체감) 감소", "Separation down" },
            { "Convergence(수렴) 증가", "Convergence up" }, { "Convergence(수렴) 감소", "Convergence down" },
            { "현재 값 저장", "Save current values" }, { "입체 값 오버레이 표시", "Show stereo values overlay" },
            { "오버레이 표시 / 숨김 (게임 중)", "Overlay show / hide (in game)" },
            { "설정(d3dx.ini) 다시 읽기", "Reload settings (d3dx.ini)" }, { "픽스(ShaderFixes) 다시 읽기", "Reload fixes (ShaderFixes)" },
            { "픽스 잠시 끄기 (누르는 동안)", "Disable fix while held" },
            { "위빙 켜기 / 끄기", "Weaving on / off" }, { "좌우 눈 바꾸기", "Swap eyes" }, { "렌티큘러 렌즈 켜기 / 끄기", "Lenticular lens on / off" }, { "테스트 무늬 (왼눈 빨강)", "Test pattern (left eye red)" },
            // tabs / wizard
            { "설치", "Install" }, { "디스플레이", "Display" }, { "입체 값", "Stereo values" }, { "geo-11 SR 설치기", "geo-11 SR Installer" },
            { "패키지 확인: ", "Package found: " }, { "패키지 없음: ", "Package missing: " },
            { "[게임 목록]에서 [게임 추가]를 눌러 설정을 시작하세요. 게임 실행 파일을 이 창에 끌어다 놓아도 됩니다.", "Press [Add game] in [Games] to start. You can also drop a game exe onto this window." },
            { "단계 ", "Step " }, { "  (완료를 누르면 게임 폴더에 적용합니다)", "  (Finish writes everything to the game folder)" },
            { "취소", "Cancel" }, { "이전", "Back" }, { "다음", "Next" }, { "완료", "Finish" },
            { "파일이 없습니다: ", "File not found: " },
            { "설정을 중단했습니다. ", "Setup cancelled. " },
            { "은(는) '재설정 필요'로 남아 있습니다. [설정 바꾸기]로 처음부터 다시 하면 됩니다.", " stays marked 'Needs setup'. Use [Change settings] to start again from step 1." },
            { "설정을 중단했습니다.", "Setup cancelled." }, { "(게임을 고르세요)", "(choose a game)" },
            { "  |  [게임 목록]의 [게임 추가] 또는 [설정 바꾸기]로 시작하면 단계대로 진행됩니다.", "  |  Start with [Add game] or [Change settings] in [Games] to go through the steps." },
            // game list
            { "게임 목록", "Games" },
            { "중간에 취소하면 게임 폴더는 바뀌지 않고 '재설정 필요'로 남습니다. Sep/Conv는 게임 안에서 저장(Ctrl+F7)한 값이 있으면 그 값입니다. 두 번 클릭하면 실행합니다.", "Cancelling leaves the game folder untouched and the entry marked 'Needs setup'. Sep/Conv show the values saved in game (Ctrl+F7) when present. Double-click to launch." },
            { "게임", "Game" }, { "상태", "Status" }, { "엔진", "Engine" }, { "비트", "-bit" }, { "업스케일", "Upscale" }, { "실행", "Launch" }, { "폴더", "Folder" },
            { "게임 추가 (설정 시작)", "Add game (start setup)" }, { "설정 바꾸기", "Change settings" }, { "폴더 열기", "Open folder" }, { "새로 고침", "Refresh" },
            { "목록에서 빼기", "Remove from list" }, { "SR 위빙만 제거", "Remove SR weaving only" }, { "전체 제거", "Uninstall all" },
            { "실행 파일이 없습니다: ", "Executable not found: " }, { "games.list 읽기 실패: ", "Could not read games.list: " }, { "games.list 쓰기 실패: ", "Could not write games.list: " },
            { " (없음)", " (missing)" }, { "일반", "Generic" }, { "켬", "On" }, { "끔", "Off" }, { "(geo-11 없음)", "(no geo-11)" },
            { "켬 → ", "On → " }, { "바탕화면", "desktop" }, { " (성능)", " (perf.)" },
            { "파일 없음", "File missing" }, { "설정 완료", "Configured" }, { "재설정 필요", "Needs setup" },
            { "게임 목록에서 게임을 먼저 고르세요.", "Select a game in the list first." },
            { "Steam으로 실행: ", "Launched via Steam: " }, { "실행: ", "Launched: " }, { "실행 실패: ", "Launch failed: " },
            { "실행 파일 (*.exe)|*.exe", "Executables (*.exe)|*.exe" }, { "설정할 게임의 실행 파일", "Executable of the game to configure" },
            { "목록에서 뺐습니다 (게임 폴더는 그대로입니다).", "Removed from the list (the game folder is untouched)." },
            // install step
            { "선택한 게임", "Selected game" },
            { "(아직 고르지 않았습니다 - [게임 목록] 탭의 [게임 추가]를 누르세요)", "(none yet - press [Add game] in the [Games] tab)" },
            { "실행 파일을 고르면 32/64비트, Unity 여부·버전, 기존 픽스(d3d11.dll), 다른 dxgi.dll(ReShade)을 자동으로 확인합니다.", "Picking the exe detects 32/64-bit, Unity and its version, an existing fix (d3d11.dll) and a foreign dxgi.dll (ReShade)." },
            { "패키지", "Package" }, { "엔진:", "Engine:" }, { "일반 geo-11 (Preferred)", "Generic geo-11 (Preferred)" }, { "Unity 버전:", "Unity version:" }, { "비트:", "Bits:" },
            { "64비트 (x64)", "64-bit (x64)" }, { "32비트 (x32)", "32-bit (x32)" },
            { "설치 범위", "Install scope" },
            { "전체 설치 (geo-11 + 픽스 + SR 위빙) - 처음 설치하는 게임", "Full install (geo-11 + fix + SR weaving) - first-time install for this game" },
            { "SR 위빙만 추가 (기존 픽스 유지) - 이미 geo-11/3DMigoto 픽스가 있는 게임", "Add SR weaving only (keep existing fix) - game that already has a geo-11/3DMigoto fix" },
            { "SR 위빙만 추가: dxgi.dll + SRWeave.ini만 넣고 d3dx.ini / ShaderFixes는 건드리지 않습니다. 게임 폴더에 d3d11.dll이 있으면 자동으로 이 쪽이 선택됩니다.", "SR weaving only: copies just dxgi.dll + SRWeave.ini and leaves d3dx.ini / ShaderFixes alone. Selected automatically when the game folder already has d3d11.dll." },
            // display step
            { "SR 위빙 (SRWeave dxgi.dll)", "SR weaving (SRWeave dxgi.dll)" },
            { "SR 위빙 사용 - 렌티큘러(무안경 3D) 패널용, ReShade 없이 SR SDK 위버로 직접 위빙", "Use SR weaving - for lenticular (glasses-free 3D) panels, weaves with the SR SDK directly, no ReShade" },
            { "좌우 눈 바꾸기 (입체가 뒤집혀 보일 때)", "Swap eyes (if the depth looks inside-out)" }, { "위빙 중 렌티큘러 렌즈 켜기", "Switch the lenticular lens on while weaving" },
            { "SR 런타임(SpatialLabs / SR Platform)이 설치돼 있고 SR Service가 실행 중이어야 합니다. 없으면 위빙만 꺼지고 게임은 SBS로 정상 실행됩니다.\n게임은 SR 패널에서 패널 원래 해상도로 전체화면 또는 테두리 없는 창으로 실행하세요.", "Needs the SR runtime (SpatialLabs / SR Platform) installed and SR Service running. Without it only weaving is off and the game runs normally in SBS.\nRun the game on the SR panel at its native resolution, fullscreen or borderless." },
            { "3D 출력 모드 (d3dxdm.ini direct_mode)", "3D output mode (d3dxdm.ini direct_mode)" },
            { "SR 위빙은 sbs(side-by-side) 입력이 필요하므로 sbs로 고정됩니다. SR 위빙을 끄면 다른 모드를 고를 수 있습니다.", "SR weaving needs side-by-side input, so this stays sbs. Turn SR weaving off to pick another mode." },
            { "업스케일 (성능이 모자랄 때)", "Upscaling (when the frame rate is too low)" }, { "업스케일 사용", "Use upscaling" },
            { "게임은 낮은 해상도(예: 1920×1080)로 그리고, geo-11이 패널 해상도로 키워서 보여 줍니다. 4K SR 패널에서 프레임이 모자랄 때 켜세요.\n", "The game renders at a lower resolution (e.g. 1920×1080) and geo-11 scales it up to the panel resolution. Turn it on when a 4K SR panel runs out of frames.\n" },
            { "켠 뒤에는 게임 안 그래픽 옵션에서 낮은 해상도를 고르면 됩니다. 끄면 게임이 고른 해상도 그대로 출력됩니다.", "Then pick a lower resolution in the game's graphics options. Off: the game's own resolution is shown as is." },
            { "키우는 방식:", "Scaling method:" },
            { "성능 우선 - 조금 더 부드럽지만 안 되는 게임이 많음", "Performance - a bit smoother, but fails in many games" },
            { "호환성 우선 - 대부분 게임에서 됨 (기본)", "Compatibility - works in most games (default)" },
            { "호환성 우선: geo-11이 패널 해상도의 출력 버퍼를 따로 만들어 거기에 키워 넣습니다. 성능 우선: 게임의 출력 버퍼에 바로 키워 그립니다(복사가 한 번 적음). ", "Compatibility: geo-11 creates its own panel-resolution output buffer and scales into it. Performance: scales straight into the game's output buffer (one copy less). " },
            { "성능 우선에서 화면이 검거나 깨지면 호환성 우선으로 돌리세요. SR 위빙은 둘 다 됩니다.", "If Performance gives a black or broken screen, go back to Compatibility. SR weaving works with both." },
            { "고급 설정 보기 (전체화면 처리, 출력 해상도)", "Show advanced settings (fullscreen handling, output resolution)" },
            { "전체화면 처리:", "Fullscreen handling:" },
            { "게임이 전체화면을 켜고 끌 수 있음 (기본)", "Game may switch fullscreen on and off (default)" }, { "항상 전체화면 고정 - 마우스 커서가 이상할 때", "Always force fullscreen - if the mouse cursor misbehaves" },
            { "출력 해상도 (0 = 바탕화면):", "Output resolution (0 = desktop):" },
            { "출력 해상도는 SR 패널 원래 해상도여야 무늬가 맞으므로 0(바탕화면)이 안전합니다. ", "The output must be the SR panel's native resolution for the pattern to line up, so 0 (desktop) is the safe choice. " },
            { "고치는 항목: d3dx.ini [Device] upscaling / upscale_mode / width / height, [Include] ShaderFixes\\upscale.ini.", "Writes: d3dx.ini [Device] upscaling / upscale_mode / width / height, [Include] ShaderFixes\\upscale.ini." },
            // stereo step
            { "기본 입체 값 (d3dxdm.ini [Stereo])", "Default stereo values (d3dxdm.ini [Stereo])" },
            { "Separation (입체감, 0 - 100):", "Separation (depth, 0 - 100):" }, { "Convergence (수렴):", "Convergence:" },
            { "dm_separation / dm_convergence. 게임 안에서 Separation·Convergence 증가/감소 키로 바꾸고 저장 키(기본 Ctrl+F7)로 저장할 수 있습니다. Convergence가 클수록 화면 밖으로 튀어나오는 느낌이 커집니다. 보통 1 - 8 정도이지만 픽스에 따라 100 이상을 쓰기도 합니다.", "dm_separation / dm_convergence. In game you can change them with the Separation / Convergence keys and save with the save key (Ctrl+F7 by default). Higher convergence pushes the image further out of the screen. Usually 1 - 8, some fixes use 100 or more." },
            { "자동 수렴 (dm_auto_convergence) - 장면에 따라 geo-11이 수렴을 자동으로 맞춤", "Auto convergence (dm_auto_convergence) - geo-11 adjusts convergence to the scene" },
            { "자동 수렴을 켜면 Convergence 증가/감소 키는 팝아웃 바이어스(dm_popout_bias)를 조절합니다.", "With auto convergence on, the convergence keys adjust the pop-out bias (dm_popout_bias) instead." },
            // hotkeys tab
            { "단축키", "Hotkeys" },
            { "바꾸려면 입력란을 클릭한 뒤 원하는 키(조합)를 누르세요. 저장할 때 설정 파일 표기로 자동 변환됩니다. [지움]은 키를 비웁니다.", "Click a box and press the key (combination) you want. It is converted to the config-file spelling when saved. [Clear] empties the key." },
            { "geo-11 입체 조절", "geo-11 stereo controls" }, { "오버레이 (3DMigoto Hunting)", "Overlay (3DMigoto Hunting)" },
            { "오버레이는 게임 화면 위에 셰이더 정보와 조작 안내를 띄우는 3DMigoto 기능(Hunting 모드)입니다. 아래 키로 게임 중에 표시/숨김을 바꿉니다.", "The overlay is 3DMigoto's Hunting mode: shader information and key hints drawn over the game. The key below shows / hides it in game." },
            { "오버레이 모드:", "Overlay mode:" },
            { "0 - 사용 안 함 (가장 빠름, 키로도 못 켬)", "0 - off (fastest, the key cannot enable it)" }, { "1 - 사용 (표시된 채 시작, 키로 숨김)", "1 - on (starts visible, key hides it)" }, { "2 - 사용 (숨긴 채 시작, 키로 표시)", "2 - on (starts hidden, key shows it)" },
            { "SR 위빙 (SRWeave) - 항상 Ctrl + Alt + 키", "SR weaving (SRWeave) - always Ctrl + Alt + key" },
            { "단축키 기본값으로", "Reset hotkeys to defaults" }, { "현재 게임에 단축키만 적용", "Apply hotkeys only to current game" },
            { "단축키는 [완료]로 설정을 적용할 때 함께 저장됩니다. 이미 설정한 게임의 키만 바꾸려면 [게임 목록]에서 게임을 고르고 [설정 바꾸기] 뒤 이 버튼을 누르세요.", "Hotkeys are saved together with [Finish]. To change only the keys of a game that is already set up, select it in [Games], press [Change settings], then this button." },
            { "지움", "Clear" },
            // help tab
            { "안내", "Help" },
            { "■ 사용 순서", "■ How to use" },
            { "  1. [게임 목록] 탭에서 [게임 추가 (설정 시작)]을 누르고 게임 실행 파일을 고릅니다. 목록에 '재설정 필요'로 올라가고 1단계로 넘어갑니다.", "  1. In [Games] press [Add game (start setup)] and pick the game exe. It appears as 'Needs setup' and step 1 opens." },
            { "     중간에 [취소]하면 게임 폴더는 바뀌지 않고 목록에 '재설정 필요'로 남습니다. [완료] 전에는 아무것도 적용되지 않으므로, [설정 바꾸기]로 1단계부터 다시 하면 됩니다.", "     [Cancel] leaves the game folder untouched and the entry marked 'Needs setup'. Nothing is written before [Finish], so just start again with [Change settings]." },
            { "■ 기본 단축키 ([단축키] 탭에서 입력란을 클릭하고 키를 눌러 바꿀 수 있습니다)", "■ Default hotkeys (change them in the [Hotkeys] tab by clicking a box and pressing a key)" },
            { "  geo-11      Ctrl+T 3D 켜기/끄기 | Ctrl+F3 / Ctrl+F4 Separation 감소/증가 | Ctrl+F5 / Ctrl+F6 Convergence 감소/증가", "  geo-11      Ctrl+T 3D on/off | Ctrl+F3 / Ctrl+F4 separation down/up | Ctrl+F5 / Ctrl+F6 convergence down/up" },
            { "              Ctrl+F7 현재 값 저장 | Ctrl+F1 입체 값 오버레이", "              Ctrl+F7 save current values | Ctrl+F1 stereo values overlay" },
            { "  오버레이    숫자패드 0 오버레이(3DMigoto Hunting: 셰이더 정보·조작 안내) 표시/숨김 (오버레이 모드가 1 또는 2일 때)", "  Overlay     NumPad 0 show/hide the overlay (3DMigoto Hunting: shader info and key hints) when overlay mode is 1 or 2" },
            { "              F10 설정/픽스 다시 읽기 | F9 (누르는 동안) 픽스 끄기", "              F10 reload settings/fixes | F9 (held) disable the fix" },
            { "  SRWeave     Ctrl+Alt+W 위빙 켜기/끄기 | Ctrl+Alt+S 좌우 눈 바꾸기 | Ctrl+Alt+L 렌즈 켜기/끄기 | Ctrl+Alt+T 테스트 무늬(왼눈 빨강/오른눈 파랑)", "  SRWeave     Ctrl+Alt+W weaving on/off | Ctrl+Alt+S swap eyes | Ctrl+Alt+L lens on/off | Ctrl+Alt+T test pattern (left red / right blue)" },
            { "■ 적용이 고치는 파일 (게임 폴더)", "■ Files written by Finish (game folder)" },
            { "  ShaderFixesDM\\hotkeys.ini  geo-11 단축키 (각 섹션의 Key =)", "  ShaderFixesDM\\hotkeys.ini  geo-11 hotkeys (Key = in each section)" },
            { "  d3dx.ini                 hunting, toggle_hunting, reload_config, reload_fixes, show_original / upscaling, upscale_mode, width, height, include upscale.ini / Unity include 줄 (UnitySwitch.ps1)", "  d3dx.ini                 hunting, toggle_hunting, reload_config, reload_fixes, show_original / upscaling, upscale_mode, width, height, include upscale.ini / Unity include line (UnitySwitch.ps1)" },
            { "  dxgi.dll                 SR 사용 시 SRWeave 복사. 다른 dxgi.dll(ReShade)은 dxgi.dll.reshade_bak으로 백업, SR을 끄면 되돌림", "  dxgi.dll                 SRWeave is copied when SR is on. A foreign dxgi.dll (ReShade) is backed up as dxgi.dll.reshade_bak and restored when SR is turned off" },
            { "■ 실행 조건", "■ Requirements" },
            { "  SR 런타임(SpatialLabs / SR Platform)이 설치돼 있고 SR Service가 실행 중이어야 위빙됩니다. 없으면 SBS 그대로 나오고 게임은 정상 실행됩니다.", "  Weaving needs the SR runtime (SpatialLabs / SR Platform) installed and SR Service running. Without it the game shows plain SBS and runs normally." },
            { "  게임은 SR 패널에서 패널 원래 해상도로 전체화면 또는 테두리 없는 창으로 실행하세요. 창 크기가 다르면 무늬가 어긋나고 SRWeave.log에 WARNING이 남습니다.", "  Run the game on the SR panel at its native resolution, fullscreen or borderless. A different window size misaligns the pattern and SRWeave.log shows a WARNING." },
            { "  32비트 게임은 x32의 32비트 SRWeave가 SR 런타임의 32비트 DLL(C:\\Program Files (x86)\\...\\Platform\\bin)을 씁니다.", "  32-bit games use the 32-bit SRWeave from x32, which loads the runtime's 32-bit DLLs (C:\\Program Files (x86)\\...\\Platform\\bin)." },
            { "■ 제거", "■ Removal" },
            { "  [SR 위빙만 제거]  dxgi.dll, SRWeave.ini/.log, ShaderFixes\\SRWeave 삭제, ReShade 백업 복원. geo-11은 그대로.", "  [Remove SR weaving only]  deletes dxgi.dll, SRWeave.ini/.log, ShaderFixes\\SRWeave and restores the ReShade backup. geo-11 stays." },
            { "  [전체 제거]       게임 폴더의 Uninstall.bat 실행 (geo-11, ShaderFixes, SRWeave 모두 삭제).", "  [Uninstall all]           runs Uninstall.bat in the game folder (removes geo-11, ShaderFixes and SRWeave)." },
            { "로그", "Log" },
            // detection / loading
            { "  d3dx_user.ini에 게임 안에서 저장한 입체 값이 있어 그 값을 보여 줍니다 (적용하면 그 파일도 함께 바꿉니다).", "  d3dx_user.ini holds stereo values saved in game; those are shown (Finish updates that file too)." },
            { "\" 은(는) 읽을 수 없는 키 표기라 기본값을 보여 줍니다.", "\" is a key spelling this tool cannot read; showing the default instead." },
            { "파일이 없습니다.", "File not found." }, { "비트 알 수 없음", "bitness unknown" }, { "버전을 읽지 못함 - 직접 고르세요", "version unreadable - choose it yourself" },
            { "감지: Unity ", "Detected: Unity " }, { "Unity 아님 (일반 geo-11)", "not Unity (generic geo-11)" },
            { "기존 d3d11.dll(geo-11/3DMigoto) 있음 → 'SR 위빙만 추가' 권장", "existing d3d11.dll (geo-11/3DMigoto) → 'SR weaving only' recommended" },
            { "SRWeave dxgi.dll 이미 설치됨", "SRWeave dxgi.dll already installed" },
            { "다른 dxgi.dll(ReShade?) 있음 → 설치 시 dxgi.dll.reshade_bak으로 백업", "foreign dxgi.dll (ReShade?) present → backed up as dxgi.dll.reshade_bak on install" },
            { "게임: ", "Game: " },
            { "먼저 [게임 목록] 탭에서 [게임 추가 (설정 시작)]을 눌러 게임 실행 파일을 고르세요.\n게임 실행 파일을 이 창에 끌어다 놓아도 됩니다.", "First press [Add game (start setup)] in the [Games] tab and pick the game exe.\nYou can also drop the exe onto this window." },
            // actions / log lines
            { "  기존 dxgi.dll 삭제 (dxgi.dll.reshade_bak이 이미 있음)", "  existing dxgi.dll deleted (dxgi.dll.reshade_bak already exists)" },
            { "  기존 dxgi.dll(ReShade?) → dxgi.dll.reshade_bak", "  existing dxgi.dll (ReShade?) → dxgi.dll.reshade_bak" },
            { "  SRWeave dxgi.dll 삭제", "  SRWeave dxgi.dll deleted" }, { " 삭제", " deleted" }, { "  ShaderFixes\\SRWeave 삭제", "  ShaderFixes\\SRWeave deleted" },
            { "  dxgi.dll.reshade_bak → dxgi.dll 복원 (ReShade)", "  dxgi.dll.reshade_bak → dxgi.dll restored (ReShade)" },
            { "패키지가 없습니다: ", "Package not found: " }, { "\n이 설치기 폴더 옆에 패키지 폴더가 있어야 합니다.", "\nThe package folders must sit next to this installer's folder." },
            { "적용: ", "Applying: " }, { "일반 geo-11", "generic geo-11" }, { "전체 설치", "full install" }, { "SR만 추가", "SR weaving only" }, { "사용", "on" }, { "사용 안 함", "off" },
            { "  원본: ", "  source: " }, { "  패키지 파일 복사 완료", "  package files copied" }, { "  dxgi.dll + SRWeave.ini 복사 (기존 픽스 유지)", "  dxgi.dll + SRWeave.ini copied (existing fix kept)" },
            { "  d3dx_user.ini: 게임 안에서 저장했던 separation/convergence도 같은 값으로 바꿈", "  d3dx_user.ini: in-game saved separation/convergence set to the same values" },
            { "  경고: d3dxdm.ini 없음 - geo-11이 아닌 3DMigoto 픽스입니다. SRWeave는 geo-11 sbs 출력이 필요합니다.", "  warning: no d3dxdm.ini - this is a 3DMigoto fix, not geo-11. SRWeave needs geo-11's sbs output." },
            { " 출력 ", " output " }, { "바탕화면 해상도", "desktop resolution" }, { ", include upscale.ini 켬", ", include upscale.ini on" },
            { "  d3dx.ini: upscaling=0, include upscale.ini 끔", "  d3dx.ini: upscaling=0, include upscale.ini off" },
            { "  UnitySwitch.ps1 없음 - Unity 버전 include 줄은 바꾸지 않았습니다.", "  UnitySwitch.ps1 missing - the Unity include line was not changed." },
            { " 비움", " cleared" }, { "완료.", "Done." }, { "설정 완료: ", "Configured: " },
            { "설정을 적용했습니다.\n\n게임을 SR 패널에서 패널 해상도로 전체화면 또는 테두리 없는 창으로 실행하세요.\n자세한 내용은 [로그] 탭에 있습니다.", "Settings applied.\n\nRun the game on the SR panel at the panel resolution, fullscreen or borderless.\nDetails are in the [Log] tab." },
            { "오류: ", "Error: " }, { "오류", "Error" },
            { "  d3dx.ini: hunting(오버레이 모드)=", "  d3dx.ini: hunting (overlay mode)=" },
            { "단축키만 적용: ", "Hotkeys only: " }, { "단축키를 적용했습니다: ", "Hotkeys applied: " },
            { "SR 위빙 제거", "Remove SR weaving" }, { "완료 (geo-11은 그대로입니다).", "Done (geo-11 is untouched)." }, { "SR 위빙 제거 완료", "SR weaving removed" },
            { "Uninstall.bat이 게임 폴더에 없습니다.", "Uninstall.bat is not in the game folder." }, { "Uninstall.bat 없음", "No Uninstall.bat" },
            { "게임 폴더의 Uninstall.bat을 실행해 geo-11, ShaderFixes, SRWeave를 모두 지웁니다.\n\n", "This runs Uninstall.bat in the game folder and removes geo-11, ShaderFixes and SRWeave.\n\n" },
            { "\n\n계속할까요?", "\n\nContinue?" },
            { "  dxgi.dll.reshade_bak → dxgi.dll 복원", "  dxgi.dll.reshade_bak → dxgi.dll restored" },
            { "Uninstall.bat 실행 완료.", "Uninstall.bat finished." }, { "전체 제거 완료", "Uninstalled" },
            { "SRWeave.log가 아직 없습니다 (게임을 한 번 실행하면 생깁니다).", "No SRWeave.log yet (it appears after the game has run once)." }, { "SRWeave.log가 아직 없습니다.", "No SRWeave.log yet." },
            // language switch
            { "언어: 한국어", "Language: English" }, { "English로 바꾸기", "Switch to 한국어" },
            // kind step (geo-11 vs Unreal Engine 4) and help text
            { "종류", "Kind" },
            { "픽스 종류", "Kind of fix" },
            { "geo-11 - Unity 게임(Universal Fix) 또는 일반 게임(Preferred)", "geo-11 - Unity game (Universal Fix) or generic game (Preferred)" },
            { "geo-11 드라이버 + 픽스 + SR 위빙. 64/32비트 패키지가 있고, Unity 게임은 Unity 버전에 맞는 include를 고릅니다. 단축키는 ShaderFixesDM\\hotkeys.ini에 씁니다.", "geo-11 driver + fix + SR weaving. 64- and 32-bit packages; a Unity game gets the include line for its Unity version. Hotkeys go to ShaderFixesDM\\hotkeys.ini." },
            { "UE4 게임 전용(64비트). Universal Fix 2 패키지를 Binaries\\Win64에 설치하고 geo-11 드라이버 배치·HUD 프로필·d3dx.ini 정리를 설치기가 대신합니다. AA/AO 개선, 전체화면, VSync, HUD 프로필은 설치 뒤 [UE4 설정...] 창에서 바꿉니다. 단축키는 d3dxdm.ini에 씁니다.", "UE4 games only (64-bit). Installs the Universal Fix 2 package into Binaries\\Win64 and does the geo-11 driver placement, HUD profile and d3dx.ini clean-up for you. AA/AO improvements, fullscreen, VSync and HUD profile are changed afterwards in the [UE4 settings...] window. Hotkeys go to d3dxdm.ini." },
            { "종류에 따라 다음 단계의 항목이 달라집니다. 두 종류는 서로 다른 패키지이므로 한 게임에 같이 설치하지 마세요.", "The next step changes with the kind. The two kinds are different packages, so do not install both into one game." },
            { "실행 파일 구조로 종류를 자동으로 고릅니다. 다르면 아래에서 바꾸세요.", "The kind is picked automatically from the exe's folder layout. Change it below if it is wrong." },
            { "자동 감지: ", "Detected: " },
            { " - 다르면 아래에서 바꾸세요.", " - change it below if it is wrong." },
            { "Unreal Engine 4 (Binaries\\Win64\\*-Win64-Shipping.exe 구조)", "Unreal Engine 4 (Binaries\\Win64\\*-Win64-Shipping.exe layout)" },
            { " (셰이더 캐시 시각 보정 ", " (shader cache time stamps fixed: " },
            { "[게임 추가]로 게임 실행 파일을 고르면 종류 → 설치 → 디스플레이 → 입체 값 순서로 설정하고 [완료]에서 게임 폴더에 적용합니다. ", "[Add game] picks the game exe, then you go Kind → Install → Display → Stereo values and [Finish] writes it to the game folder. " },
            { "  2. 1단계 [종류]: geo-11(Unity / 일반)과 Unreal Engine 4(Universal Fix 2) 중 하나. 실행 파일 구조로 자동으로 골라 주며, 다르면 바꿉니다. [다음].", "  2. Step 1 [Kind]: geo-11 (Unity / generic) or Unreal Engine 4 (Universal Fix 2). Picked automatically from the exe's folder layout; change it if wrong. [Next]." },
            { "  3. 2단계 [설치]: 고른 게임이 위에 표시되고 비트, 엔진(Unity / 일반), 기존 픽스가 자동으로 잡힙니다. 확인하고 [다음].", "  3. Step 2 [Install]: the chosen game is shown; bits, engine (Unity / generic) and an existing fix are detected. Check and press [Next]." },
            { "  4. 3단계 [디스플레이]: SR 위빙 사용 여부(SR 패널이 아니면 끄고 3D 출력 모드 선택), 필요하면 업스케일. [다음].", "  4. Step 3 [Display]: SR weaving on/off (off on a non-SR screen, then pick the 3D output mode), upscaling if needed. [Next]." },
            { "  5. 4단계 [입체 값]: Separation / Convergence. [완료]를 누르면 게임 폴더에 모두 적용되고 목록이 '설정 완료'로 바뀝니다.", "  5. Step 4 [Stereo values]: Separation / Convergence. [Finish] writes everything to the game folder and the list shows 'Configured'." },
            { "  6. [게임 목록]에서 실행(Steam 게임은 Steam으로), 설정 바꾸기, 폴더 열기, SR 위빙만 제거, 전체 제거를 할 수 있습니다.", "  6. From [Games] you can launch (Steam games through Steam), change settings, open the folder, remove SR weaving only, or uninstall everything." },
            { "  7. [단축키] 탭은 [완료]할 때 함께 저장됩니다. 이미 설정한 게임의 키만 바꾸려면 [설정 바꾸기] 뒤 [현재 게임에 단축키만 적용].", "  7. The [Hotkeys] tab is saved with [Finish]. To change only the keys of a game already set up: [Change settings] then [Apply hotkeys only to current game]." },
            { "  ShaderCache / ShaderCacheDM 는 전체 설치 때만 비웁니다 (SR만 추가·값 변경 때는 그대로 두어 다음 실행이 느려지지 않게).", "  ShaderCache / ShaderCacheDM are cleared only on a full install (kept on SR-only / value changes so the next start is not slow)." },
            // launch argument -dx11 for UF2 (Steam launch options / Heroic launcherArgs)
            { "실행기에 -dx11 넣기 (Steam 시작 옵션 / Heroic launcherArgs)", "Put -dx11 into the launcher (Steam launch options / Heroic launcherArgs)" },
            { "UF2는 게임을 -dx11 인자로 실행해야 합니다. [완료] 때 자동으로 넣으며, Steam 게임은 Steam의 시작 옵션에(Steam을 잠시 종료했다가 다시 시작), Epic/GOG 게임은 Heroic의 게임 설정(launcherArgs)에 씁니다. 이 설치기의 [실행]과 게임 목록의 실행 열에서 -dx11 여부를 볼 수 있습니다.", "UF2 needs the game started with the -dx11 argument. [Finish] adds it for you: for a Steam game into Steam's launch options (Steam is shut down briefly and restarted), for an Epic/GOG game into Heroic's per-game settings (launcherArgs). This installer's [Launch] and the Launch column in the games list show whether -dx11 is set." },
            { "     UF2는 게임을 -dx11 인자로 실행해야 합니다. [완료] 때 Steam 시작 옵션(Steam을 잠시 종료했다가 다시 시작) 또는 Heroic(Epic/GOG)의 게임 설정에 -dx11을 넣고, 설치기의 [실행]도 -dx11을 붙입니다.", "     UF2 needs the game started with -dx11. [Finish] puts -dx11 into the Steam launch options (Steam is shut down briefly and restarted) or into Heroic's (Epic/GOG) per-game settings, and the installer's [Launch] adds it too." },
            { "  주의: Steam 시작 옵션에 -dx11이 없습니다. [UE4 설정]의 [실행기에 -dx11 넣기]를 쓰세요.", "  note: Steam's launch options have no -dx11. Use [Put -dx11 into the launcher] in the UE4 section." },
            { " (-dx11 없음)", " (no -dx11)" },
            { "  Steam 시작 옵션에 이미 ", "  Steam launch options already contain " },
            { "가 있습니다: ", ": " },
            { "  Steam: 이 게임(appid ", "  Steam: no localconfig.vdf knows this game (appid " },
            { ")을 아는 localconfig.vdf를 찾지 못했습니다. Steam 라이브러리 → 속성 → 시작 옵션에 ", "). Put " },
            { "를 직접 넣으세요.", " into Steam library → Properties → Launch options yourself." },
            { "Unreal Engine 4 Universal Fix 2는 게임을 ", "Unreal Engine 4 Universal Fix 2 needs the game started with the " },
            { " 인자로 실행해야 합니다.\nSteam 라이브러리에서 실행할 때도 붙도록 Steam의 시작 옵션에 넣으려면 Steam을 잠시 종료해야 합니다 (편집 뒤 자동으로 다시 시작).\n\n", " argument.\nTo make Steam add it when the game is launched from the library, Steam has to be shut down briefly (it is started again after the edit).\n\n" },
            { "지금 Steam을 종료하고 넣을까요?\n[아니요]를 누르면 Steam 라이브러리 → 속성 → 시작 옵션에 ", "Shut Steam down and add it now?\nIf you choose [No], put " },
            { "를 직접 넣어야 합니다.", " into Steam library → Properties → Launch options yourself." },
            { "Steam 시작 옵션", "Steam launch options" },
            { "  Steam 시작 옵션은 넣지 않았습니다. Steam 라이브러리 → 속성 → 시작 옵션에 ", "  Steam launch options not changed. Put " },
            { "  Steam이 종료되지 않아 시작 옵션을 넣지 못했습니다. Steam을 직접 끝내고 [UE4 설정] 또는 [완료]를 다시 하세요.", "  Steam did not shut down, so the launch options were not changed. Close Steam yourself and run [UE4 settings] or [Finish] again." },
            { "  Steam: localconfig.vdf를 바꾸지 못했습니다. 시작 옵션에 ", "  Steam: could not change localconfig.vdf. Put " },
            { "  Steam 시작 옵션 = \"", "  Steam launch options = \"" },
            { "  Heroic(", "  Heroic (" },
            { ": 게임 설정 launcherArgs = \"", ": per-game launcherArgs = \"" },
            { "  Steam / Heroic 게임이 아닙니다. 이 설치기의 [실행]은 ", "  Not a Steam / Heroic game. This installer's [Launch] starts the game with " },
            { "를 붙여 실행하고, 바로 가기로 실행할 때는 ", "; when starting it from a shortcut add " },
            { "를 직접 붙이세요.", " yourself." },
            { "  d3dxdm.ini: 3D 켜기/끄기·현재 값 저장 키 섹션 추가 (UF2에는 없던 것)", "  d3dxdm.ini: 3D on/off and save-values key sections added (UF2 ships without them)" },
            { "  경고: 게임의 geo-11이 0.6.40이라 현재 값 저장 키가 동작하지 않습니다 (패키지에도 0.6.109가 없음).", "  warning: the game's geo-11 is 0.6.40, so the save-values key will not work (the package has no 0.6.109 either)." },
            { "  geo-11 드라이버를 0.6.40 → 0.6.109로 교체 (현재 값 저장 키에 필요)", "  geo-11 driver replaced 0.6.40 → 0.6.109 (needed by the save-values key)" },
            { "  경고: geo-11 드라이버 교체 실패 (게임이 실행 중?): ", "  warning: could not replace the geo-11 driver (game running?): " },
            { "HelixMod 블로그", "HelixMod blog" },
            { "3D 픽스 모음·소식 (settings.ini helixmod=)", "3D fixes and news (settings.ini helixmod=)" },
            { "3D Vision 커뮤니티 Discord 초대 (settings.ini discord=)", "3D Vision community Discord invite (settings.ini discord=)" },
            // Unreal Engine 4 settings window (replaces the console config tool)
            { "AA/AO 개선 설치 (모션 블러·비네트·렌즈 플레어 끄기, 게임 AA 설정은 그대로)", "Install the AA/AO improvements (motion blur, vignette and lens flare off, game AA settings kept)" },
            { "게임의 설정 폴더(%LOCALAPPDATA%\\<프로젝트>\\Saved\\Config\\WindowsNoEditor)의 Engine.ini와 Scalability.ini 끝에 패키지의 권장 설정을 붙이고 원본은 *_backup.ini로 보관합니다. 게임을 한 번도 실행하지 않아 그 폴더가 없으면 건너뛰고 로그에 남깁니다.", "Appends the package's recommended settings to Engine.ini and Scalability.ini in the game's config folder (%LOCALAPPDATA%\\<Project>\\Saved\\Config\\WindowsNoEditor) and keeps the originals as *_backup.ini. If the game has never run and the folder does not exist, this is skipped and noted in the log." },
            { "UE4 설정... (AA/AO, 전체화면, VSync, HUD 프로필)", "UE4 settings... (AA/AO, fullscreen, VSync, HUD profile)" },
            { "Universal Fix 2의 설정 도구가 하던 항목을 명령창 없이 이 창에서 바꿉니다. [완료]로 설치한 뒤 필요할 때 여세요.", "What Universal Fix 2's config tool used to do, without a console window. Open it after installing with [Finish]." },
            { "UF2가 아직 설치되지 않았습니다. 먼저 [완료]로 설치하세요.", "UF2 is not installed yet. Install with [Finish] first." },
            { "UE4 설정을 적용했습니다: ", "UE4 settings applied: " }, { "UE4 설정...", "UE4 settings..." }, { "Unreal Engine 4 게임이 아닙니다.", "Not an Unreal Engine 4 game." },
            { "UE4 설정 (Universal Fix 2)", "UE4 settings (Universal Fix 2)" },
            { "AA/AO 개선 (Engine.ini · Scalability.ini)", "AA/AO improvements (Engine.ini · Scalability.ini)" },
            { "AA/AO 개선 설치", "Install the AA/AO improvements" },
            { "모션 블러·비네트·렌즈 플레어 끄기, 그림자·AO 품질 고정. 원래 Engine.ini / Scalability.ini는 *_backup.ini로 보관되고 읽기 전용이 됩니다. 체크를 끄고 적용하면 원래 파일로 되돌립니다.", "Motion blur, vignette and lens flare off, fixed shadow / AO quality. The original Engine.ini / Scalability.ini are kept as *_backup.ini and become read-only. Untick and apply to restore the originals." },
            { "게임 AA 설정 그대로 (기본)", "Keep the game's AA settings (default)" },
            { "AA 강제 지정 (반투명 물체가 깨질 때)", "Force AA (for broken transparent objects)" },
            { "원래 도구의 \"ENABLE AA in Game\" / \"DISABLE AA in Game\"에 해당합니다. 강제 지정은 TAA 2샘플·AA 품질 고정 값을 넣습니다.", "The original tool's \"ENABLE AA in Game\" / \"DISABLE AA in Game\". Forcing writes TAA 2 samples and a fixed AA quality." },
            { "강제 전체화면", "Force fullscreen" },
            { "geo-11이 전체화면 스왑체인을 만듭니다. 창 모드에서 문제가 있을 때만 켜세요.", "geo-11 creates a fullscreen swap chain. Turn on only for windowed-mode problems." },
            { "게임 설정 폴더가 아직 없습니다. 게임을 한 번 실행한 뒤 다시 여세요.", "The game's config folder does not exist yet. Run the game once, then open this again." },
            { "현재: 설치됨 - ", "Currently: installed - " }, { "현재: 설치 안 됨 - ", "Currently: not installed - " },
            { "화면", "Display" },
            { "화면 표시(OSD) - 픽스 상태를 게임 화면에 표시", "On-screen display (OSD) - shows the fix status in the game" },
            { "VSync (NVIDIA 프로필):", "VSync (NVIDIA profile):" },
            { "게임 설정 따름 (기본)", "Application controlled (default)" }, { "항상 켬", "Force on" }, { "항상 끔", "Force off" }, { "1/2 (절반 주사율)", "1/2 (half refresh)" },
            { "SR 패널에서는 게임 설정 따름(기본)을 권합니다. 끊김이 심할 때만 1/2 등을 시험하세요.", "On an SR panel keep the default (application controlled). Try 1/2 etc. only for heavy stutter." },
            { "HUD 프로필 (AutoDepthUIHUD.ini)", "HUD profile (AutoDepthUIHUD.ini)" }, { "프로필:", "Profile:" },
            { "게임별 HUD 깊이 설정입니다. 실행 파일 이름과 같은 프로필이 있으면 그것이 기본이고, 없으면 DEFAULT입니다. (", "Per-game HUD depth settings. A profile named after the exe is the default when it exists, otherwise DEFAULT. (" },
            { "자동 선택: ", "auto-selected: " },
            { "그 밖의 항목", "Other items" },
            { "조준 키 수렴 배율, HUD 픽스 끄기/셰이더 헌팅, 자동 수렴/자동 깊이 모드는 이 패키지(Win11판)에 해당 파일이 없어 원래 도구에서도 동작하지 않습니다. 필요하면 아래에서 원래 도구(명령창)를 열 수 있습니다.", "Aim-key convergence scale, HUD-fix disable / shader hunting and auto convergence / auto depth modes have no files in this package (Win11 build), so the original tool cannot do them either. The original console tool can still be opened below." },
            { "원래 설정 도구(명령창) 열기", "Open the original config tool (console)" },
            { "적용", "Apply" }, { "닫기", "Close" },
            { "UE4 설정 적용: ", "Applying UE4 settings: " }, { "UE4 설정 적용 완료.", "UE4 settings applied." },
            { "  AA/AO: 게임 설정 폴더가 아직 없습니다 (게임을 한 번 실행한 뒤 다시 하세요).", "  AA/AO: the game's config folder does not exist yet (run the game once, then retry)." },
            { "  AA/AO: 패키지에 Scalability.ini / Engine_additions.txt가 없습니다.", "  AA/AO: Scalability.ini / Engine_additions.txt missing from the package." },
            { "  AA/AO 개선 설치 (", "  AA/AO improvements installed (" }, { "게임 AA 설정 그대로", "game AA settings kept" }, { "AA 강제 지정", "AA forced" },
            { "  AA/AO: 게임 설정 폴더가 없습니다.", "  AA/AO: no game config folder." },
            { "  AA/AO 개선 제거 (원래 Engine.ini / Scalability.ini 복원): ", "  AA/AO improvements removed (original Engine.ini / Scalability.ini restored): " },
            { "  HUD 프로필: ", "  HUD profile: " },
            // Unreal Engine 4 (Universal Fix 2)
            { "Unreal Engine 4 추가 설정 (Universal Fix 2)", "Unreal Engine 4 additional settings (Universal Fix 2)" },
            { "  UF2: geo-11 드라이버(ShaderFixes\\Geo11)를 게임 폴더에 배치", "  UF2: geo-11 driver (ShaderFixes\\Geo11) placed in the game folder" },
            { "  UF2: HUD 프로필 ", "  UF2: HUD profile " },
            { "  UF2: d3dx.ini force_stereo=2, get_resolution_from, 3dvision2sbs include 끔", "  UF2: d3dx.ini force_stereo=2, get_resolution_from, 3dvision2sbs include off" },
            { "Unreal Engine 4 (Universal Fix 2 패키지)", "Unreal Engine 4 (Universal Fix 2 package)" },
            { "     UE4 게임은 Universal Fix 2 패키지를 Binaries\\Win64에 설치합니다. AA/AO 개선·전체화면·VSync·HUD 프로필 같은 게임별 설정은 설치 뒤 [UE4 설정...] 창에서 바꿉니다.", "     UE4 games get the Universal Fix 2 package in Binaries\\Win64. Per-game settings such as AA/AO improvements, fullscreen, VSync and HUD profile are changed afterwards in the [UE4 settings...] window." },
        };
    }
}
