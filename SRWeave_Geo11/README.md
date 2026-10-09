# SRWeave_Geo11 — geo-11 SBS를 SR 패널용으로 위빙하는 dxgi.dll

UE4 Universal Fix 2 SR판, 그리고 GameBridge 배포본 기반의 `geo-11 v0.6.109_Unity_Complete_SR`·`geo-11 v0.6.109_Preferred_SR`
(x64/x32)에 들어가는 `dxgi.dll`(루트와 `ShaderFixes\SRWeave\`)의 소스다. ReShade 코드는 없고 SR SDK의 `IDX11Weaver1`만 쓴다.
게임 폴더에서 geo-11(`d3d11.dll`) 옆에 `dxgi.dll`로 놓이고, System32(x64)/SysWOW64(x86) `dxgi.dll`의 export 20개를 모두
같은 서수로 넘겨준다(5개는 C++, 나머지 15개는 점프 스텁: x64는 `src\dxgi_forward.asm`, x86은 `src\dxgi_forward_x86.cpp`의 naked 함수).
32비트 dxgi는 `CreateDXGIFactory1/2/`(무접미) 서수가 10/11/12로 64비트와 달라 `src\dxgi32.def`를 따로 둔다.

단축키는 `SRWeave.ini`의 `key_weave`/`key_swap`/`key_lens`/`key_test`로 바꿀 수 있다(Ctrl+Alt 고정; 글자/숫자, F1-F24, NUMPAD0-9,
VK_ 이름, 0x 16진수, none). 잘못된 값은 로그에 남기고 기본값을 쓴다.

## 동작

1. 첫 `CreateDXGIFactory*` 호출 때 System32 dxgi로 팩토리를 하나 만들어(창 필요 없음) 진짜
   `IDXGIFactory::CreateSwapChain`, `IDXGIFactory2::CreateSwapChainForHwnd`를 MinHook으로 후킹한다.
2. 게임(geo-11 경유)이 스왑체인을 만들면 진짜 dxgi가 돌려준 객체의 vtable에서 Present 주소를 얻는다.
   그 객체가 래퍼일 때를 대비해, 필드 중 vtable 18칸 중 12칸 이상이 System32 `dxgi.dll`을 가리키고
   `QueryInterface(IDXGISwapChain)`에 답하는 객체를 진짜 스왑체인으로 고르는 경로도 남겨 두었다.
3. 진짜 `IDXGISwapChain::Present`/`ResizeBuffers`, `IDXGISwapChain1::Present1`을 MinHook으로 인라인 후킹한다.
   geo-11이 SBS를 다 만든 뒤 진짜 Present를 부르면, 그 안에서 위빙한다.
4. 위빙: 백버퍼 → 복사 텍스처 → `setInputViewTexture(srv, 폭/2, 높이, 포맷)` → 백버퍼 RTV 바인딩 → `weave()` →
   파이프라인 상태 복원 → 원래 Present. D3D 호출은 진짜 디바이스로만 하므로 geo-11이 위버의 그리기를 스테레오로 바꾸지 않는다.

SR 초기화 순서는 SRCapture3D `sr_direct.cpp`와 같다. SR 런타임 DLL은 지연 로드로 SpatialLabs/SR Platform 폴더에서 찾는다.
런타임이나 SR Service가 없으면 위빙만 꺼지고 게임은 그대로 돈다.

`tridef-injector\srweave`(TriDef용 SRWeaveDX11)에서 갈라져 나왔다. 다른 점: 임시 디바이스·스왑체인·창을 만들지 않음(아래),
디바이스가 바뀐 스왑체인 상태 초기화, flip model 버퍼별 RTV, 백버퍼/창 크기 불일치 경고, dxgi 전체 export.

### sRGB 백버퍼 (2026-10-08, art of rally)

Unity 게임(art of rally)이 독점 전체화면에서 `R8G8B8A8_UNORM_SRGB`(29) 백버퍼를 만들자 매 프레임 `CreateShaderResourceView failed 0x80070057`이 나며
위빙이 안 됐다. 복사 텍스처를 백버퍼와 같은 typed sRGB 포맷으로 만들고 뷰는 UNORM으로 만들었기 때문이다(typed 텍스처는 자기 포맷의 뷰만 허용).
지금은 복사 텍스처를 그 포맷 계열의 TYPELESS(`TypelessOf`)로 만들어 UNORM/UNORM_SRGB 뷰를 모두 허용하고, 백버퍼 RTV는 백버퍼의 정확한
포맷으로 만든다. 뷰 생성이 실패하면 포맷을 로그에 남기고 그 스왑체인은 건너뛰며(매 프레임 반복 로그 없음), ResizeBuffers 때 다시 시도한다.
srtest에 `srgb` 인자(DISCARD + UNORM_SRGB 백버퍼)로 재현·검증했다.

### 임시 스왑체인을 만들면 안 되는 이유 (2026-10-03)

처음 판은 srweave처럼 숨은 창에 임시 디바이스+스왑체인을 만들어 Present 주소를 얻었다. geo-11은 그 임시 스왑체인도 감싸고
그 창을 서브클래싱하는데, 그 뒤로 **게임에서 키보드·마우스·컨트롤러가 전부 먹지 않았다**. 키 메시지는 게임 창 스레드까지
도착했지만 게임은 반응하지 않았고, `weave=0`(SR 없이 훅만)에서도 똑같았다. 그래서 원인은 SR이 아니라 임시 창이다.
팩토리 후킹으로 바꾼 뒤 같은 테스트(메뉴에서 ↓ → 선택이 CONTINUE에서 NEW GAME으로 이동)를 통과했다.
`diag_input=1`이면 게임 창 스레드가 받는 입력·활성화 메시지와 Raw Input 등록을 `SRWeave.log`에 남긴다.

## 빌드

```
powershell -ExecutionPolicy Bypass -File build.ps1            # x64 + x86
powershell -ExecutionPolicy Bypass -File build.ps1 -Arch x86  # 한쪽만 (x64 | x86)
```

Visual Studio 2026 C++ 도구(x64 빌드는 MASM 포함)와 SR SDK 1.34.10이 필요하다. SDK 경로 기본값은
SR 런타임 설치본(또는 Leia/SR SDK)을 풀어 둔 폴더의 `simulatedreality-1.34.10-win64-Release (extracted SDK)`(x64)와
`simulatedreality-1.34.10-win32-Release (extracted SDK)`(x86)이고, 다른 곳이면 `build.ps1 -Arch x86 -DSR_SDK=<경로>`로 바꾼다.
32비트 SDK는 런타임 설치본 `LeiaSR-Runtime-1.34.10-win64\temp\simulatedreality-1.34.10-win32-Release.exe`(NSIS)를 7-Zip으로 풀면 나온다
(헤더는 64비트와 동일, `lib\*32.lib`, `third_party\OpenCV\lib\x86`). 32비트 런타임 DLL(`SimulatedRealityCore32.dll` 등)은 SR Platform이
`C:\Program Files (x86)\Simulated Reality\Platform\bin`에 설치한다. 결과물은 `bin\x64\dxgi.dll`, `bin\x86\dxgi.dll`. 정적 CRT라 재배포 패키지가 필요 없다.
빌드 뒤 dll과 `SRWeave.ini`를 각 SR판 폴더(의 x64/x32) 루트와 `ShaderFixes\SRWeave\`에 복사한다.

스모크 테스트(게임 없이): scratchpad의 `srtest`(D3D11 창 + 스왑체인, N프레임 Present)를 x86/x64로 빌드해 옆에 `dxgi.dll`+`SRWeave.ini`만
두고 실행하면 `SRWeave.log`에 훅 → SR 컨텍스트 → DX11 위버 생성 → `first weaved frame`이 남아야 한다 (2026-10-08 두 아키텍처 모두 통과).

MinHook(BSD-2)은 `third_party\minhook`. SR SDK·런타임은 Leia/Acer 것이라 포함하지 않는다.
