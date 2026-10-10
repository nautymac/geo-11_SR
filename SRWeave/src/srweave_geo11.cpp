// SRWeave for geo-11 (UE4 Universal Fix 2, Unity Universal Fix, plain geo-11) - dxgi.dll proxy that
// weaves geo-11's side-by-side output for an SR (Simulated Reality / SpatialLabs / Leia) lenticular
// display, inside the game process, right before the real IDXGISwapChain::Present.
// No ReShade, no screen capture: only the SR SDK's own IDX11Weaver1. Builds for x64 and x86.
//
//   game folder:  d3d11.dll  = geo-11 (renders both eyes, composes SBS into the back buffer)
//                 dxgi.dll   = this file (forwards every dxgi export to System32\dxgi.dll)
//
// geo-11 wraps the swap chain and finally calls the *real* dxgi Present. We inline-hook that real
// function (MinHook), so by the time we run the back buffer already holds geo-11's SBS image:
// copy it to a texture -> setInputViewTexture(half width) -> bind back buffer -> weave() -> Present.
// All our D3D calls go to the real device/context (from the real swap chain), never through
// geo-11, so geo-11 does not stereo-ize the weaver's draw.
//
// SR sequence is the same as SRCapture3D's sr_direct.cpp (and VRto3D's LeiaSR presenter):
//   SRContext::create -> CreateDX11Weaver(ctx, d3dContext, hwnd) -> setLatencyInFrames(1)
//   -> setContext -> SwitchableLensHint::create -> context->initialize()
#include <windows.h>
#include <delayimp.h>
#include <d3d11_1.h>
#include <dxgi1_2.h>

#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <exception>
#include <map>
#include <mutex>
#include <string>

#include "MinHook.h"
#include "sr/management/srcontext.h"
#include "sr/sense/display/switchablehint.h"
#include "sr/utility/exception.h"
#include "sr/weaver/dx11weaver.h"

namespace {

template <typename T> void SafeRelease(T*& p) { if (p) { p->Release(); p = nullptr; } }

// ---- config / log --------------------------------------------------------------------------

struct Config {
    bool weave = true;       // Ctrl+Alt+W
    bool swapEyes = false;   // Ctrl+Alt+S - geo-11 outputs L|R; set if depth looks inverted
    bool lens = true;        // Ctrl+Alt+L - lenticular lens on while weaving
    bool test = false;       // Ctrl+Alt+T - red left / blue right instead of the game
    int srgb = -1;           // -1 auto: follow the back buffer format (UNORM_SRGB -> sRGB views), 0 force UNORM, 1 force sRGB
    bool log = true;
    int latencyFrames = 1;
    bool diagInput = false;  // log what input the game window thread receives
    bool weaverHwnd = true;  // give the weaver the game window (0: nullptr, weaver assumes fullscreen)
    // hotkeys (always Ctrl+Alt + this virtual key; 0 = unbound). SRWeave.ini: key_weave=W, key_swap=S, key_lens=L, key_test=T
    int keyWeave = 'W', keySwap = 'S', keyLens = 'L', keyTest = 'T';
};
Config g_cfg;
HMODULE g_self = nullptr;
std::mutex g_logMutex;
FILE* g_logFile = nullptr;

std::wstring SelfDir()
{
    wchar_t p[MAX_PATH] = {};
    GetModuleFileNameW(g_self, p, MAX_PATH);
    std::wstring s(p);
    size_t i = s.find_last_of(L"\\/");
    return i == std::wstring::npos ? L"." : s.substr(0, i);
}

void Log(const char* fmt, ...)
{
    if (!g_logFile) return;
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    SYSTEMTIME t;
    GetLocalTime(&t);
    std::lock_guard<std::mutex> lk(g_logMutex);
    fprintf(g_logFile, "%02d:%02d:%02d.%03d [%lu] %s\n", t.wHour, t.wMinute, t.wSecond, t.wMilliseconds, GetCurrentThreadId(), buf);
    fflush(g_logFile);
}

// "W", "5", "F6", "NUMPAD0", "VK_F6", "0x57" or "none" -> virtual key code (0 = unbound, -1 = not understood)
int ParseKey(const wchar_t* s)
{
    std::wstring k(s);
    while (!k.empty() && iswspace(k.back())) k.pop_back();
    size_t st = 0;
    while (st < k.size() && iswspace(k[st])) st++;
    k = k.substr(st);
    for (auto& ch : k) ch = (wchar_t)towupper(ch);
    if (k.empty() || k == L"NONE" || k == L"0") return 0;
    if (k.rfind(L"VK_", 0) == 0) k = k.substr(3);
    if (k.size() == 1 && ((k[0] >= L'A' && k[0] <= L'Z') || (k[0] >= L'0' && k[0] <= L'9'))) return k[0];
    if (k.size() >= 2 && k[0] == L'F' && iswdigit(k[1])) { int n = _wtoi(k.c_str() + 1); if (n >= 1 && n <= 24) return VK_F1 + n - 1; }
    if (k.rfind(L"NUMPAD", 0) == 0 && k.size() == 7 && iswdigit(k[6])) return VK_NUMPAD0 + (k[6] - L'0');
    if (k.rfind(L"0X", 0) == 0) { int v = (int)wcstol(k.c_str(), nullptr, 16); return v > 0 && v < 256 ? v : -1; }
    static const struct { const wchar_t* n; int vk; } named[] = {
        { L"SPACE", VK_SPACE }, { L"TAB", VK_TAB }, { L"RETURN", VK_RETURN }, { L"ENTER", VK_RETURN }, { L"BACK", VK_BACK },
        { L"INSERT", VK_INSERT }, { L"DELETE", VK_DELETE }, { L"HOME", VK_HOME }, { L"END", VK_END }, { L"PRIOR", VK_PRIOR }, { L"NEXT", VK_NEXT },
        { L"PAGEUP", VK_PRIOR }, { L"PAGEDOWN", VK_NEXT }, { L"LEFT", VK_LEFT }, { L"RIGHT", VK_RIGHT }, { L"UP", VK_UP }, { L"DOWN", VK_DOWN },
        { L"MULTIPLY", VK_MULTIPLY }, { L"ADD", VK_ADD }, { L"SUBTRACT", VK_SUBTRACT }, { L"DECIMAL", VK_DECIMAL }, { L"DIVIDE", VK_DIVIDE },
        { L"PAUSE", VK_PAUSE }, { L"SCROLL", VK_SCROLL }, { L"SNAPSHOT", VK_SNAPSHOT },
    };
    for (const auto& e : named) if (k == e.n) return e.vk;
    return -1;
}

int LoadKey(const std::wstring& ini, const wchar_t* name, int def)
{
    wchar_t buf[64] = {};
    GetPrivateProfileStringW(L"SRWeave", name, L"", buf, 64, ini.c_str());
    if (!buf[0]) return def;
    int vk = ParseKey(buf);
    if (vk < 0) { Log("SRWeave.ini: %ls=%ls not understood - keeping default", name, buf); return def; }
    return vk;
}

void LoadConfig()
{
    std::wstring ini = SelfDir() + L"\\SRWeave.ini";
    auto b = [&](const wchar_t* k, bool d) { return GetPrivateProfileIntW(L"SRWeave", k, d ? 1 : 0, ini.c_str()) != 0; };
    g_cfg.weave = b(L"weave", true);
    g_cfg.swapEyes = b(L"swap_eyes", false);
    g_cfg.lens = b(L"lens", true);
    g_cfg.test = b(L"test", false);
    {
        // "auto" is not a number, so read it as a string
        wchar_t buf[16] = {};
        GetPrivateProfileStringW(L"SRWeave", L"srgb", L"auto", buf, 16, ini.c_str());
        if (_wcsicmp(buf, L"1") == 0) g_cfg.srgb = 1;
        else if (_wcsicmp(buf, L"0") == 0) g_cfg.srgb = 0;
        else g_cfg.srgb = -1;   // "auto" or anything else
    }
    g_cfg.log = b(L"log", true);
    g_cfg.latencyFrames = (int)GetPrivateProfileIntW(L"SRWeave", L"latency_frames", 1, ini.c_str());
    g_cfg.diagInput = b(L"diag_input", false);
    g_cfg.weaverHwnd = b(L"weaver_hwnd", true);
    if (g_cfg.log) {
        // overwrite each run: one game session per log
        std::wstring lp = SelfDir() + L"\\SRWeave.log";
        g_logFile = _wfopen(lp.c_str(), L"w");
    }
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    Log("=== SRWeave geo-11 (dxgi.dll proxy) in %ls (pid %lu) ===", exe, GetCurrentProcessId());
    Log("config: weave=%d swap_eyes=%d lens=%d test=%d srgb=%s latency_frames=%d diag_input=%d weaver_hwnd=%d",
        g_cfg.weave, g_cfg.swapEyes, g_cfg.lens, g_cfg.test, g_cfg.srgb < 0 ? "auto" : g_cfg.srgb ? "1" : "0", g_cfg.latencyFrames, g_cfg.diagInput, g_cfg.weaverHwnd);
    g_cfg.keyWeave = LoadKey(ini, L"key_weave", 'W');
    g_cfg.keySwap = LoadKey(ini, L"key_swap", 'S');
    g_cfg.keyLens = LoadKey(ini, L"key_lens", 'L');
    g_cfg.keyTest = LoadKey(ini, L"key_test", 'T');
    Log("hotkeys (Ctrl+Alt+vk): weave 0x%02X swap 0x%02X lens 0x%02X test 0x%02X", g_cfg.keyWeave, g_cfg.keySwap, g_cfg.keyLens, g_cfg.keyTest);
}

// ---- real System32 dxgi / d3d11 ------------------------------------------------------------

HMODULE g_dxgi = nullptr, g_d3d11 = nullptr;
std::once_flag g_realOnce;

HMODULE LoadSystemDll(const wchar_t* name)
{
    wchar_t dir[MAX_PATH] = {};
    GetSystemDirectoryW(dir, MAX_PATH);
    // Full path: a different module from us (dxgi.dll) and from geo-11 (d3d11.dll) in the game folder.
    return LoadLibraryExW((std::wstring(dir) + L"\\" + name).c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
}

void LoadReal()
{
    std::call_once(g_realOnce, [] { g_dxgi = LoadSystemDll(L"dxgi.dll"); });
}
template <typename T> T RealDxgi(const char* n) { LoadReal(); return g_dxgi ? (T)GetProcAddress(g_dxgi, n) : nullptr; }

std::string ModuleOf(const void* addr)
{
    HMODULE m = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)addr, &m) || !m)
        return "?";
    char p[MAX_PATH] = {};
    GetModuleFileNameA(m, p, MAX_PATH);
    return p;
}

bool AddrInModule(const void* addr, HMODULE m)
{
    HMODULE owner = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)addr, &owner);
    return owner && owner == m;
}

// ---- SR runtime ----------------------------------------------------------------------------

// The SR runtime DLLs are delay-loaded; look in the SpatialLabs / SR Platform folders first.
// The 64-bit runtime (SimulatedRealityCore.dll ...) lives under Program Files, the 32-bit one
// (SimulatedRealityCore32.dll ...) under Program Files (x86); each build only needs its own.
const wchar_t* const kRuntimeDirs[] = {
#if defined(_WIN64)
    L"C:\\Program Files\\Acer\\SpatialLabs\\Platform\\bin",
    L"C:\\Program Files\\Simulated Reality\\Platform\\bin",
    L"C:\\Program Files\\LeiaSR\\Platform\\bin",
#else
    L"C:\\Program Files (x86)\\Acer\\SpatialLabs\\Platform\\bin",
    L"C:\\Program Files (x86)\\Simulated Reality\\Platform\\bin",
    L"C:\\Program Files (x86)\\LeiaSR\\Platform\\bin",
#endif
};

FARPROC WINAPI DliHook(unsigned reason, DelayLoadInfo* info)
{
    if (reason != dliNotePreLoadLibrary || !info || !info->szDll) return nullptr;
    wchar_t name[MAX_PATH] = {};
    MultiByteToWideChar(CP_ACP, 0, info->szDll, -1, name, MAX_PATH);
    for (const wchar_t* dir : kRuntimeDirs) {
        std::wstring cand = std::wstring(dir) + L"\\" + name;
        if (HMODULE m = LoadLibraryExW(cand.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH)) return (FARPROC)m;
    }
    return nullptr;   // normal search (PATH)
}

struct SrCore {
    SR::SRContext* ctx = nullptr;
    SR::SwitchableLensHint* lens = nullptr;
    bool initialized = false;
    bool dead = false;
    bool lensOn = false;
    ULONGLONG lastTry = 0;
    int tries = 0;
} g_sr;

int DelayLoadFilter(unsigned code)
{
    return (code == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND)
            || code == VcppException(ERROR_SEVERITY_ERROR, ERROR_PROC_NOT_FOUND))
           ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH;
}

void DoCreate(SR::SRContext** out) { *out = SR::SRContext::create(); }

bool DoCreateSEH(SR::SRContext** out)
{
    __try {
        DoCreate(out);
        return true;
    } __except (DelayLoadFilter(GetExceptionCode())) {
        return false;
    }
}

bool SrEnsureContext()
{
    if (g_sr.ctx) return true;
    if (g_sr.dead) return false;
    ULONGLONG now = GetTickCount64();
    if (now - g_sr.lastTry < 2000) return false;   // don't hammer the service from the render thread
    g_sr.lastTry = now;
    g_sr.tries++;
    try {
        if (!DoCreateSEH(&g_sr.ctx)) {
            Log("SR runtime DLLs not found (SimulatedRealityCore/DirectX.dll) - weaving disabled");
            g_sr.dead = true;
            return false;
        }
    } catch (const SR::ServerNotAvailableException&) {
        g_sr.ctx = nullptr;
        if (g_sr.tries == 1 || g_sr.tries % 10 == 0) Log("SR Service not available (try %d)", g_sr.tries);
        if (g_sr.tries >= 60) { Log("giving up on SR Service"); g_sr.dead = true; }
        return false;
    } catch (const std::exception& e) {
        Log("SRContext::create failed: %s", e.what());
        g_sr.ctx = nullptr; g_sr.dead = true;
        return false;
    } catch (...) {
        Log("SRContext::create failed (unknown exception)");
        g_sr.ctx = nullptr; g_sr.dead = true;
        return false;
    }
    Log("SR context created (try %d)", g_sr.tries);
    return g_sr.ctx != nullptr;
}

void SrFinishInit()
{
    if (!g_sr.ctx || g_sr.initialized) return;
    try {
        g_sr.lens = SR::SwitchableLensHint::create(*g_sr.ctx);   // owned by the context
        g_sr.ctx->initialize();
        g_sr.initialized = true;
        Log("SR context initialized");
    } catch (const std::exception& e) {
        Log("SR initialize failed: %s", e.what());
        g_sr.dead = true;
    } catch (...) {
        Log("SR initialize failed (unknown exception)");
        g_sr.dead = true;
    }
}

void SrSetLens(bool on)
{
    if (!g_sr.lens || g_sr.lensOn == on) return;
    try {
        if (on) g_sr.lens->enable(); else g_sr.lens->disable();
        g_sr.lensOn = on;
        Log("lens %s", on ? "ON" : "OFF");
    } catch (...) {
        Log("lens hint call failed");
    }
}

// ---- hotkeys (Ctrl+Alt+key, rising edge, polled once per Present) ---------------------------

bool Hotkey(int vk)
{
    static bool prev[256] = {};
    if (vk <= 0 || vk > 255) return false;
    bool down = (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(vk) & 0x8000);
    bool edge = down && !prev[vk & 0xff];
    prev[vk & 0xff] = down;
    return edge;
}

// returns true if the lens state should be re-applied
bool PollHotkeys()
{
    bool lensChanged = false;
    if (Hotkey(g_cfg.keyWeave)) { g_cfg.weave = !g_cfg.weave; Log("hotkey: weave %s", g_cfg.weave ? "ON" : "OFF"); lensChanged = true; }
    if (Hotkey(g_cfg.keySwap)) { g_cfg.swapEyes = !g_cfg.swapEyes; Log("hotkey: swap_eyes %d", g_cfg.swapEyes); }
    if (Hotkey(g_cfg.keyTest)) { g_cfg.test = !g_cfg.test; Log("hotkey: test pattern %d", g_cfg.test); }
    if (Hotkey(g_cfg.keyLens)) { g_cfg.lens = !g_cfg.lens; Log("hotkey: lens %d", g_cfg.lens); lensChanged = true; }
    return lensChanged;
}

// ---- per-swapchain weaving -----------------------------------------------------------------

typedef HRESULT(STDMETHODCALLTYPE* PFN_Present)(IDXGISwapChain*, UINT, UINT);
typedef HRESULT(STDMETHODCALLTYPE* PFN_Present1)(IDXGISwapChain1*, UINT, UINT, const DXGI_PRESENT_PARAMETERS*);
typedef HRESULT(STDMETHODCALLTYPE* PFN_ResizeBuffers)(IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT);
PFN_Present g_origPresent = nullptr;
PFN_Present1 g_origPresent1 = nullptr;
PFN_ResizeBuffers g_origResizeBuffers = nullptr;

struct ScState {
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    SR::IDX11Weaver1* weaver = nullptr;
    ID3D11Texture2D* tex = nullptr;            // copy of the SBS back buffer (weaver input)
    ID3D11ShaderResourceView* srv = nullptr;
    ID3D11RenderTargetView* texRtv = nullptr;  // test pattern
    ID3D11RenderTargetView* bbRtv = nullptr;
    ID3D11Texture2D* bbTex = nullptr;          // which back buffer bbRtv points at (not ref-counted)
    UINT w = 0, h = 0;
    DXGI_FORMAT fmt = DXGI_FORMAT_UNKNOWN;
    HWND hwnd = nullptr;
    bool failed = false;
    bool inputBound = false;
    bool sizeWarned = false;
    unsigned frames = 0;
};
std::mutex g_scMutex;
std::map<IDXGISwapChain*, ScState> g_scs;
thread_local bool t_inPresent = false;

// The typeless member of the back buffer's format family. Our copy of the back buffer is created with
// this so that UNORM and UNORM_SRGB views are both legal on it. A typed *_UNORM_SRGB texture accepts
// views of exactly that format only - that is what made CreateShaderResourceView fail with E_INVALIDARG
// (0x80070057) every frame in games whose back buffer is sRGB (Unity "art of rally" in exclusive fullscreen).
DXGI_FORMAT TypelessOf(DXGI_FORMAT f)
{
    switch (f) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: case DXGI_FORMAT_R8G8B8A8_UNORM: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB: return DXGI_FORMAT_R8G8B8A8_TYPELESS;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: case DXGI_FORMAT_B8G8R8A8_UNORM: case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8A8_TYPELESS;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS: case DXGI_FORMAT_B8G8R8X8_UNORM: case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB: return DXGI_FORMAT_B8G8R8X8_TYPELESS;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS: case DXGI_FORMAT_R10G10B10A2_UNORM: return DXGI_FORMAT_R10G10B10A2_TYPELESS;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: case DXGI_FORMAT_R16G16B16A16_FLOAT: return DXGI_FORMAT_R16G16B16A16_TYPELESS;
    default: return f;
    }
}

bool IsSrgbFormat(DXGI_FORMAT f)
{
    return f == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB || f == DXGI_FORMAT_B8G8R8A8_UNORM_SRGB || f == DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
}

// srgbMode: -1 auto (match the back buffer), 0 UNORM, 1 UNORM_SRGB
DXGI_FORMAT ViewFormat(DXGI_FORMAT f, int srgbMode)
{
    const bool srgb = srgbMode < 0 ? IsSrgbFormat(f) : srgbMode != 0;
    switch (f) {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS: case DXGI_FORMAT_R8G8B8A8_UNORM: case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return srgb ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
    case DXGI_FORMAT_B8G8R8A8_TYPELESS: case DXGI_FORMAT_B8G8R8A8_UNORM: case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return srgb ? DXGI_FORMAT_B8G8R8A8_UNORM_SRGB : DXGI_FORMAT_B8G8R8A8_UNORM;
    case DXGI_FORMAT_B8G8R8X8_TYPELESS: case DXGI_FORMAT_B8G8R8X8_UNORM: case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return srgb ? DXGI_FORMAT_B8G8R8X8_UNORM_SRGB : DXGI_FORMAT_B8G8R8X8_UNORM;
    case DXGI_FORMAT_R10G10B10A2_TYPELESS: return DXGI_FORMAT_R10G10B10A2_UNORM;
    case DXGI_FORMAT_R16G16B16A16_TYPELESS: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    default: return f;
    }
}

// Everything the weaver's draw may clobber; restored so the game's next frame starts unchanged.
struct SavedState {
    ID3D11RenderTargetView* rtv[8] = {}; ID3D11DepthStencilView* dsv = nullptr;
    UINT nvp = 16; D3D11_VIEWPORT vp[16] = {};
    UINT nsc = 16; D3D11_RECT sc[16] = {};
    ID3D11RasterizerState* rs = nullptr;
    ID3D11BlendState* bs = nullptr; float bf[4] = {}; UINT sm = 0;
    ID3D11DepthStencilState* dss = nullptr; UINT sref = 0;
    ID3D11VertexShader* vs = nullptr; ID3D11PixelShader* ps = nullptr; ID3D11GeometryShader* gs = nullptr;
    ID3D11HullShader* hs = nullptr; ID3D11DomainShader* ds = nullptr;
    ID3D11InputLayout* il = nullptr; D3D11_PRIMITIVE_TOPOLOGY topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    ID3D11Buffer* vb[2] = {}; UINT vbs[2] = {}, vbo[2] = {};
    ID3D11Buffer* ib = nullptr; DXGI_FORMAT ibf = DXGI_FORMAT_UNKNOWN; UINT ibo = 0;
    ID3D11ShaderResourceView* psSrv[8] = {}; ID3D11SamplerState* psSamp[4] = {};
    ID3D11Buffer* vsCb[4] = {}; ID3D11Buffer* psCb[4] = {};

    void Capture(ID3D11DeviceContext* c) {
        c->OMGetRenderTargets(8, rtv, &dsv);
        c->RSGetViewports(&nvp, vp);
        c->RSGetScissorRects(&nsc, sc);
        c->RSGetState(&rs);
        c->OMGetBlendState(&bs, bf, &sm);
        c->OMGetDepthStencilState(&dss, &sref);
        c->VSGetShader(&vs, nullptr, nullptr); c->PSGetShader(&ps, nullptr, nullptr); c->GSGetShader(&gs, nullptr, nullptr);
        c->HSGetShader(&hs, nullptr, nullptr); c->DSGetShader(&ds, nullptr, nullptr);
        c->IAGetInputLayout(&il); c->IAGetPrimitiveTopology(&topo);
        c->IAGetVertexBuffers(0, 2, vb, vbs, vbo); c->IAGetIndexBuffer(&ib, &ibf, &ibo);
        c->PSGetShaderResources(0, 8, psSrv); c->PSGetSamplers(0, 4, psSamp);
        c->VSGetConstantBuffers(0, 4, vsCb); c->PSGetConstantBuffers(0, 4, psCb);
    }
    void Restore(ID3D11DeviceContext* c) {
        c->OMSetRenderTargets(8, rtv, dsv);
        c->RSSetViewports(nvp, vp);
        c->RSSetScissorRects(nsc, sc);
        c->RSSetState(rs);
        c->OMSetBlendState(bs, bf, sm);
        c->OMSetDepthStencilState(dss, sref);
        c->VSSetShader(vs, nullptr, 0); c->PSSetShader(ps, nullptr, 0); c->GSSetShader(gs, nullptr, 0);
        c->HSSetShader(hs, nullptr, 0); c->DSSetShader(ds, nullptr, 0);
        c->IASetInputLayout(il); c->IASetPrimitiveTopology(topo);
        c->IASetVertexBuffers(0, 2, vb, vbs, vbo); c->IASetIndexBuffer(ib, ibf, ibo);
        c->PSSetShaderResources(0, 8, psSrv); c->PSSetSamplers(0, 4, psSamp);
        c->VSSetConstantBuffers(0, 4, vsCb); c->PSSetConstantBuffers(0, 4, psCb);
        for (auto& p : rtv) SafeRelease(p);
        SafeRelease(dsv); SafeRelease(rs); SafeRelease(bs); SafeRelease(dss);
        SafeRelease(vs); SafeRelease(ps); SafeRelease(gs); SafeRelease(hs); SafeRelease(ds);
        SafeRelease(il); for (auto& p : vb) SafeRelease(p); SafeRelease(ib);
        for (auto& p : psSrv) SafeRelease(p); for (auto& p : psSamp) SafeRelease(p);
        for (auto& p : vsCb) SafeRelease(p); for (auto& p : psCb) SafeRelease(p);
    }
};

void ReleaseSizeObjects(ScState& s)
{
    SafeRelease(s.srv); SafeRelease(s.texRtv); SafeRelease(s.tex); SafeRelease(s.bbRtv);
    s.bbTex = nullptr;
    s.w = s.h = 0;
    s.inputBound = false;
}

void ResetState(ScState& s)
{
    ReleaseSizeObjects(s);
    if (s.weaver) { try { s.weaver->destroy(); } catch (...) {} s.weaver = nullptr; }
    SafeRelease(s.ctx); SafeRelease(s.dev);
    s = ScState{};
}

// ---- input diagnostics (diag_input=1) ------------------------------------------------------
// Counts what the game window's thread actually receives, to tell "input never arrives" apart
// from "arrives but the game ignores it" (e.g. it thinks it is deactivated).

std::atomic<long> g_nKey{ 0 }, g_nMouse{ 0 }, g_nRawInput{ 0 };
HHOOK g_msgHook = nullptr, g_callHook = nullptr;

void LogWndProc(const char* when, HWND hwnd)
{
    if (!g_cfg.diagInput || !hwnd) return;
    void* wp = (void*)GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
    Log("WndProc %s: %p (%s), foreground=%d", when, wp, ModuleOf(wp).c_str(), GetForegroundWindow() == hwnd);
    RAWINPUTDEVICE rid[16] = {};
    UINT n = 16;
    UINT got = GetRegisteredRawInputDevices(rid, &n, sizeof(RAWINPUTDEVICE));
    if (got == (UINT)-1) { Log("  raw input: query failed (%lu)", GetLastError()); return; }
    Log("  raw input registrations: %u", got);
    for (UINT i = 0; i < got; i++) {
        DWORD pid = 0;
        if (rid[i].hwndTarget) GetWindowThreadProcessId(rid[i].hwndTarget, &pid);
        char cls[128] = {};
        if (rid[i].hwndTarget) GetClassNameA(rid[i].hwndTarget, cls, sizeof cls);
        Log("    page 0x%x usage 0x%x flags 0x%lx target %p class '%s'%s", rid[i].usUsagePage, rid[i].usUsage, rid[i].dwFlags,
            rid[i].hwndTarget, cls, rid[i].hwndTarget == hwnd ? " (game window)" : "");
    }
}

LRESULT CALLBACK GetMsgHook(int code, WPARAM wp, LPARAM lp)
{
    if (code == HC_ACTION && wp == PM_REMOVE) {
        const MSG* m = (const MSG*)lp;
        if (m->message >= WM_KEYFIRST && m->message <= WM_KEYLAST) g_nKey++;
        else if (m->message >= WM_MOUSEFIRST && m->message <= WM_MOUSELAST) g_nMouse++;
        else if (m->message == WM_INPUT) g_nRawInput++;
    }
    return CallNextHookEx(g_msgHook, code, wp, lp);
}

LRESULT CALLBACK CallWndHook(int code, WPARAM wp, LPARAM lp)
{
    if (code == HC_ACTION) {
        const CWPSTRUCT* c = (const CWPSTRUCT*)lp;
        switch (c->message) {
        case WM_ACTIVATEAPP: Log("msg WM_ACTIVATEAPP active=%d hwnd %p", (int)c->wParam, c->hwnd); break;
        case WM_ACTIVATE:    Log("msg WM_ACTIVATE state=%d hwnd %p", (int)LOWORD(c->wParam), c->hwnd); break;
        case WM_SETFOCUS:    Log("msg WM_SETFOCUS hwnd %p", c->hwnd); break;
        case WM_KILLFOCUS:   Log("msg WM_KILLFOCUS hwnd %p -> %p", c->hwnd, (void*)c->wParam); break;
        case WM_CAPTURECHANGED: Log("msg WM_CAPTURECHANGED hwnd %p", c->hwnd); break;
        }
    }
    return CallNextHookEx(g_callHook, code, wp, lp);
}

void InstallInputDiag(HWND hwnd)
{
    if (!g_cfg.diagInput || !hwnd || g_msgHook) return;
    DWORD tid = GetWindowThreadProcessId(hwnd, nullptr);
    g_msgHook = SetWindowsHookExW(WH_GETMESSAGE, GetMsgHook, g_self, tid);
    g_callHook = SetWindowsHookExW(WH_CALLWNDPROC, CallWndHook, g_self, tid);
    Log("input diag hooks on window thread %lu (present thread %lu): %p %p", tid, GetCurrentThreadId(), g_msgHook, g_callHook);
}

void InputDiagTick(HWND hwnd)
{
    if (!g_cfg.diagInput) return;
    static ULONGLONG last = 0;
    ULONGLONG now = GetTickCount64();
    if (now - last < 2000) return;
    last = now;
    static int ticks = 0;
    if (++ticks == 6) LogWndProc("~10s after weaving started", hwnd);
    long k = g_nKey.exchange(0), m = g_nMouse.exchange(0), r = g_nRawInput.exchange(0);
    bool asyncDown = false;
    for (int vk = 8; vk < 256 && !asyncDown; vk++) asyncDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
    if (k || m || r || asyncDown)
        Log("input last 2s: key msgs %ld, mouse msgs %ld, WM_INPUT %ld | a key is down now: %d | foreground=%d",
            k, m, r, asyncDown, GetForegroundWindow() == hwnd);
}

bool EnsureWeaver(IDXGISwapChain* sc, ScState& s)
{
    if (s.weaver) return true;
    if (!SrEnsureContext()) return false;
    DXGI_SWAP_CHAIN_DESC d = {};
    sc->GetDesc(&d);
    s.hwnd = d.OutputWindow ? d.OutputWindow : GetForegroundWindow();
    WeaverErrorCode code = WeaverErrorCode::WeaverSuccess;
    LogWndProc("before CreateDX11Weaver", s.hwnd);
    InstallInputDiag(s.hwnd);
    try {
        code = SR::CreateDX11Weaver(g_sr.ctx, s.ctx, g_cfg.weaverHwnd ? s.hwnd : nullptr, &s.weaver);
    } catch (const std::exception& e) {
        Log("CreateDX11Weaver threw: %s", e.what()); s.weaver = nullptr;
    } catch (...) {
        Log("CreateDX11Weaver threw (unknown)"); s.weaver = nullptr;
    }
    if (code != WeaverErrorCode::WeaverSuccess || !s.weaver) {
        Log("CreateDX11Weaver failed (code %d) - weaving disabled for swapchain %p", (int)code, sc);
        s.weaver = nullptr;
        s.failed = true;
        return false;
    }
    try {
        s.weaver->setLatencyInFrames((uint64_t)g_cfg.latencyFrames);
        s.weaver->setContext(s.ctx);
    } catch (...) {}
    Log("DX11 weaver created for swapchain %p (hwnd %p, %u buffers, swap effect %d, windowed %d)",
        sc, s.hwnd, d.BufferCount, (int)d.SwapEffect, d.Windowed);
    LogWndProc("after CreateDX11Weaver", s.hwnd);
    SrFinishInit();
    LogWndProc("after SR initialize", s.hwnd);
    SrSetLens(g_cfg.lens && g_cfg.weave);
    return true;
}

bool EnsureObjects(ScState& s, ID3D11Texture2D* bb, const D3D11_TEXTURE2D_DESC& bd)
{
    DXGI_FORMAT vf = ViewFormat(bd.Format, g_cfg.srgb);
    if (s.tex && s.w == bd.Width && s.h == bd.Height && s.fmt == vf && s.bbTex == bb) return true;
    if (s.tex && s.w == bd.Width && s.h == bd.Height && s.fmt == vf) {
        // same size, different back buffer (flip model rotates buffers): only the RTV is per buffer
        SafeRelease(s.bbRtv);
    } else {
        ReleaseSizeObjects(s);
        D3D11_TEXTURE2D_DESC td = bd;
        td.Format = TypelessOf(bd.Format);   // typeless: UNORM and UNORM_SRGB views are both legal; CopyResource from the typed back buffer is too
        td.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        td.MiscFlags = 0; td.CPUAccessFlags = 0; td.Usage = D3D11_USAGE_DEFAULT;
        td.MipLevels = 1; td.ArraySize = 1; td.SampleDesc.Count = 1; td.SampleDesc.Quality = 0;
        HRESULT hr = s.dev->CreateTexture2D(&td, nullptr, &s.tex);
        if (FAILED(hr)) {
            if (!s.failed) Log("CreateTexture2D %ux%u fmt %d (back buffer fmt %d) failed 0x%08lx - weaving disabled for this swap chain", bd.Width, bd.Height, (int)td.Format, (int)bd.Format, hr);
            s.failed = true; return false;
        }
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
        sd.Format = vf; sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D; sd.Texture2D.MipLevels = 1;
        hr = s.dev->CreateShaderResourceView(s.tex, &sd, &s.srv);
        if (FAILED(hr)) {
            if (!s.failed) Log("CreateShaderResourceView (view fmt %d on texture fmt %d, back buffer fmt %d) failed 0x%08lx - weaving disabled for this swap chain", (int)vf, (int)td.Format, (int)bd.Format, hr);
            ReleaseSizeObjects(s); s.failed = true; return false;
        }
        D3D11_RENDER_TARGET_VIEW_DESC rd = {};
        rd.Format = vf; rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        s.dev->CreateRenderTargetView(s.tex, &rd, &s.texRtv);
        s.w = bd.Width; s.h = bd.Height; s.fmt = vf;
        Log("SBS texture %ux%u fmt %d, view fmt %d (back buffer fmt %d, msaa %u)", s.w, s.h, (int)td.Format, (int)vf, (int)bd.Format, bd.SampleDesc.Count);
    }
    // Output view with the same sRGB-ness as the input view, so the weaver decodes and encodes
    // gamma the same number of times (an sRGB back buffer with a UNORM input view and an sRGB
    // output view double-encoded gamma: picture too bright, RFG Re-MARS-tered). Swap chain back
    // buffers accept the sRGB/non-sRGB sibling view; in auto mode vf is the back buffer's own
    // format anyway. If the sibling is refused, fall back to the exact back buffer format.
    D3D11_RENDER_TARGET_VIEW_DESC rd = {};
    rd.Format = vf;
    rd.ViewDimension = bd.SampleDesc.Count > 1 ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;
    HRESULT hr = s.dev->CreateRenderTargetView(bb, &rd, &s.bbRtv);
    if (FAILED(hr) && vf != bd.Format) {
        rd.Format = bd.Format;
        hr = s.dev->CreateRenderTargetView(bb, &rd, &s.bbRtv);
        if (SUCCEEDED(hr)) Log("WARNING: back buffer RTV fmt %d != input view fmt %d - gamma may be off", (int)bd.Format, (int)vf);
    }
    if (FAILED(hr)) {
        if (!s.failed) Log("CreateRenderTargetView(back buffer fmt %d, view fmt %d) failed 0x%08lx - weaving disabled for this swap chain", (int)bd.Format, (int)rd.Format, hr);
        ReleaseSizeObjects(s); s.failed = true; return false;
    }
    s.bbTex = bb;
    return true;
}

void Weave(IDXGISwapChain* sc)
{
    ScState* sp;
    {
        std::lock_guard<std::mutex> lk(g_scMutex);
        sp = &g_scs[sc];
    }
    ScState& s = *sp;
    InputDiagTick(s.hwnd);
    if (PollHotkeys()) SrSetLens(g_cfg.lens && g_cfg.weave);
    if (!g_cfg.weave) return;

    // The map is keyed by pointer; a new swap chain can reuse a freed one's address.
    ID3D11Device* dev = nullptr;
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&dev)) || !dev) {
        if (!s.failed) { s.failed = true; Log("swapchain %p has no D3D11 device - skipped", sc); }
        return;
    }
    if (s.dev != dev) {
        if (s.dev) Log("swapchain %p now belongs to another device - resetting", sc);
        ResetState(s);
        s.dev = dev;              // keeps the reference from GetDevice
        s.dev->GetImmediateContext(&s.ctx);
    } else {
        dev->Release();
    }
    if (s.failed) return;

    ID3D11Texture2D* bb = nullptr;
    if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb)) || !bb) return;
    D3D11_TEXTURE2D_DESC bd = {};
    bb->GetDesc(&bd);
    if (bd.Width < 2) { bb->Release(); return; }
    if (!EnsureWeaver(sc, s) || !EnsureObjects(s, bb, bd)) { bb->Release(); return; }

    if (!s.sizeWarned) {
        RECT cr = {};
        if (s.hwnd && GetClientRect(s.hwnd, &cr) && ((UINT)cr.right != bd.Width || (UINT)cr.bottom != bd.Height)) {
            Log("WARNING: back buffer %ux%u differs from window client %ldx%ld - the image gets scaled after weaving "
                "and the lenticular pattern will not line up. Use borderless/fullscreen at the panel's native resolution.",
                bd.Width, bd.Height, cr.right, cr.bottom);
        }
        s.sizeWarned = true;
    }

    const UINT w = s.w, h = s.h, hw = w / 2;
    ID3D11DeviceContext* c = s.ctx;
    if (g_cfg.test && s.texRtv) {
        ID3D11DeviceContext1* c1 = nullptr;
        if (SUCCEEDED(c->QueryInterface(__uuidof(ID3D11DeviceContext1), (void**)&c1)) && c1) {
            const float red[4] = { 0.8f, 0, 0, 1 }, blue[4] = { 0, 0, 0.8f, 1 };
            D3D11_RECT L = { 0, 0, (LONG)hw, (LONG)h }, R = { (LONG)hw, 0, (LONG)w, (LONG)h };
            c1->ClearView(s.texRtv, red, &L, 1);
            c1->ClearView(s.texRtv, blue, &R, 1);
            c1->Release();
        }
    } else if (bd.SampleDesc.Count > 1) {
        c->ResolveSubresource(s.tex, 0, bb, 0, s.fmt);
    } else if (g_cfg.swapEyes) {
        D3D11_BOX L = { 0, 0, 0, hw, h, 1 }, R = { hw, 0, 0, w, h, 1 };
        c->CopySubresourceRegion(s.tex, 0, hw, 0, 0, bb, 0, &L);
        c->CopySubresourceRegion(s.tex, 0, 0, 0, 0, bb, 0, &R);
    } else {
        c->CopyResource(s.tex, bb);
    }
    if (!s.inputBound) {
        try { s.weaver->setInputViewTexture(s.srv, (int)hw, (int)h, s.fmt); } catch (...) {}
        s.inputBound = true;
    }

    SavedState st;
    st.Capture(c);
    ID3D11RenderTargetView* rt = s.bbRtv;
    c->OMSetRenderTargets(1, &rt, nullptr);
    D3D11_VIEWPORT vp = { 0, 0, (float)w, (float)h, 0, 1 };
    c->RSSetViewports(1, &vp);
    D3D11_RECT full = { 0, 0, (LONG)w, (LONG)h };
    c->RSSetScissorRects(1, &full);
    try {
        s.weaver->weave();
    } catch (const std::exception& e) {
        Log("weave() threw: %s - weaving disabled", e.what()); s.failed = true;
    } catch (...) {
        Log("weave() threw (unknown) - weaving disabled"); s.failed = true;
    }
    st.Restore(c);
    bb->Release();
    if (++s.frames == 1) Log("first weaved frame: %ux%u (each eye %ux%u)", w, h, hw, h);
}

void OnPresent(IDXGISwapChain* sc, UINT flags)
{
    if (t_inPresent || !sc || (flags & DXGI_PRESENT_TEST)) return;
    t_inPresent = true;
    Weave(sc);
    t_inPresent = false;
}

HRESULT STDMETHODCALLTYPE Hook_Present(IDXGISwapChain* sc, UINT sync, UINT flags)
{
    OnPresent(sc, flags);
    return g_origPresent(sc, sync, flags);
}

HRESULT STDMETHODCALLTYPE Hook_Present1(IDXGISwapChain1* sc, UINT sync, UINT flags, const DXGI_PRESENT_PARAMETERS* pp)
{
    OnPresent(sc, flags);
    return g_origPresent1(sc, sync, flags, pp);
}

HRESULT STDMETHODCALLTYPE Hook_ResizeBuffers(IDXGISwapChain* sc, UINT n, UINT w, UINT h, DXGI_FORMAT f, UINT flags)
{
    {
        // the back buffer RTV holds a reference that would make ResizeBuffers fail
        std::lock_guard<std::mutex> lk(g_scMutex);
        auto it = g_scs.find(sc);
        if (it != g_scs.end()) { ReleaseSizeObjects(it->second); it->second.sizeWarned = false; it->second.failed = false; Log("ResizeBuffers %ux%u fmt %d", w, h, (int)f); }
    }
    return g_origResizeBuffers(sc, n, w, h, f, flags);
}

// ---- hook installation ---------------------------------------------------------------------

bool ReadPtr(const void* at, void** out)
{
    SIZE_T n = 0;
    return ReadProcessMemory(GetCurrentProcess(), at, out, sizeof(void*), &n) && n == sizeof(void*);
}

// geo-11 (3DMigoto) wraps every swap chain - even one made directly with System32 d3d11 - in its
// own object whose vtable lives in geo-11's d3d11.dll. The real IDXGISwapChain is one of the
// wrapper's fields: the first object whose vtable mostly points into System32 dxgi.dll and that
// answers QueryInterface(IDXGISwapChain).
IDXGISwapChain* FindRealSwapChain(IDXGISwapChain* sc)
{
    void** vt = nullptr;
    if (!ReadPtr(sc, (void**)&vt)) return nullptr;
    if (AddrInModule(vt[8], g_dxgi)) { sc->AddRef(); return sc; }
    for (int i = 1; i < 256; i++) {
        void* cand = nullptr;
        const ULONG_PTR align = sizeof(void*) - 1;   // 7 on x64, 3 on x86
        if (!ReadPtr((void**)sc + i, &cand) || !cand || cand == sc || ((ULONG_PTR)cand & align)) continue;
        void** cvt = nullptr;
        if (!ReadPtr(cand, (void**)&cvt) || !cvt || ((ULONG_PTR)cvt & align)) continue;
        int hits = 0;
        for (int k = 0; k < 18; k++) {
            void* f = nullptr;
            if (!ReadPtr(&cvt[k], &f)) { hits = -1; break; }
            if (f && AddrInModule(f, g_dxgi)) hits++;
        }
        if (hits < 12) continue;
        IDXGISwapChain* real = nullptr;
        if (SUCCEEDED(((IUnknown*)cand)->QueryInterface(__uuidof(IDXGISwapChain), (void**)&real)) && real) {
            Log("unwrapped: field %d of wrapper %p -> real swap chain %p (%d/18 slots in dxgi.dll)", i, (void*)sc, cand, hits);
            return real;
        }
    }
    return nullptr;
}

void HookOne(const char* name, void** vt, int index, void* detour, void** orig)
{
    void* target = vt[index];
    if (!AddrInModule(target, g_dxgi)) {
        Log("%s: slot %d -> %p is in %s, not System32 dxgi.dll - not hooked", name, index, target, ModuleOf(target).c_str());
        return;
    }
    MH_STATUS st = MH_CreateHook(target, detour, orig);
    Log("hook %s @ %p: %s", name, target, MH_StatusToString(st));
}

// No throw-away device/swap chain/window: geo-11 wraps and subclasses whatever it sees, and a
// temp window of ours replaced the game window's procedure in geo-11 - keyboard, mouse and
// controller stopped reaching the game. Instead hook the real factory's CreateSwapChain /
// CreateSwapChainForHwnd (a factory needs no window) and take the Present address from the
// game's own swap chain the moment the real dxgi creates it.
typedef HRESULT(STDMETHODCALLTYPE* PFN_CreateSwapChain)(IDXGIFactory*, IUnknown*, DXGI_SWAP_CHAIN_DESC*, IDXGISwapChain**);
typedef HRESULT(STDMETHODCALLTYPE* PFN_CreateSwapChainForHwnd)(IDXGIFactory2*, IUnknown*, HWND, const DXGI_SWAP_CHAIN_DESC1*,
                                                               const DXGI_SWAP_CHAIN_FULLSCREEN_DESC*, IDXGIOutput*, IDXGISwapChain1**);
PFN_CreateSwapChain g_origCreateSwapChain = nullptr;
PFN_CreateSwapChainForHwnd g_origCreateSwapChainForHwnd = nullptr;
std::once_flag g_presentHookOnce;

void OnSwapChainCreated(IDXGISwapChain* sc)
{
    std::call_once(g_presentHookOnce, [sc] {
        IDXGISwapChain* real = FindRealSwapChain(sc);
        if (!real) { Log("swap chain %p: no real dxgi swap chain found - no Present hook", (void*)sc); return; }
        void** vt = *(void***)real;
        HookOne("IDXGISwapChain::Present", vt, 8, (void*)&Hook_Present, (void**)&g_origPresent);
        HookOne("IDXGISwapChain::ResizeBuffers", vt, 13, (void*)&Hook_ResizeBuffers, (void**)&g_origResizeBuffers);
        IDXGISwapChain1* sc1 = nullptr;
        if (SUCCEEDED(real->QueryInterface(__uuidof(IDXGISwapChain1), (void**)&sc1)) && sc1) {
            HookOne("IDXGISwapChain1::Present1", *(void***)sc1, 22, (void*)&Hook_Present1, (void**)&g_origPresent1);
            sc1->Release();
        }
        real->Release();
        Log("MH_EnableHook (present): %s", MH_StatusToString(MH_EnableHook(MH_ALL_HOOKS)));
    });
}

HRESULT STDMETHODCALLTYPE Hook_CreateSwapChain(IDXGIFactory* f, IUnknown* dev, DXGI_SWAP_CHAIN_DESC* d, IDXGISwapChain** out)
{
    HRESULT hr = g_origCreateSwapChain(f, dev, d, out);
    if (SUCCEEDED(hr) && out && *out) {
        Log("CreateSwapChain -> %p (%ux%u, hwnd %p)", (void*)*out, d ? d->BufferDesc.Width : 0, d ? d->BufferDesc.Height : 0, d ? d->OutputWindow : nullptr);
        OnSwapChainCreated(*out);
    }
    return hr;
}

HRESULT STDMETHODCALLTYPE Hook_CreateSwapChainForHwnd(IDXGIFactory2* f, IUnknown* dev, HWND hwnd, const DXGI_SWAP_CHAIN_DESC1* d,
                                                      const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* fs, IDXGIOutput* o, IDXGISwapChain1** out)
{
    HRESULT hr = g_origCreateSwapChainForHwnd(f, dev, hwnd, d, fs, o, out);
    if (SUCCEEDED(hr) && out && *out) {
        Log("CreateSwapChainForHwnd -> %p (%ux%u, hwnd %p)", (void*)*out, d ? d->Width : 0, d ? d->Height : 0, hwnd);
        OnSwapChainCreated(*out);
    }
    return hr;
}

std::once_flag g_factoryHookOnce;

// Called from the first dxgi export call, before anyone can have created a swap chain.
void StartHooks()
{
    std::call_once(g_factoryHookOnce, [] {
        LoadReal();
        auto create1 = RealDxgi<HRESULT(WINAPI*)(REFIID, void**)>("CreateDXGIFactory1");
        IDXGIFactory2* f = nullptr;
        if (!create1 || FAILED(create1(__uuidof(IDXGIFactory2), (void**)&f)) || !f) { Log("real IDXGIFactory2 not available - no hooks"); return; }
        MH_Initialize();
        void** vt = *(void***)f;
        HookOne("IDXGIFactory::CreateSwapChain", vt, 10, (void*)&Hook_CreateSwapChain, (void**)&g_origCreateSwapChain);
        HookOne("IDXGIFactory2::CreateSwapChainForHwnd", vt, 15, (void*)&Hook_CreateSwapChainForHwnd, (void**)&g_origCreateSwapChainForHwnd);
        f->Release();
        Log("MH_EnableHook (factory): %s", MH_StatusToString(MH_EnableHook(MH_ALL_HOOKS)));
    });
}

}   // namespace

extern "C" const PfnDliHook __pfnDliNotifyHook2 = DliHook;

// ---- dxgi.dll exports ----------------------------------------------------------------------

extern "C" {

HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** out)
{
    StartHooks();
    auto f = RealDxgi<HRESULT(WINAPI*)(REFIID, void**)>("CreateDXGIFactory");
    return f ? f(riid, out) : E_FAIL;
}
HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** out)
{
    StartHooks();
    auto f = RealDxgi<HRESULT(WINAPI*)(REFIID, void**)>("CreateDXGIFactory1");
    return f ? f(riid, out) : E_FAIL;
}
HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID riid, void** out)
{
    StartHooks();
    auto f = RealDxgi<HRESULT(WINAPI*)(UINT, REFIID, void**)>("CreateDXGIFactory2");
    return f ? f(flags, riid, out) : E_FAIL;
}
HRESULT WINAPI DXGIDeclareAdapterRemovalSupport(void)
{
    auto f = RealDxgi<HRESULT(WINAPI*)(void)>("DXGIDeclareAdapterRemovalSupport");
    return f ? f() : E_FAIL;
}
HRESULT WINAPI DXGIGetDebugInterface1(UINT flags, REFIID riid, void** out)
{
    auto f = RealDxgi<HRESULT(WINAPI*)(UINT, REFIID, void**)>("DXGIGetDebugInterface1");
    return f ? f(flags, riid, out) : E_FAIL;
}

// The remaining exports are plain jumps (dxgi_forward.asm); this resolves their targets.
void* SRW_ResolveDxgi(int index)
{
    static const char* const kNames[] = {
        "ApplyCompatResolutionQuirking", "CompatString", "CompatValue",
        "DXGID3D10CreateDevice", "DXGID3D10CreateLayeredDevice", "DXGID3D10GetLayeredDeviceSize", "DXGID3D10RegisterLayers",
        "DXGIDisableVBlankVirtualization", "DXGIDumpJournal", "DXGIReportAdapterConfiguration",
        "PIXBeginCapture", "PIXEndCapture", "PIXGetCaptureState", "SetAppCompatStringPointer", "UpdateHMDEmulationStatus",
    };
    static void* cache[sizeof kNames / sizeof kNames[0]] = {};
    if (index < 0 || index >= (int)(sizeof kNames / sizeof kNames[0])) return nullptr;
    if (!cache[index]) cache[index] = (void*)RealDxgi<FARPROC>(kNames[index]);
    return cache[index];
}

}   // extern "C"

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = inst;
        DisableThreadLibraryCalls(inst);
        LoadConfig();
    }
    return TRUE;
}
