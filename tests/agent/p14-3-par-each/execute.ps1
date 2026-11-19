# 数据驱动套件：判定逻辑只有一份，在 tests/_shared/driver.ps1
param([string]$WorkDir = $PSScriptRoot)
$ErrorActionPreference = "Stop"

. (Join-Path (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent) '_shared\driver.ps1')
Invoke-RioSuite -WorkDir $WorkDir
