# Candidate06 S3 UI — original PNG review

Source: `c2c422012f4f219fb5e63ac24294a6ba8428f999`.
**All 12 original PNGs inspected; no blocking layout finding in this scope.**
The two actual packaged runs passed 49 assertions each, zero failures, exit 0.
Each result binds the same six S3 runtime identities before and after execution.

| Resolution | Actual run | Viewed images |
| --- | --- | --- |
| 1600×900 | `20260909T210302_430077_ui_15050378` | All six, `01_starter_0_7_56.png` through `06_restarted_starter.png` |
| 1280×720 | `20260909T210316_412498_ui_655b534c` | All six, in the same capture order |

Images were viewed directly at their original resolution. No image was cropped,
retouched, resaved or re-encoded. `review.json` binds actual PNG bytes/SHA256,
IHDR dimensions, per-image frame/state metadata, source and runtime identities,
and result/checks/input/log hashes. The proposed image paths are relative to
the repository's `Evidence/Candidate06/UI` directory.

At both sizes, starter/restarted gameplay shows zero points, 100 health,
M1911 7/56 and empty slot 2, with separate round, points/health and weapon panels.
The expanded help tray fits completely: held height modifiers, LMB fire,
Box hold F, upgrade tap F/auto-return, shake settings, weapon forcing, all
three forced pickup buttons, and Back to game remain readable without a
clipped final row. The compact 1500-point state keeps its principal panels
separated. A small aim-height caption crosses background floor lettering at
the chosen cursor position; the main HUD values and controls remain readable.

Pause shows round 0, zero kills, 1500 points and three distinct Resume/Restart/
Quit controls. Game Over shows 550 points and distinct Try again/Quit controls;
the balance reflects the disclosed 950-point Box purchase before the fatal-damage
fixture. The restarted screenshot returns to the original loadout and zero
points. Headings, elapsed-time text and buttons fit within both viewports.

All 49 ordered assertions per run match their logs. Each actor report also
contains five valid drawn-button-target metadata rows, which are not extra
assertions. Production controller input exercises grant points, close tray,
resume, restart and quit. This is engine-simulated input, not native desktop
input; the three pickup buttons are visually checked here and have separate
native-action evidence. The legacy actor's `candidate: 05` field does not change
the actual Candidate06 S3 source/runtime binding.

Scope is these 12 static images at 1600×900 and 1280×720. No other resolution,
display scaling, continuous playback, audio audition, performance, natural
movement or player-feel approval is claimed. The fixture uses declared point
setup, a Box-approach teleport, actual purchase, fatal damage, restart and quit.
Raw private logs are not proposed for publication.
