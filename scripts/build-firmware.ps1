param([ValidateSet('Watch','Hello','Revision')][string]$Target = 'Watch')
. "$PSScriptRoot/common.ps1"
$cli = Get-ArduinoCli
$build = Join-Path $Root ".build/$Target"
$arguments = @('compile','--fqbn',$Fqbn,'--build-path',$build,'--libraries',"$Vendor/libraries")
$json = Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Arduino/libraries/ArduinoJson'
if (Test-Path -LiteralPath $json) { $arguments += @('--library',$json) }
Invoke-Checked $cli ($arguments + @((Get-Sketch $Target)))
