# AI Watch firmware

Install Arduino CLI, then run `pwsh -File scripts/setup-arduino.ps1` to install Espressif core 3.3.13 and ArduinoJson 7.4.3. The required Waveshare V2 libraries are pinned by the committed vendor snapshot. Run `pwsh -File scripts/build-firmware.ps1`.

Copy firmware/AIWatch/config.example.h to config.h alongside the sketch. Set the 2.4 GHz Wi-Fi credentials, shared API token, and status URL at the private server address. Upload on Windows with `pwsh -File scripts/deploy-firmware.ps1 -Port COM3`, substituting the detected port. Use Serial Monitor at 115200 baud. The offline preview can compile without a local config.

## Security

CI builds firmware and checks Arduino-ESP32 against OSV by Git commit. Review the printed inventory for vendored libraries without reliable advisory mapping. Apply firmware dependency changes through reviewed pull requests.

Related repositories: [server](https://github.com/athegreat90/ai-watch-server) [hooks](https://github.com/athegreat90/ai-watch-hooks)
