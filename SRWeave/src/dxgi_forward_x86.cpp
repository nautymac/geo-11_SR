// x86 counterpart of dxgi_forward.asm: jump stubs for the dxgi.dll exports whose signatures
// SRWeave does not model. Each stub asks SRW_ResolveDxgi(index) for the SysWOW64 function and
// jumps to it with the caller's stack untouched (the arguments stay where the caller pushed
// them; eax/ecx/edx are caller-saved under both stdcall and cdecl, so nothing else needs saving).
#if defined(_M_IX86)
#include <windows.h>

extern "C" void* SRW_ResolveDxgi(int index);

#define SRW_FWD(name, idx)                                  \
    extern "C" __declspec(naked) void name()                \
    {                                                       \
        __asm { push idx }                                  \
        __asm { call SRW_ResolveDxgi }                      \
        __asm { add esp, 4 }                                \
        __asm { test eax, eax }                             \
        __asm { jz fail }                                   \
        __asm { jmp eax }                                   \
        __asm { fail: mov eax, 0x80004005 } /* E_FAIL */    \
        __asm { ret }                                       \
    }

SRW_FWD(ApplyCompatResolutionQuirking, 0)
SRW_FWD(CompatString, 1)
SRW_FWD(CompatValue, 2)
SRW_FWD(DXGID3D10CreateDevice, 3)
SRW_FWD(DXGID3D10CreateLayeredDevice, 4)
SRW_FWD(DXGID3D10GetLayeredDeviceSize, 5)
SRW_FWD(DXGID3D10RegisterLayers, 6)
SRW_FWD(DXGIDisableVBlankVirtualization, 7)
SRW_FWD(DXGIDumpJournal, 8)
SRW_FWD(DXGIReportAdapterConfiguration, 9)
SRW_FWD(PIXBeginCapture, 10)
SRW_FWD(PIXEndCapture, 11)
SRW_FWD(PIXGetCaptureState, 12)
SRW_FWD(SetAppCompatStringPointer, 13)
SRW_FWD(UpdateHMDEmulationStatus, 14)
#endif
