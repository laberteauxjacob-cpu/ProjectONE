# Final packaged portability review

Source S: 3d75c2faa0cecef6075f00755d8e4f5dd2562817. Actual run 20260914T100207_905780_portability_7d977372 completed on /Game/ONE/Maps/Portability06: **150 assertions passed, zero failed; all three appearances completed**. The timeline contains 2,454 actor observations; capture contains 732 frames at 1600×900. This review opened **24 original frames**, eight chronological points per appearance, with full-frame context and no crops or alteration.

| Appearance | Original frame numbers | Actual runtime observations |
| --- | --- | --- |
| Maintenance | 48, 87, 98, 166, 206, 227, 251, 264 | One attack/19 player damage; one fall and recovery; death and cleanup |
| Laboratory | 292, 329, 340, 391, 431, 453, 478, 491 | One attack/19 player damage; one fall and recovery; death and cleanup |
| Facility Staff | 520, 559, 568, 627, 668, 689, 714, 727 | One attack/19 player damage; one fall and recovery; death and cleanup |

Every original assertion matches the engine log and runner result. Original final source/runtime inputs were recorded unchanged. Report/log/numeric files and each selected JPEG were rehashed; large runtime binaries, unselected frames and WAV were not reread. Nineteen initialization warnings remain: 18 missing-mesh bone lookups and one CrowdManager warning; no Error/Fatal events were found.

## Visible observations

Blue Maintenance clothing, the pale Laboratory coat and Staff's grey/brown clothing and darker hair remain distinguishable. Both relocated machines, floor seams and the upper wall stay in view. Selected bodies remain within the image and clear of the HUD.

Attack samples show raised/bent arms near the player. Early fall samples still have partly upright torsos and sever/blood feedback; later samples show extended bodies near the floor, raised torsos with bent legs, and upright recovered poses. Missing-arm silhouettes and detached pieces persist through those samples. Corpse-stage images show localized body/part clusters and red patches. Each cleanup image removes those remains and patches, with zero remaining on the HUD. No gross stretching, exploded geometry, missing whole appearance or persistent residue was seen.

The runtime separately records one attack contact/damage dispatch, one fall/recovery and one death cue per appearance, with 6/6/7 foot cues and maximum one active voice. Body-cue counters are zero; no missing event is inferred. Declared packet probes add one kill and 140 points per episode. Four requested regions are absent at its end; cleanup checks retire the actor, four pieces, wound sources, queries and voices.

## Scope and limitations

The fixture uses authored spawn/player markers and normal pursuit/attack, but applies injury, fall and corpse-cut probes directly. It temporarily disables random drops while retaining death eligibility, restores/retreats the player and invokes ordinary cleanup. This is not weapon discharge, natural knockdown or native human-input evidence.

Sparse snapshots do not establish continuous smoothness, foot planting or exact contact forces/timing. Fine face/eye/gore detail is small or turned away; not every cut or hidden piece is resolved. No continuous playback or perceptual audition occurred. Voice counters and recorded audio do not establish sound quality or audible synchronization. This is bounded visual review, not user approval or performance measurement.

[Frame identities](frame_identities.csv) and [portable review](review.json) retain original names, timestamps, hashes, settings and separate runtime/visual findings.
