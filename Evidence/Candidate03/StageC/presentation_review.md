# Stage C presentation review

The second rendered editor-game recording uses the corrected flash material,
the existing room lights at 18% intensity in dim mode, fixed exposure and the
ordinary 1600x900 gameplay camera. It contains 2,805 actual JPEG captures and
genuine Unreal master-submix audio. This is an internal stage gate; final
Candidate03 packaged regression follows Stage D.

The first recording passed transform assertions but failed visual review: an
unchecked `RGB` output name did not connect the vertex-color emissive input.
The importer now uses Unreal's aggregate output and asserts every connection,
then reads both complete branches back after compilation. The earlier very dark
room setting also obscured the comparison. That recording is superseded.

The rerun passed 121 presentation checks, including same-frame evaluated muzzle
positions, attached light and shape positions, expiry, no immediate sound-bank
repeats and genuine reload transfers. Separate production-state casing checks
passed 49 cases, including extraction timing, interruption, inheritance, gravity,
bounce, supported settling, lifetime and count limits. The weapon regression
passed all 85 retained checks after the Stage C code changes.

Root inspected consecutive carbine sequences for all eight bright/dim firing
conditions. Exact frame windows are in `presentation_metrics.json`. The short
white/warm tapered flash is attached to the barrel, while the thin trajectory
continues along its fired direction. Dim frames show brief local illumination
on the floor, weapon and player; neighboring frames return to the dim setting
without a persistent glow. Moving brass appears as small separate specks; its
subtle appearance is appropriate to its 4.5 cm size, but it is less readable
than the larger shotgun hull. No contact sound was added.

The independent shotgun review inspected 318 consecutive crops across phases
8–15, a native full frame and a detailed 24-frame extraction window. The flash
stays at the current barrel in all four movement conditions in both light modes.
Frames 01439–01459 show a small red/brown hull separating near the receiver,
changing orientation and following a curved flight. Casings occupy only a few
native pixels, and dim cases are difficult to see. Exact floor contact and
settling are established by lifecycle checks, not certified from these images.
Thin trajectories remain more visually prominent than the short flash in bright
conditions. The existing mechanical crouch/arm silhouette remains provisional.

Stage C's technical and bounded visual gate passed before Stage D began. This
does not claim final character naturalism, audible approval or packaged testing.

Review uses chronological captured sequences and enlarged crops for inspection;
it does not claim that isolated stills establish motion or that the reviewer
played a continuous video. The full recording is assembled from actual capture
timestamps without synthesized frames, with the genuine engine audio.

The twelve original shot sources have six independently authored parameter
profiles per gun. The in-game audio audit measures energy, coverage, clipping
and actual bank selections. No perceptual audition tool was available, so
realistic timbre, audible variation and mix quality remain unapproved by ear.
The recording provides the comparison for the user.
