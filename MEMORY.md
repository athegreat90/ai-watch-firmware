# Project memory

ESP32-S3 with 16 MB flash and 8 MB OPI PSRAM. CO5300 and CST820-compatible touch were verified on the V2 test examples. Use the touch initialization retry and keep the focus timer independent of Wi-Fi. The deployed combined project returned HTTP 200 from the watch. A fresh standalone build still requires verification.

Docs decisions (2026-11-21): doc changes are folded into 1.1.0 (no new version). Screen behaviors and timings moved to docs/SCREENS.md; maintainer and CI content to docs/MAINTAINING.md. Troubleshooting texts ("Backend: check token", "Backend offline") were taken from the server Linux guide and not re-checked against AIWatch.ino; Linux/macOS port names are examples. Each repo keeps its own docs with no cross-repo files. Always record decisions here.

HTTPS decision (2026-11-22, v1.1.1): BRIDGE_URL may be https. The root CA is pinned (ISRG Root X1, valid to 2035-06-04, SHA-256 96:BC:EC:06...08:C6) instead of setInsecure, so the token cannot be sniffed by a man in the middle. Update RootCA.h if the server's chain changes root. The clock is synced with configTime (status -3 until synced), and the https path uses 4 s timeouts because fetch() runs in its own task. Version 1.1.1 comes from the release automation (patch tag). Not yet flash-tested on the watch.
