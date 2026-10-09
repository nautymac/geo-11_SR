; Jump stubs for the dxgi.dll exports whose signatures SRWeave does not model.
; Each one asks SRW_ResolveDxgi(index) for the System32 function and jumps to it with the
; caller's registers and stack untouched.
EXTERN SRW_ResolveDxgi:PROC

FWD MACRO name, idx
    LOCAL fail
name PROC
    push rcx
    push rdx
    push r8
    push r9
    sub rsp, 68h                 ; 32 shadow + 4 xmm saves, keeps rsp 16-aligned at the call
    movdqu [rsp+20h], xmm0
    movdqu [rsp+30h], xmm1
    movdqu [rsp+40h], xmm2
    movdqu [rsp+50h], xmm3
    mov ecx, idx
    call SRW_ResolveDxgi
    movdqu xmm0, [rsp+20h]
    movdqu xmm1, [rsp+30h]
    movdqu xmm2, [rsp+40h]
    movdqu xmm3, [rsp+50h]
    add rsp, 68h
    pop r9
    pop r8
    pop rdx
    pop rcx
    test rax, rax
    jz fail
    jmp rax
fail:
    mov eax, 80004005h           ; E_FAIL
    ret
name ENDP
ENDM

.code
FWD ApplyCompatResolutionQuirking, 0
FWD CompatString, 1
FWD CompatValue, 2
FWD DXGID3D10CreateDevice, 3
FWD DXGID3D10CreateLayeredDevice, 4
FWD DXGID3D10GetLayeredDeviceSize, 5
FWD DXGID3D10RegisterLayers, 6
FWD DXGIDisableVBlankVirtualization, 7
FWD DXGIDumpJournal, 8
FWD DXGIReportAdapterConfiguration, 9
FWD PIXBeginCapture, 10
FWD PIXEndCapture, 11
FWD PIXGetCaptureState, 12
FWD SetAppCompatStringPointer, 13
FWD UpdateHMDEmulationStatus, 14
END
