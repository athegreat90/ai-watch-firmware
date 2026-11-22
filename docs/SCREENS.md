# Screens and behaviors

| Screen or behavior | Trigger | What it does |
|---|---|---|
| NEEDS YOU | A session reports it needs attention | Takes over the screen with a blinking border, project and wait time. Repeats every 2 minutes until you tap to dismiss it or acknowledge it in the session detail |
| Task done | A turn of 5 minutes or more finishes | A 4-second celebration |
| Limits | Opened from Home | 5-hour and weekly bars for Claude Code, Codex and Antigravity, with time to reset |
| Focus | Tap **Focus** | 25-minute timer: start, pause, resume, reset. Runs without the server |
| Meeting mode | Toggle on the Focus screen, or automatically while the focus timer runs | No overlays, display dimmed, small "Meeting" badge (amber if something needs you) |
| Idle robot | 15 minutes without activity | A robot that moves to avoid AMOLED burn-in. Any touch or new event wakes it |

## Serial markers

At 115200 baud the watch prints timestamped lines: `Screen:`, `Alert:`, `Celebrate:`, `Limits:`, `Meeting:` and `Overlay:`. Use them to confirm which screen and overlay is active after a build.
