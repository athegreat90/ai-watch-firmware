param(
    [ValidateSet('Watch','Hello','Revision')][string]$Target = 'Watch',
    [Parameter(Mandatory)][string]$Port
)
. "$PSScriptRoot/common.ps1"
Assert-Port $Port
if ($Target -eq 'Watch' -and !(Test-Path "$Root/firmware/AIWatch/config.h")) {
    throw 'Configure the Watch before uploading.'
}
& "$PSScriptRoot/build-firmware.ps1" -Target $Target
$cli = Get-ArduinoCli
Invoke-Checked $cli @('upload','--verbose','--fqbn',$Fqbn,'--port',$Port,
    '--input-dir',"$Root/.build/$Target",(Get-Sketch $Target))
