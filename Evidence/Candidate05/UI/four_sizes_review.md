# Candidate05 S4 UI review at four sizes

All 24 original PNGs were independently viewed. No blocking UI defect was observed in these six states at 1280×720, 1600×900, 1920×1080, and 2560×1080. Each packaged fixture recorded 49 checks, zero failures, a Quit request, normal shutdown and exit 0 (196 checks total).

Source: `6b8621cef9d2a87de6e6eabc1743359c6274da5a`. Session: `S4_FinalChecks_20260909`. Reviewed: 2026-09-09 UTC. The paired JSON records exact attempt IDs, image names, bytes, SHA-256 hashes, real PNG dimensions, frame metadata and source bindings.

| Original image in each attempt | Observed result |
| --- | --- |
| `01_starter_0_7_56.png` | Round 0 / 0 LEFT, H SANDBOX, unpadded 0 Points, +100 health, 7 / 56 ammo, M1911 image/name, selected slot 1 and Empty slot 2 are visible. Compact panels remain separated. |
| `02_expanded_tools.png` | HELP + SANDBOX shows the movement/weapon/menu control legend; Return to rounds; Enemies; Weapons + Points; Next box: RANDOM; Scene + Reset; Back to game. Button labels and key badges fit; the central tray stays clear of corner HUD panels. |
| `03_compact_1500.png` | Compact HUD displays 1500 Points without padding, +100 health, 7 / 56 ammo, selected M1911 and Empty slot 2. The tools tray is closed and the points label stays clear of health. |
| `04_pause.png` | PAUSED panel reads Round 0, 0 kills, 1500 Points and Survived 0m 3s. Resume/Esc, Restart/Enter and Quit/Q fit within their buttons; dimmed gameplay remains visible behind the panel. |
| `05_death.png` | GAME OVER panel reads Round 0, 0 kills, 550 Points and Survived 0m 4s. Try again/Enter and Quit/Q fit. Background HUD agrees on 550 points and zero health; the fixture's real 950-point box spend explains the change from 1500. |
| `06_restarted_starter.png` | Restarted compact gameplay returns to 0 Points, +100 health, 7 / 56 ammo, selected M1911 and Empty slot 2. Pause/death/tool overlays are absent. |

The 1280×720 control legend and Enter key badges are small but readable in the original pixels. At 2560×1080 the world extends horizontally while HUD elements retain a centered safe region; menus stay centered. No required glyph omission, unintended panel overlap or edge clipping was observed in these images. The cyan line and gold sandbox range marks are world rulers.

This is a new review of all four supplemental attempts, not a relabeling of the earlier default-suite stills. Original image files were opened without edits. The first ultrawide preview was transported at a reduced size, so all six ultrawide images were reopened with original detail explicitly forwarded.

These fixtures exercise production controller events and drawn button coordinates, not native OS input. The fixture grants 10000 points and explicitly spends 8500 for the 1500 display; a real box purchase spends 950 before explicit fatal damage exposes the death menu.

Sparse stills do not establish animation quality, active box/Ready prompts, aura transitions, native controls, performance, audio quality, arbitrary resolutions, physical display/DPI readability or user perceptual approval. Those remain separate reviews.
