@echo off
cd %~dp0

REM Create the .vscode directory if it doesn't exist
if not exist ".vscode" mkdir ".vscode"

REM Move the configuration files into the .vscode directory
move "settings.json" ".vscode\settings.json"
move "tasks.json" ".vscode\tasks.json"
move "launch.json" ".vscode\launch.json"

REM Grant read and write permissions to the current user for the .vscode directory and its contents
icacls ".vscode" /grant "%username%:R,W"
icacls ".vscode\*" /grant "%username%:R,W"

echo Permissions updated and files moved.
