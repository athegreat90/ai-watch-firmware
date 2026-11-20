$ErrorActionPreference = 'Stop'
$Root = Split-Path $PSScriptRoot -Parent
$Fqbn = 'esp32:esp32:esp32s3:CPUFreq=240,FlashSize=16M,PSRAM=opi,CDCOnBoot=cdc,USBMode=hwcdc'
$Vendor = Join-Path $Root 'vendor/arduino-v2'
function Get-ArduinoCli {
    $cli = Get-Command arduino-cli -CommandType Application -ErrorAction SilentlyContinue
    if ($cli) { return $cli.Source }
    foreach ($path in @(
        "$env:ProgramFiles/Arduino CLI/arduino-cli.exe",
        "$env:LOCALAPPDATA/Programs/arduino-ide/resources/app/lib/backend/resources/arduino-cli.exe",
        "$env:ProgramFiles/Arduino IDE/resources/app/lib/backend/resources/arduino-cli.exe"
    )) { if (Test-Path -LiteralPath $path) { return $path } }
    throw 'Arduino CLI could not be found. Install Arduino IDE 2.x or Arduino CLI.'
}
function Invoke-Checked {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Command failed with exit code $LASTEXITCODE." }
}
function Get-Sketch([string]$Target) {
    switch ($Target) {
        'Hello' { Join-Path $Vendor 'examples/01_HelloWorld' }
        'Revision' { Join-Path $Root 'firmware/VerifyRevision' }
        'Watch' { Join-Path $Root 'firmware/AIWatch' }
    }
}
function Assert-Port([string]$Port) {
    if ($Port -notin [IO.Ports.SerialPort]::GetPortNames()) {
        throw "Port $Port is absent. Run scripts/diagnose-environment.ps1."
    }
}
