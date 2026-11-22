#pragma once
// Copy to config.h and replace placeholders. DO NOT share config.h.
#define WIFI_SSID     "YOUR_2_4_GHZ_WIFI"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define BRIDGE_URL    "http://192.168.68.100:8765/api/status"
// Or an https server whose certificate chains to ISRG Root X1 (see RootCA.h):
// #define BRIDGE_URL "https://your-server.example.org/api/status"
#define MONITOR_TOKEN "REPLACE_WITH_TOKEN_FROM_bridge/config.local.json"
