param([ValidateSet('Watch','Hello','Revision')][string]$Target = 'Watch')
. "$PSScriptRoot/common.ps1"
$cli = Get-ArduinoCli
$build = Join-Path $Root ".build/$Target"
$arguments = @('compile','--fqbn',$Fqbn,'--build-path',$build,'--libraries',"$Vendor/libraries")
Invoke-Checked $cli ($arguments + @((Get-Sketch $Target)))
