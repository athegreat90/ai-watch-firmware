# AI Watch firmware


## Build and upload

Deploy the [server](https://github.com/athegreat90/ai-watch-server) first. Clone this repository and install Arduino CLI and PowerShell 7. From the repository root, install the pinned ESP32 core and ArduinoJson:

```powershell
pwsh -File scripts/setup-arduino.ps1
pwsh -File scripts/build-firmware.ps1
arduino-cli board list
```

The first build runs as an offline preview. For live monitoring, copy `firmware/AIWatch/config.example.h` to ignored `firmware/AIWatch/config.h`. Set your 2.4 GHz Wi-Fi SSID and password, the server token, and a full status URL such as `http://192.168.1.50:8765/api/status`.

Use the serial port listed by Arduino CLI. For example, on Windows:

```powershell
pwsh -File scripts/deploy-firmware.ps1 -Port COM3
arduino-cli monitor -p COM3 --config baudrate=115200
```

On Linux or macOS, substitute the detected device path. In Arduino IDE, use ESP32S3 Dev Module at 240 MHz with 16 MB flash, OPI PSRAM, and hardware USB CDC enabled. This sketch targets the Waveshare V2 CO5300/CST820 hardware.

After reset, check that serial logs report display and touch initialization and that the dashboard shows **Backend online**. If the display needs an independent check, build and upload with `-Target Hello` to run the vendor example, then flash Watch again.

Tap **Sessions** to view agent activity and open a session. Use **Focus** to start, pause, resume, or reset the 25-minute timer. It runs without the server. Use **Refresh** to poll immediately and **Light** to cycle brightness.

## Releases

A successful code-changing push to main produces one new patch release. Reruns reuse its version. See [GitHub Releases](https://github.com/athegreat90/ai-watch-firmware/releases).

## Security

CI builds firmware and checks Arduino-ESP32 against OSV by Git commit. Review the printed inventory for vendored libraries without reliable advisory mapping. Daily remediation opens a tracking issue for indexed Arduino-ESP32 advisories. It proposes a same-major upstream SemVer fix when the patched core compiles and clears the scan. Set SECURITY_APP_ID and SECURITY_APP_PRIVATE_KEY as described in the server repository. Review unindexed dependencies manually.


Related repositories: [server](https://github.com/athegreat90/ai-watch-server) | [hooks](https://github.com/athegreat90/ai-watch-hooks)
