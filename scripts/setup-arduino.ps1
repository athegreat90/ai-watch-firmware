$ErrorActionPreference = "Stop"
arduino-cli core install esp32:esp32@3.3.13 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
if ($LASTEXITCODE) { exit $LASTEXITCODE }
arduino-cli lib install ArduinoJson@7.4.3
exit $LASTEXITCODE
