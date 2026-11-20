param([string]$WorkDir = $PSScriptRoot)
$ErrorActionPreference = "Stop"
. (Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) '_shared\driver.ps1')
Invoke-RioSuite -WorkDir $WorkDir