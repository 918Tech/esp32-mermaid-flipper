$ErrorActionPreference = "Stop"
Set-Location (Join-Path $PSScriptRoot "..")

if (-not (Get-Command codex -ErrorAction SilentlyContinue)) {
    throw "Codex CLI is not installed or not on PATH."
}

$Prompt = @"
Read AGENTS.md and follow it exactly. Provision the Mermaid MVP hardware connected to this machine. If the local Mermaid toolchain is missing, run scripts\mermaid-bootstrap.ps1. Then run the audited self-test, doctor, build/flash/verify path through scripts\mermaid-mvp-flash.ps1. You may repair only the non-destructive issues AGENTS.md explicitly allows, and after any repair rerun self-test and builds before another hardware write. Never use the prohibited recovery shortcuts. Do not report success unless MERMAID_MVP_PROVISIONING_OK is printed; otherwise stop with the exact observed failure and preserve dist\mvp-logs\inventory.json.
"@

& codex exec --sandbox workspace-write --ask-for-approval on-request -c approvals_reviewer=auto_review $Prompt
exit $LASTEXITCODE
