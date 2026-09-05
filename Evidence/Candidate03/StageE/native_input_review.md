# Final packaged native input

The archive-bound executable from source
`e81e1137ed891570c73c8c278fc2c8cc2250bd04` was launched from the extracted
Candidate03 archive and operated through native mouse/keyboard controls.
The [event record](native_input_review.json) distinguishes intended actions,
observed results and private log identities. The complete
[native recording](../native_controls.mp4) contains genuine engine audio;
[assembly](native_capture_assembly.json), [audio](native_audio.json) and
[motion review](native_motion_review.md) describe their separate evidence.

The observer saved 4,970 gameplay frames at 1600×900. Its zero-failure completion
marker means the recorder finished; it contains no functional assertions.
An earlier blocked/idle recording is excluded. An OS permission prompt cleared
before the reviewed session; the agent did not act on that prompt.

| Native action | Observed result |
| --- | --- |
| Two mouse drags with carbine firing | Aim changed in opposite directions; ammunition 24 → 23 → 22 |
| R, carbine | Reload shown; completed 24/190 from 22/192 |
| 2, shotgun fire, R | Shotgun mesh/HUD selected; one shot 6 → 5; pump and reload shown; completed 6/35 |
| 1, Tab, wheel up/down | Selected carbine, shotgun, carbine, shotgun; inventories retained |
| F7, shotgun fire, F4, F7 | Dim lighting, one shot 6 → 5, both weapons refilled, bright lighting restored |
| Escape twice | Pause overlay appeared and cleared |
| F2 | One infected spawned, approached and killed the stationary player |
| LMB and R after death | Shotgun stayed 6/36; no shot or reload began |
| Q after recorder completion | Normal exit; both game processes disappeared |

There were exactly two carbine and two shotgun discharges in this recording,
zero kills and zero points. The input labeled “fire toward spawned infected”
was attempted after the logged death, so it is not a successful enemy hit.

A second session used the documented command
`.\Scripts\Launch.ps1 -Candidate Candidate03`, which supplied only windowed
1600×900 options. It had no observer or validation flags. Normal round 1 started
with six infected. F1 entered the empty sandbox; F3 spawned six. The log records
player death immediately before F5 reset the encounter to zero infected,
100 health and full inventory. A carbine click consumed one round. F6 accepted
the cleanup command, but there were no corpses/blood to clear and the case's
six-second lifetime had elapsed; this is not populated-cleanup proof.

F1 returned to normal rounds. Escape paused and Enter restarted the countdown.
After five seconds, round 1 again had six infected, health 100 and full 24/192
carbine plus 6/36 shotgun ammunition, with no stray shot observed. Escape/Q
exited normally with logged status zero and both processes gone.

The two private logs retain 26 and 65 initialization/transition warnings,
respectively: socket queries before mesh setup and missing-Recast crowd-manager
messages. Neither log contains an Error, Fatal or ensure. Their full private
contents are not published.

The available native tool delivered taps rather than sustained W/Shift input.
This session therefore does not prove eight-direction native movement, sprint
reload interruption, automatic empty reload or pump interruption. Those cases
are covered by the separate production-input packaged tests and staged movement
recordings. Populated cleanup is covered by the physicality scenario. Perceptual
audio listening remains unavailable. The four shot windows and two reload
windows contain audio; the final approximately 95 seconds are silent. This
recording does not establish audible infected attacks or an approved mixed
combat soundscape. Live UI observations, saved-frame motion inspection and
automated checks are reported separately.
