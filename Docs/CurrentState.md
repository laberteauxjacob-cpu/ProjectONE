# Current state — Candidate07

Candidate07 integrates Maintenance, Laboratory and Facility Staff on the retained
infected rig, with an infected-specific motion graph, physical crowd contact,
living fall/recovery and recorded physical audio. The three appearances share
statistics and behavior. The player mesh, bind, animation graph and weapon grips
are preserved.

**Release status: Windows prerelease prepared; native control review is blocked and visual/audio approval remains provisional.** Gameplay source S is
`3d75c2faa0cecef6075f00755d8e4f5dd2562817`; publication revision D is [the candidate07 tag](https://github.com/laberteauxjacob-cpu/ProjectONE/tree/candidate07).
Package and checksum: [ProjectONE-Candidate07-Windows.zip](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate07/ProjectONE-Candidate07-Windows.zip), `e3dae5b6ead801d03c6bc67362f8849ae465df88bd1e691ceed2681b18e61348`.
The [publication-verification attachment](https://github.com/laberteauxjacob-cpu/ProjectONE/releases/download/candidate07/Candidate07-PublicationVerification.json) identifies actual D and records public refs, anonymous downloads, fresh D checkout/LFS and prior-candidate preservation when issued. These later public checks are not asserted by this committed text.

The [Candidate07 pass](Passes/Candidate07.md) holds the detailed evidence and
limits. [Controls](../README.md#controls), [combat rules](CombatRules.md) and
[survival/reward tuning](Candidate06SurvivalRewards.md) describe current play.

## Infected behavior and appearance

Maintenance wears damaged blue workwear; Laboratory has a lined split coat and
grey receding hair; Facility Staff has rolled sleeves, a loose tie and brown
trousers. Each uses five modular parts on the same 21-bone authored bind, with
the existing head, both-arm and supported left-leg sever paths. All three source
models passed 140 evaluated seam samples each. Their appearance data does not
define damage, health, speed, rewards or weapon behavior.

Fourteen infected-only clips cover locomotion, turns, five sided attacks, heavy
hit/stumble and separate prone/supine recoveries. Attack choice uses available
limbs, distance, bearing, approach speed and recent performance. Bounded entry
momentum and late recovery movement connect attacks to pursuit. A swing still
has at most one valid health contact and respects cover, interruption and death.

Living upper-body simulation yields around the capsule-driven lower body. Up to
four living actors can use full-body fall simulation concurrently. Falling
cancels attack, keeps the same live actor and health, and grants no kill reward.
Get-up requires local solid support and room for the body. Pickup triggers cannot
act as floors or block recovery. Bounded effort can move the fallen actor around
another actual fallen/dead body; it cannot remove that blocker or bypass a wall.
Persistent obstruction can leave the actor fallen and damageable.

Recovery preserves missing parts and uses the captured physical pose before the
authored rise. Prone/supine durations are 2.90/3.35 seconds including the 0.35-second
entry blend. Death cancels recovery; death/drop identity remains singular, and a
physical death's reward origin follows the evaluated pelvis.

## Preserved gameplay values

| Rule | Current value |
| --- | --- |
| Player walk / sprint | 225 / 370 cm/s |
| Infected shamble / pursuit, health, strike | 100 / 195 cm/s; 112 HP; 19 damage |
| Player repeated-damage protection | 0.55 seconds |
| Starting inventory | M1911 7/56; empty second slot |
| Regeneration | After 15 accepted-damage-free seconds, 10% of current maximum HP per second |
| Combat points | 10 per living victim per discharge; +100 body kill or +120 qualified head kill |
| Head health damage | 1.5×; head trauma does not execute a surviving target |
| Mystery Box | 950 points; five-second reveal; excludes owned/reserved/upgraded families |
| Pack-a-Punch | 5,000 points; nine-second processing; automatic original-slot return; 15-second ready expiry loses the gun without refund |
| Natural power-up drop | One 1% total roll per registered death; equal initial type weights |
| Timed effects / world pickups | Insta-Kill and Double Points refresh to 30 seconds; world lifetime 30 seconds, cap eight |

Max Ammo fills available owned guns while retaining machine reservations. Pause
freezes gameplay timers. Logical mouse aim stays separate from camera shake;
the H tray retains 100%, 50% and Off options. Detailed operation, penetration,
pellet-majority head qualification and refund rules remain in the linked tuning.

## Audio and review status

The recorded bank has 76 retained originals and 134 processed cues, with editable
recipes, hashes and notices. Actual shot/mechanism/contact events drive playback;
shared pools, priorities and cooldowns bound voices. Firearm and Foley analogues
are identified explicitly. Upgrade energy remains intentionally synthesized;
machine and pickup accents retain their earlier design.

The fresh public S checkout built Editor, Game and the Windows package and passed all 39 engine tests. All 26 C07 packaged runs passed 7,506 driver/companion assertions; six separate C06 baseline profiles passed 228. Four final engine recordings passed full decoding and numerical PCM checks. Native desktop capture was blocked; no native input pass or perceptual audio audition is claimed.

Final native input: BLOCKED_NATIVE_CAPTURE: Windows returned cursor-access and monitor-capture errors after a fresh-window retry. Zero native actions or usable images; only the launched test process was terminated, and all six runtime files still match. Normal Quit and held-input review were not verified. Final chronological original-frame
review: 131 selected original chronological gameplay frames across four recordings, plus all 12 UI PNGs, were inspected with exact source/runtime/frame bindings. This is bounded still review, not continuous playback; the aim-height caption has a minor contrast issue over a pale floor stencil. Media/decode/PCM results:
PASS: four final engine recordings each passed all 32 inspection checks and full video/audio decoding. Combat, physicality, ordinary encounter and portability contain 2,380 / 2,413 / 3,292 / 867 encoded video frames with audio endpoints 79.317333 / 80.426667 / 109.717333 / 28.900000 seconds. Encoded counts include documented timing holds and differ from original capture-frame counts. Original PCM measurements, source-frame inventories, assembly timing and hashes are retained; this is not continuous viewing or audition. Recording-free performance and comparison:
All 19 recording-free workload analyses retain 66,511 original engine rows (44,389 C07; 22,122 C06) on the same RTX 3090/Ryzen 5950X host at 1600×900, cap 120, VSync off and D3D12. At 18 requested enemies, mean frame time rose from 8.4063 to 10.1898 ms for M4A1 and from 8.3484 to 10.4848 ms for 870. The C07 mixed 18-enemy run measured 10.6963 ms mean, 15.7497 ms p99 and 53.0143 ms maximum. All 19 maxima were first CSV rows; later spikes remain in the data. Actual live/state fractions, all distributions and single-sample/non-isolated host limits are retained in the performance evidence; no speedup or causal subsystem-cost claim.

Recorded output, cue counters and digital clipping checks do not establish
listening quality. **No perceptual audio audition is claimed.** Bounded frame
inspection is not continuous playback or user approval of game feel.

## Known limits and next work

RMB/Left Ctrl select fixed planes 168/65 cm above the player's floor; torso is
125 cm, with Ctrl priority and LMB as the only fire input. In a retained motion
validation, ten real Ctrl+LMB shots used world Z 68.474 cm while the corpse's body
query top was 61.743–62.698 cm. All missed. The exact regional query can intersect
that anatomy, but ordinary mouse height selection cannot follow it. That failed
[compatibility record](../Evidence/Candidate07/Verification/low_aim_compatibility.md)
is retained, outside final passing-matrix totals.

Recovery still needs physical space. The ordinary encounter recorded a sustained-contact fall, a 2.509 cm effort stopped for lost floor support, and a later prone recovery preserving 100.8 HP. A nearby corpse disappeared before clearance, so effort-caused escape is not established. The arranged physicality film completed two prone recoveries, retained four Fallen actors before phase cleanup and captured no supine recovery; its folded-to-prone entry remains conspicuous. No universal all-falls-recover promise is made.
The get-up entry remains stylized, and hidden crowd contacts/seams are outside
sparse-frame review. Host/cap/population limits belong with each profile result.

Recommend a separately authorized floor-level targeting pass. Perk chambers,
finished maps and hidden spawn-to-entrance routing remain future work; the tiny
Portability06 level tests relocated dependencies rather than a production map.
