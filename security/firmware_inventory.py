"""Check OSV's Git commit index for the pinned Arduino core."""
import json
import os
import re
import urllib.request
from pathlib import Path

setup = Path("scripts/setup-arduino.ps1").read_text()
version = setup.split("esp32:esp32@", 1)[1].split()[0]
url = "https://api.github.com/repos/espressif/arduino-esp32/commits/" + version
with urllib.request.urlopen(url, timeout=20) as response:
    commit = json.load(response)["sha"]
request = urllib.request.Request("https://api.osv.dev/v1/query",
    data=json.dumps({"commit": commit}).encode(), headers={"Content-Type": "application/json"})
with urllib.request.urlopen(request, timeout=20) as response:
    advisories = json.load(response).get("vulns", [])
for finding in advisories:
    print(f'::warning::Arduino-ESP32 {version}: {finding["id"]}')
for props in sorted(Path("vendor").rglob("library.properties")):
    identity = [line for line in props.read_text(encoding="utf-8-sig").splitlines()
                if line.startswith(("name=", "version="))]
    print("Manual advisory review required: " + ", ".join(identity))
print("Manual advisory review required: ArduinoJson 7.4.3 and bundled ESP-IDF libraries.")

if advisories and os.environ.get("GITHUB_OUTPUT"):
    with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
        output.write("advisories=" + ",".join(finding["id"] for finding in advisories) + "\n")
    versions = []
    for finding in advisories:
        fixes = [event["fixed"] for affected in finding.get("affected", [])
                 for item in affected.get("ranges", [])
                 if item.get("type") == "SEMVER"
                 for event in item.get("events", []) if "fixed" in event
                 and re.fullmatch(r"\d+\.\d+\.\d+", event["fixed"])]
        if not fixes:
            break
        versions.extend(fixes)
    else:
        current = tuple(map(int, version.split(".")))
        candidate = max(tuple(map(int, value.split("."))) for value in versions)
        if current < candidate and current[0] == candidate[0]:
            with open(os.environ["GITHUB_OUTPUT"], "a", encoding="utf-8") as output:
                output.write("candidate=" + ".".join(map(str, candidate)) + "\n")
if advisories and "--require-clear" in __import__("sys").argv:
    raise SystemExit("Arduino-ESP32 revision still has indexed vulnerabilities.")
