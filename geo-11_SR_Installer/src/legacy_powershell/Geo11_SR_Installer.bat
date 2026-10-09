@echo off
:: geo-11 SR installer GUI (Unity / plain geo-11, 32/64-bit, SR weaving on/off, stereo values, hotkeys)
powershell -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0Geo11SRInstaller.ps1"
if errorlevel 1 pause
