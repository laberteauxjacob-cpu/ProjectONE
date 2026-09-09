# Candidate05 carried-rifle and machine profiles

Four completed packaged workloads use source `6b8621cef9d2a87de6e6eabc1743359c6274da5a`: M4A1 and Overcurrent with requested live targets of12 and18.
Host condition: `ambient_processes_recorded_no_isolation_claim`. These are explicitly non-isolated observations, not a controlled comparison to earlier candidates. Ambient processes can change measured costs; no causal improvement or GPU-time claim follows.

Each capture begins after real production-input acquisition and any rifle upgrade. Setup/construction/compilation are excluded. Every numeric CSV row after capture starts is retained, including initialization transitions, zero-duration rows and every frame-time spike. The measured scenario includes physical pistol deposit, two-machine overlap, carried rifle held bursts/reloads, replenished live enemies, physics/debris and explicit profile-only health protection. Screenshot and audio recording remain disabled.

| Rifle / requested live | Full mean ms | p95 ms | p99 ms | Maximum ms | Exact-live fraction | Both Active s | Committed combat shots |
| --- | --- | --- | --- | --- | --- | --- | --- |
| M4A1 / 12 | 8.3500 | 8.9213 | 9.6114 | 25.3392 | 75.74% | 3.700500 | 144 |
| M4A1 / 18 | 8.5201 | 9.5430 | 10.3240 | 34.0449 | 71.63% | 3.708500 | 144 |
| Overcurrent / 12 | 8.3533 | 9.1223 | 9.8786 | 18.8083 | 65.14% | 3.704200 | 180 |
| Overcurrent / 18 | 8.4065 | 9.2134 | 10.1402 | 22.7024 | 61.37% | 3.704900 | 180 |

Each analysis contains the full run, complete cumulative-FrameTime frames inside10–20s, both-machine-Active counter selection and actual rifle-combat phase. Selected reports point into the one complete timeline; no duplicate numeric timeline or removal of unfavorable spikes is used.
Percentiles interpolate at(n−1)*p. Spike thresholds are strictly greater than16.7,33.3,50 and100ms. Exact count fractions describe measured frames; replenishment does not mean the target count was continuously maintained.
MachineState and MachineVisual are inclusive timings in ONEProgression; MachineVisual nests inside MachineState. PresentationDriver is a separate inclusive scope in ONECandidate05Presentation. Individual Chaos/physics scopes stay separate and are never summed/subtracted to estimate unique CPU wall time. HeldAura/AmbientVoices/ZombieVoices/LiveMagazines are counts, not milliseconds or audio quality measures.
The source event token includes UPGRADED_COMBAT for both variants. Variant identity instead comes from explicit CSV metadata, EffectiveUpgraded and actual family/variant observations, with the requested rifle checked throughout combat.
Requested1600x900/cap120/VSync0 settings and actual CSV resolution/available console readbacks are reported separately. Raw CSV command lines, user/host details and event text remain private. Every numeric source cell is reparsed after writing; inventory.json binds output byte sizes and SHA256 values.
No earlier-candidate report is read or treated as a matched baseline. One run per workload supplies no confidence interval or causal attribution.
