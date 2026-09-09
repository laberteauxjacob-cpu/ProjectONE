# Candidate06 rifle, shotgun, pickup and machine profiles

6 completed packaged workloads use source `c2c422012f4f219fb5e63ac24294a6ba8428f999`. Each explicitly selected weapon variant appears once at requested live counts 6/12/18.
Host condition: `ambient_processes_recorded_no_isolation_claim`. These are explicitly non-isolated observations, not a controlled comparison to earlier candidates. Ambient processes can change measured costs; no causal improvement or GPU-time claim follows.

Each capture begins after real production-input acquisition and any requested weapon upgrade plus two actual pickup overlap collections and three forced world-drop placements. Setup/construction/compilation are excluded. Every numeric CSV row after capture starts is retained, including initialization transitions, zero-duration rows and every frame-time spike. The measured scenario includes physical pistol deposit, two-machine overlap, the requested carried weapon firing/reloading, replenished live enemies, ordinary timed power-up decay, world pickups, physics/debris and explicit profile-only health protection. Screenshot and audio recording remain disabled.

| Weapon / requested live | Full mean ms | p95 ms | p99 ms | Maximum ms | Exact-live fraction | Both machines Active s | Committed combat shots |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 870 / 6 | 8.3359 | 8.7096 | 8.9514 | 20.6116 | 93.26% | 3.217200 | 16 |
| 870 / 12 | 8.3398 | 8.7993 | 9.2964 | 25.5873 | 89.03% | 3.208500 | 16 |
| 870 / 18 | 8.3468 | 8.9135 | 9.4007 | 30.0013 | 86.28% | 3.217300 | 16 |
| M4A1 / 6 | 8.3399 | 8.8964 | 9.6056 | 21.2065 | 79.85% | 3.216800 | 144 |
| M4A1 / 12 | 8.3709 | 9.0975 | 10.2173 | 24.9783 | 69.75% | 3.211200 | 144 |
| M4A1 / 18 | 8.3750 | 9.0609 | 10.2795 | 31.4236 | 64.91% | 3.217200 | 144 |

Each analysis contains the full run, complete cumulative-FrameTime frames inside 10–20s, both-machine Active selection, requested-weapon combat, both timed effects, at least three world drops and their combined occupancy. Selected reports point into one complete timeline; no duplicate numeric timeline or removal of unfavorable spikes is used.
Exact-live fractions use rows with recorded project counters; each file has one initial row without those counters. All numeric frame rows remain in the timing statistics. Each workload's maximum occurs at frame zero. Across 22,129 rows, seven frames exceed 16.7 ms and none exceed 33.3 ms; these observations do not establish a locked frame rate.
Percentiles interpolate at(n−1)*p. Spike thresholds are strictly greater than16.7,33.3,50 and100ms. Exact count fractions describe measured frames; replenishment does not mean the target count was continuously maintained.
MachineState and MachineVisual are inclusive timings in ONEProgression; MachineVisual nests inside MachineState. PresentationDriver is a separate inclusive scope in ONECandidate05Presentation. Individual Chaos/physics scopes stay separate and are never summed/subtracted to estimate unique CPU wall time. HeldAura/AmbientVoices/ZombieVoices/LiveMagazines are counts, not milliseconds or audio quality measures.
The source event token includes UPGRADED_COMBAT for both variants. Variant identity instead comes from explicit CSV metadata, EffectiveUpgraded and actual family/variant observations, with EffectiveFamily and the requested weapon checked throughout combat.
Pickup occupancy reports actual measured frame fractions and gameplay-clock durations. LastShot contact/outcome values repeat until the next discharge; never sum them as cumulative hits. Timed effects and world-drop lifetimes are allowed to expire during the capture.
Requested 1600x900/cap120/VSync0 settings and actual CSV resolution/available console readbacks are reported separately. Raw CSV command lines, user/host details and event text remain private. Every numeric source cell is reparsed after writing; inventory.json binds output byte sizes and SHA256 values.
No earlier-candidate report is read or treated as a matched baseline. One run per workload supplies no confidence interval or causal attribution.
