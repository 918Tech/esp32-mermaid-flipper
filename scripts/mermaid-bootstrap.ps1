$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

py -3 -m venv .venv-mermaid
$Python = Join-Path (Get-Location) ".venv-mermaid\Scripts\python.exe"
$Ufbt = Join-Path (Get-Location) ".venv-mermaid\Scripts\ufbt.exe"
& $Python -m pip install --upgrade pip
& $Python -m pip install -r requirements-mvp.txt
$env:UFBT_HOME = Join-Path (Get-Location) ".ufbt"
& $Ufbt update --channel=release
Write-Output "MERMAID_MVP_BOOTSTRAP_OK"
