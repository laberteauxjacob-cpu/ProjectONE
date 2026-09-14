# Candidate07 recorded audio

The bank contains 134 processed cues made from 76 retained source files (38,893,854 original bytes). Physical shots, mechanism Foley, performed creature voices, footsteps, clothing, contacts and ambience use actual recordings. Only the existing intentional upgrade energy design is synthesized; pickup and machine reward cues retain their earlier authored design.

[CREDITS.md](CREDITS.md) and [source_inventory.json](source_inventory.json) identify every selected original, creator, title, source URL, license version, retrieval date, downloaded/archive identity and unchanged per-file hash. [Notices](Notices) retains upstream notices and CC0 / CC BY 3.0 / CC BY 4.0 legal texts. These grants apply only to the listed audio. No general license is assigned to Project ONE.

| Runtime family | Recorded foundation and qualification |
| --- | --- |
| M1911 reports | Prepared 1911 recordings, near/mid microphone perspectives |
| M4A1 reports | Prepared AR-15/M4-family recordings; exact M4A1 identity unverified |
| 870 reports | Recorded Benelli Nova 12-gauge pump shotgun analogue |
| Mechanical actions | Recorded mixed rifle/stapler/tape-measure Foley; SXP 20-gauge pump rack; casing handling. Exact per-model action identity is not established |
| Upgrade reports | Above physical recordings with the retained original Last Word / Overcurrent / Gravebreaker energy formula layered at fixed playback pitch |
| Infected idle/pursuit/attack/hit/heavy hit/fall/death | Performed moans and studio grunts/yells, moderate rate/filter edits. No recordings of actual injury or distress |
| Player/infected steps | Concrete footsteps, boots on an aluminum ladder for metal, and cloth/leather recordings; four variants per owner/surface |
| Body/flesh contact | Wet-towel-on-stone and soft-object impacts; explicit Foley analogues |
| Case/shell contact | Actual casing / empty 12-gauge shell on concrete; recorded metal Foley layer for metal surfaces |
| Magazine/world impact | Recorded object/metal impacts used as analogues; exact magazine drop or live bullet impact recordings are not claimed |
| Facility ambience | Actual mini-fan exhaust, water drops and plate impacts. Low-pass fan variant stands in for a distant motor |

Six Freesound inputs are the official publicly accessible MP3 representations, not downloaded upload masters. Published OGG sources are also retained as delivered. Processing to PCM does not recover information lost by upstream codecs. Sources requiring a purchase, account, NC restriction, or an unverified game/video extraction were not used.

`recipes.json` is editable source for trims, layers, gain, frequency roll-off, loop flags and intentional energy overlays. `manifest.json` binds the actual source hashes, exact generated WAV hashes, trim adjustments, duration, PCM format, peak/RMS, and runtime asset paths. The six report variants per firearm come from four excerpts across two mic perspectives plus two additional EQ/tail edits; this is not a claim of six independent performances. Case/shell variants explicitly use three edits of one contact cluster. Other banks use multiple actual recorded events. Runtime randomization chooses assets with no immediate repeat and leaves pitch at 1.

The processor verifies all selected hashes before decoding. It uses arithmetic channel averaging, resampling to 48 kHz, smooth roll-offs, DC removal, documented cuts/layers, measured onset trimming with up to 2 ms of preroll, short edge fades and peak normalization. Loops have an 80 ms cyclic crossfade. Physical excitation is never regenerated from noise or oscillators. The intentional energy-only formula reuses Project ONE's C05 parameters and C03 math primitives; it does not include the old synthesized gun-report layer.

From the repository root, with Python 3.10+, NumPy and PyAV installed:

```powershell
py -3.10 -B Scripts/process_candidate07_audio.py
py -3.10 -B Scripts/process_candidate07_audio.py --check
```

`--decoder-library <directory>` supports an existing separate PyAV installation. No package installation, download or Unreal launch is performed by this script. `Scripts/import_candidate07_audio.py` is run separately by the Unreal pipeline with commandlet audio enabled. It imports only manifest-matching PCM into `/Game/ONE/Audio/Candidate07`; actual import results are written privately before curation.

Offline processing and exact recomputation passed for all 134 PCM files with zero full-scale samples. This verifies bytes and numerical headroom. It does not establish audition, perceived realism, localization, repetition comfort, final in-engine balance or user approval. The recorded analogues and provisional creature role selections remain listening-review limitations.

Runtime contact synchronization and budgets are described in [the audio integration note](../../../Docs/Candidate07Audio.md).
