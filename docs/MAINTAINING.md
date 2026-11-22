# Maintaining

## Releases

A successful code-changing push to main produces one new patch release. Reruns reuse its version. Documentation-only changes do not need a new version. See [GitHub Releases](https://github.com/athegreat90/ai-watch-firmware/releases).

## Security automation

CI builds firmware and checks Arduino-ESP32 against OSV by Git commit. Review the printed inventory for vendored libraries without reliable advisory mapping. Daily remediation opens a tracking issue for indexed Arduino-ESP32 advisories. It proposes a same-major upstream SemVer fix when the patched core compiles and clears the scan. Set repository variable SECURITY_APP_ID and secret SECURITY_APP_PRIVATE_KEY using a GitHub App installed on this repository with Contents, Pull requests and Issues write permissions. Review unindexed dependencies manually.

## Notes

`MEMORY.md` records maintainer decisions (not user documentation).
