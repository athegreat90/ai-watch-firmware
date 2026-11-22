# AI Watch firmware

Firmware for a Waveshare ESP32-S3 touch watch (CO5300 display, CST820 touch) that shows what your AI coding agents are doing: active sessions, "needs you" alerts, usage limits and a focus timer. It polls the [AI Watch server](https://github.com/athegreat90/ai-watch-server).

Related repositories: [server](https://github.com/athegreat90/ai-watch-server) | [hooks](https://github.com/athegreat90/ai-watch-hooks). This is firmware v1.1.0; use it with the 1.1.0 server.

## Requirements

- The Waveshare V2 board this sketch targets (ESP32-S3, 16 MB flash, OPI PSRAM, CO5300/CST820).
- A 2.4 GHz Wi-Fi network that can reach the server. The watch does not support 5 GHz.
- A running server, deployed first, with its token.
- [Arduino CLI](https://arduino.github.io/arduino-cli/) and PowerShell 7 (`pwsh`) for the scripts.
- A USB data cable.

## Quick start

From the repository root, install the pinned ESP32 core and ArduinoJson, then build and list serial ports:

```powershell
pwsh -File scripts/setup-arduino.ps1
pwsh -File scripts/build-firmware.ps1
arduino-cli board list
```

The first build runs as an offline preview. For live monitoring, copy `firmware/AIWatch/config.example.h` to the ignored `firmware/AIWatch/config.h` and edit it:

| Define | Meaning |
|---|---|
| `WIFI_SSID` | Your 2.4 GHz Wi-Fi name |
| `WIFI_PASSWORD` | Its password |
| `BRIDGE_URL` | Full status URL of the server, for example `http://192.168.1.50:8765/api/status` |
| `MONITOR_TOKEN` | The `token` from the server's `bridge/config.local.json` |

Never share `config.h`. Then upload with the port that Arduino CLI listed and open the serial monitor:

```powershell
pwsh -File scripts/deploy-firmware.ps1 -Port COM3
arduino-cli monitor -p COM3 --config baudrate=115200
```

On Linux or macOS use the detected device path instead of `COM3` (for example `/dev/ttyACM0` or `/dev/cu.usbmodem*`).

Arduino IDE settings:

| Setting | Value |
|---|---|
| Board | ESP32S3 Dev Module |
| CPU frequency | 240 MHz |
| Flash size | 16 MB |
| PSRAM | OPI PSRAM |
| USB CDC on boot | Enabled (hardware USB CDC) |

## Verify

After reset, the serial log should report display and touch initialization and the dashboard should show **Backend online**. If the display needs an independent check, build and upload with `-Target Hello` to run the vendor example, then flash Watch again.

## Using the watch

Tap **Sessions** to view agent activity and open a session. Use **Focus** to start, pause, resume or reset the 25-minute timer. It runs without the server. Use **Refresh** to poll immediately and **Light** to cycle brightness. Alerts, the celebration, the Limits screen, meeting mode and the idle robot are described in [docs/SCREENS.md](docs/SCREENS.md).

The serial monitor (115200) prints timestamped markers (`Screen:`, `Alert:`, `Celebrate:`, `Limits:`, `Meeting:`, `Overlay:`) that help when checking a build.

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| Shows "Backend: check token" | `MONITOR_TOKEN` differs from the server's token |
| Shows "Backend offline" | Wrong `BRIDGE_URL`, server not reachable from the Wi-Fi network, firewall, or 5 GHz-only Wi-Fi |
| Offline preview only | `firmware/AIWatch/config.h` is missing |
| Upload fails or port busy | Close the serial monitor and any other program using the port, then retry with the port from `arduino-cli board list` |
| Blank or unresponsive display | Check the serial log for display and touch initialization; run the `-Target Hello` check above |

## Releases and maintenance

See [GitHub Releases](https://github.com/athegreat90/ai-watch-firmware/releases) and the [changelog](CHANGELOG.md). CI, security automation and release rules are in [docs/MAINTAINING.md](docs/MAINTAINING.md).
