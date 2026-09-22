@echo off
rem Allows the project build to be launched from a standard Command Prompt or
rem a PowerShell session whose policy blocks local .ps1 files.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build.ps1" %*
exit /b %ERRORLEVEL%
