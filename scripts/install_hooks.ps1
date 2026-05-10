$ErrorActionPreference = "Stop"

git config core.hooksPath .githooks

Write-Host "Configured repository hooks path to .githooks"
Write-Host "Pre-commit and pre-push privacy scans are now enabled."
