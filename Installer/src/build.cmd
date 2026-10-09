@echo off
:: Builds ..\Geo11SRInstaller.exe (WinForms, .NET Framework 4.x, no console) from Geo11SRInstaller.cs.
:: Uses the Roslyn csc bundled with Visual Studio if present, else the .NET Framework 4 csc (C# 5).
setlocal
set SRC=%~dp0Geo11SRInstaller.cs
set OUT=%~dp0..\Geo11SRInstaller.exe
set CSC=
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -property installationPath 2^>nul`) do set VS=%%i
if defined VS if exist "%VS%\MSBuild\Current\Bin\Roslyn\csc.exe" set CSC=%VS%\MSBuild\Current\Bin\Roslyn\csc.exe
if not defined CSC set CSC=%WINDIR%\Microsoft.NET\Framework64\v4.0.30319\csc.exe
echo csc: %CSC%
"%CSC%" /nologo /target:winexe /optimize+ /codepage:65001 /platform:anycpu /win32manifest:"%~dp0app.manifest" /win32icon:"%~dp0app.ico" /out:"%OUT%" /r:System.dll /r:System.Core.dll /r:System.Drawing.dll /r:System.Windows.Forms.dll /r:System.Web.Extensions.dll "%SRC%" "%~dp0Lang.cs" "%~dp0Ue4.cs" "%~dp0Launchers.cs"
if errorlevel 1 (echo BUILD FAILED & exit /b 1)
echo built: %OUT%
