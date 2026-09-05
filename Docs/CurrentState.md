# Current state — Candidate04

Candidate04 implements the M1911 starter, a 950-point Mystery Box, a
5,000-point Pack-a-Punch and independent Last Word, Overcurrent and Gravebreaker
variants within the existing arena. The catalog has six rows; inventory has
exactly two owned slots and supports a valid unarmed state. The complete rules,
balance values and controls are in the [pass report](Passes/Candidate04.md).

This checkpoint is built in the editor. The initial inventory/native corrections
and runtime checks have been exercised; final public-source build, packaged
verification, recording, native-input attempt and performance evidence remain
in progress. See the [Candidate04 evidence index](../Evidence/Candidate04/README.md)
for the final distinction between implementation and verification.

The expanded progression run passed 72 assertions, including retrieval cancellation,
reserved ownership, technical recovery and real level restart. The arsenal run
passed 148 assertions for six effective weapons, exact event timing, physical
magazines, damage, cover and bounded penetration. The corrected legacy weapon
suite passed 85 assertions. Its first run exposed test-loadout initialization
order; the explicit legacy fixture now runs after ordinary starter initialization.
The first movement run is not accepted as two-family coverage. New trial identity
assertions require actual rifle/shotgun selection in the final packaged rerun.

Both complete production-input walkthroughs passed 58 checks. Capture 02 produced
2,890 engine JPEGs and 148.331 seconds of engine master audio. All 18 selected
weapon/machine audio phases contain signal, with zero full-scale samples; no
perceptual listening is claimed. Chronological frame inspection found improved
shotgun visibility but remaining tray glare, prompting relocation of the light
away from the hands before final package inspection. These working-tree checks
do not establish a released binary or user visual approval.

## Gameplay and ownership

Normal play starts with M1911 7/56 and slot 2 empty. Hold F for 0.4 seconds on an
in-range unobstructed machine. Release, target/range change, slot change, pause
or death cancels unfinished holds; another action requires release and a fresh
press. Visible prompts disclose price, action, reserved state and replacement.

The box chooses one eligible family at payment and shows actual assembled models
for five seconds. Weights begin equal and are editable. Reserved families are
excluded. Rewards wait indefinitely; collection revalidates the exact slot and
instance plan. Duplicate available families refill their effective variant
without creating a copy or downgrading it; full ammunition consumes the disclosed
result. Sandbox T grants 10,000 points; Z/X/C force the next family and V resets
random, while normal prices stay in force.

Pack-a-Punch reserves the same instance and slot and spends 5,000 atomically at
handoff acceptance. Preaccept cancellation is uncharged. Nine seconds include
intake, clamping, processing and output. The other available weapon remains
usable; depositing the only weapon warns of and enters an unarmed state. A box
reward can fill the other empty slot. Fresh collection restores that same slot
and instance once, upgraded and refilled, through normal equip. Upgrades have one
tier. Current-run technical failure before delivery restores the original weapon
snapshot and refunds once; death/reset invalidate old deliveries and refunds.

Pistol and rifle magazines drop from the evaluated seated prop at their removal
events, inherit movement, bounce and settle without blocking characters, shots
or navigation. Twelve actors maximum and eight-second lifetimes bound them.
Fresh hand props remain separate. Cancellation does not recreate an old magazine;
a loaded gun with its magazine removed requires R before firing. Tube shotguns
have no detachable magazine. Existing casings, pump obligations, sprint priority,
auto-reload and regional pellet resolution remain in use.

## Preserved foundation and limits

Candidate03's eight-direction 225/370 cm/s player movement, independent mouse aim,
stepping turns, 100/195 cm/s infected movement, regional damage, finite blood and
skeletal remains are retained. The same infected archetype, round/sandbox modes
and prior 96 map actors remain, with two powered machines and rebuilt navigation.
No map expansion, perks, extra enemy archetype or additional weapon family was added.

Prior character stride/pose limitations, fixed fingers, camera occlusion and
small-prop readability remain provisional. Runtime weapon and machine inspection
must be assessed at normal camera scale. Original synthesized sound banks and
recorded engine WAVs do not establish perceptual sound approval. Performance must
be measured again with the new effects; Candidate03 capped results are no guarantee.

The preserved [Candidate03 release](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/tag/candidate03)
uses gameplay source `e81e1137ed891570c73c8c278fc2c8cc2250bd04`, with final tag
commit `4aded688bbdb1a8bcfdc3ba1d6a6e3eb698c909a`. Its 398,043,452-byte Windows
archive SHA-256 remains
`ada689b2314d3047d581d0523a2cbcd91f57661d06554d6288a6b4fae79600d2`.
The [Candidate03 pass report](Passes/Candidate03.md) retains its distinct evidence.
