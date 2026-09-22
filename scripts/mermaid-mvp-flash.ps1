$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")
$Python = Join-Path (Get-Location) ".venv-mermaid\Scripts\python.exe"
if (-not (Test-Path $Python)) {
    throw "Run scripts\mermaid-bootstrap.ps1 first."
}
$env:Path = (Join-Path (Get-Location) ".venv-mermaid\Scripts") + ";" + $env:Path
$env:UFBT_HOME = Join-Path (Get-Location) ".ufbt"
& $Python tools\mermaid_flasher.py provision
exit $LASTEXITCODE
