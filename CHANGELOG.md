# Changelog

## [Unreleased]

## [1.1.1]

- Accept an `https://` `BRIDGE_URL`: the server certificate is verified against the embedded ISRG Root X1 (`RootCA.h`), the clock is synced over NTP, and HTTPS requests wait up to 4 s. Plain `http` URLs behave as before.

## [1.1.0]

- Full-screen blinking "NEEDS YOU" alert with project and wait time, repeated every 2 minutes until acknowledged.
- Celebration screen when a turn of 5 minutes or more finishes.
- Limits screen with 5-hour and weekly bars for Claude Code, Codex and Antigravity, with time to reset.
- Meeting mode (manual toggle on the Focus screen, automatic while the focus timer runs): no overlays, dimmed display.
- Idle robot after 15 quiet minutes, moving to avoid AMOLED burn-in.
- Serial log markers for screen, alert, celebration, limits and meeting changes.
- Documentation: add requirements, `config.h` reference, troubleshooting and `docs/SCREENS.md`; add `docs/MAINTAINING.md`.
- Home now opens Limits instead of Refresh; Refresh remains on the Sessions screen.

## [1.0.0]

- Initial standalone firmware release.
