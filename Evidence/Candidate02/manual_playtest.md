# Direct packaged control review

The complete release archive was extracted and its root `ProjectONE.exe` launched without candidate/test flags. Native keyboard and mouse input controlled the normal 1600×900 gameplay camera. This was a short launch/control review, not a sustained human playthrough or a perceptual audio audition.

The initial review used source `60b6b9f1ab3efa168011e56d7aabfb8d0f6fd9cd`:

- The ordinary round started, infected reached the idle player, and the death screen appeared. Enter restarted with 100 health, carbine 24/192, shotgun 6/36, zero score and a five-second countdown.
- F1 entered the sandbox with zero enemies and visible distance references. Direct selection with 2 displayed the SG-01 mesh and correct HUD.
- A mouse click fired one shotgun shell (6→5) and displayed the pump operation. R began loading; Escape paused at 5/36; resume completed to 6/35. Tab displayed the carbine at 24/192 while retaining shotgun 6/35.
- A carbine mouse click reduced its loaded count to 23. F2 registered one infected and F3 increased the count to seven. F4 restored both slots to 24/192 and 6/36.
- **Failure found:** two sandbox spawns remained on the right rack while the other infected reached and killed the stationary player. Source/geometry review confirmed isolated tabletop navigation was accepted by the developer spawner. This blocked publication and required a source correction and new package.
- F5 reset the sandbox after death to zero enemies, full health and both initial ammunition counts. F1 returned to normal rounds. Escape displayed pause; Q quit.
- F6 was invoked only after the player had died; this interaction does not establish its cleanup result. Actual gore/case cleanup is covered by the packaged runtime checks.

The rack correction was directly checked in the extracted `94f8426` build: F2 placed the infected on the floor beyond the bench, and it reached and attacked the stationary player. That review also found Unreal's inherited F1–F5 development rendering shortcuts executing alongside sandbox actions. F2 could leave the scene unlit. The final configuration removes exactly those five inherited bindings.

**Final control follow-up passed on `0637288d32b6fbebc67ef93d4f03e439ff38bb67`.** The adopted fresh public-checkout package was launched normally, without test/candidate flags:

- F1 entered the empty sandbox with normal lighting. F2 spawned one infected on the floor; it reached the player. F3 increased the count to seven, all visibly on the floor. F4 retained full ammunition. Lighting and shadows remained intact through these inputs.
- F5 reset to zero enemies, 100 health, carbine 24/192 and shotgun 6/36, while preserving normal lighting. This directly checks the inherited-shortcut correction.
- Direct selection with 2 showed the shotgun. One mouse click consumed one shell (6→5) and displayed the pump action. R began reloading; Escape paused at 5/36; resume and subsequent completion produced 6/35.
- A mouse-wheel input switched to the carbine at 24/192 while preserving shotgun 6/35. A mouse click reduced the carbine to 23. F6 was invoked with lighting unchanged; exact cleanup counts remain covered by the runtime integration checks.
- F4 restored both slots to 24/192 and 6/36. [The captured final control view](manual_controls.jpg) shows that state. F1 returned to normal round countdown; Escape paused and Q quit.

The final ZIP (`0cb433a407c46e511c9fc831d516dfbfa68e660bad8c505a3786f04f5e54e7a3`) was then extracted separately. Every manifest file size/hash and the main executable hash matched. Launching its root executable without flags opened the actual Candidate02 arena and ordinary round. A Windows firewall prompt appeared over the running game; automation did not act on that security prompt and stopped only this smoke-test process. This verifies extraction and game startup, not additional controls behind the modal. The full direct control review above used the byte-identical package before archiving.

The engine also showed a Windows firewall prompt during an earlier extracted-build launch; no Allow action was issued by automation. The review does not claim a clean operating-system installation or a successful prerequisite installation on a machine without development tools.

The separate [visual review](visual_review.md) describes inspected chronological gameplay frames. [Combat checks](combat_checks.txt) and [legacy presentation checks](presentation_checks.txt) describe instrumented gameplay. These forms of evidence do not establish perfect finger contact, foot planting, long-session stability or subjective sound quality.
