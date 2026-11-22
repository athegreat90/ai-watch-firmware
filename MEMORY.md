# Project memory

ESP32-S3 with 16 MB flash and 8 MB OPI PSRAM. CO5300 and CST820-compatible touch were verified on the V2 test examples. Use the touch initialization retry and keep the focus timer independent of Wi-Fi. The deployed combined project returned HTTP 200 from the watch. A fresh standalone build still requires verification.

Docs decisions (2026-11-21): doc changes are folded into 1.1.0 (no new version). Screen behaviors and timings moved to docs/SCREENS.md; maintainer and CI content to docs/MAINTAINING.md. Troubleshooting texts ("Backend: check token", "Backend offline") were taken from the server Linux guide and not re-checked against AIWatch.ino; Linux/macOS port names are examples. Each repo keeps its own docs with no cross-repo files. Always record decisions here.
