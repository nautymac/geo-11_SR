cmake_minimum_required(VERSION 3.20)
# SRWeave for geo-11: dxgi.dll proxy that weaves geo-11's SBS output with the SR SDK right before
# the real IDXGISwapChain::Present. Builds for x64 (-A x64) and x86 (-A Win32); see build.ps1.
project(SRWeaveGeo11 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Static CRT: the DLL lands in arbitrary game folders.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(SRW_ARCH x64)
    set(SRW_SDK_DEFAULT "D:/OneDrive/3D/LeiaSR Runtime Installer(SDK)/simulatedreality-1.34.10-win64-Release (extracted SDK)")
    set(SRW_SR_SUFFIX "")
    enable_language(ASM_MASM)
else()
    set(SRW_ARCH x86)
    set(SRW_SDK_DEFAULT "D:/OneDrive/3D/LeiaSR Runtime Installer(SDK)/simulatedreality-1.34.10-win32-Release (extracted SDK)")
    set(SRW_SR_SUFFIX "32")   # SimulatedRealityCore32.dll etc.
endif()

set(SR_SDK "${SRW_SDK_DEFAULT}" CACHE PATH "extracted simulatedreality-<ver>-win64/win32-Release SDK for this architecture")
if(NOT EXISTS "${SR_SDK}/include/sr/weaver/dx11weaver.h")
    message(FATAL_ERROR "SR SDK not found at ${SR_SDK} (-DSR_SDK=<path>)")
endif()
if(NOT EXISTS "${SR_SDK}/lib/SimulatedRealityDirectX${SRW_SR_SUFFIX}.lib")
    message(FATAL_ERROR "SR SDK at ${SR_SDK} has no ${SRW_ARCH} libraries (SimulatedRealityDirectX${SRW_SR_SUFFIX}.lib)")
endif()
set(SRWEAVE_OUT_DIR "${CMAKE_SOURCE_DIR}/bin/${SRW_ARCH}" CACHE PATH "output folder")

add_library(minhook STATIC
    third_party/minhook/src/buffer.c
    third_party/minhook/src/hook.c
    third_party/minhook/src/trampoline.c
    third_party/minhook/src/hde/hde32.c
    third_party/minhook/src/hde/hde64.c)   # each hde file compiles to nothing on the other architecture
target_include_directories(minhook PUBLIC third_party/minhook/include)

if(SRW_ARCH STREQUAL x64)
    set(SRW_FORWARD src/dxgi_forward.asm)
    set(SRW_DEF src/dxgi.def)
else()
    set(SRW_FORWARD src/dxgi_forward_x86.cpp)
    set(SRW_DEF src/dxgi32.def)
endif()

add_library(SRWeaveGeo11 SHARED src/srweave_geo11.cpp ${SRW_FORWARD} ${SRW_DEF})
target_include_directories(SRWeaveGeo11 PRIVATE
    "${SR_SDK}/include"
    "${SR_SDK}/third_party/OpenCV/include"
    "${SR_SDK}/third_party/GLog/include")
# /wd4005: the SDK's IWeaverBase.h redefines WIN32_LEAN_AND_MEAN
target_compile_options(SRWeaveGeo11 PRIVATE $<$<COMPILE_LANGUAGE:CXX>:/EHsc /permissive- /utf-8 /W3 /wd4005>)
target_compile_definitions(SRWeaveGeo11 PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
# No dxgi/d3d11 import libs: this DLL *is* dxgi.dll; everything Direct3D comes from System32/SysWOW64 at runtime.
target_link_libraries(SRWeaveGeo11 PRIVATE minhook
    "${SR_SDK}/lib/SimulatedRealityCore${SRW_SR_SUFFIX}.lib"
    "${SR_SDK}/lib/SimulatedRealityDirectX${SRW_SR_SUFFIX}.lib"
    "${SR_SDK}/lib/SimulatedRealityDisplays${SRW_SR_SUFFIX}.lib"
    "${SR_SDK}/third_party/OpenCV/lib/${SRW_ARCH}/opencv_world343.lib"
    delayimp user32)
target_link_options(SRWeaveGeo11 PRIVATE
    /DELAYLOAD:SimulatedRealityCore${SRW_SR_SUFFIX}.dll
    /DELAYLOAD:SimulatedRealityDirectX${SRW_SR_SUFFIX}.dll
    /DELAYLOAD:SimulatedRealityDisplays${SRW_SR_SUFFIX}.dll
    /DELAYLOAD:opencv_world343.dll)
if(SRW_ARCH STREQUAL x86)
    # the SDK's 32-bit import libraries are not SAFESEH-registered; naked stubs need no SEH either
    target_link_options(SRWeaveGeo11 PRIVATE /SAFESEH:NO)
endif()
set_target_properties(SRWeaveGeo11 PROPERTIES OUTPUT_NAME dxgi PREFIX ""
    RUNTIME_OUTPUT_DIRECTORY "${SRWEAVE_OUT_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${SRWEAVE_OUT_DIR}")
