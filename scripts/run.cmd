@echo off
rem Starts Voyager-2 with a process-local PowerShell policy bypass.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0run.ps1" %*
exit /b %ERRORLEVEL%
