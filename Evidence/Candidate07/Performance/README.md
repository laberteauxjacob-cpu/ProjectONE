# Candidate07 final profile comparison

Source `3d75c2faa0cecef6075f00755d8e4f5dd2562817`; preserved Candidate06 source `c2c422012f4f219fb5e63ac24294a6ba8428f999`.

Thirteen Candidate07 and six newly measured Candidate06 packaged runs. Six requested workloads match; seven additional mixed workloads have no Candidate06 counterpart. All numeric timelines, original analyses, spikes and state counts remain available.

| Candidate / weapon / requested / explicit falls | Rows | Mean ms | p95 ms | p99 ms | Max ms | Driver exact / valid |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| [C06 / M4A1 / 6 / none](c06/m4a1_6_analysis.json) | 3694 | 8.3391 | 8.8550 | 9.4739 | 21.3798 | 2754 / 3693 (74.57%) |
| [C06 / M4A1 / 12 / none](c06/m4a1_12_analysis.json) | 3687 | 8.3530 | 9.0548 | 10.2082 | 26.6295 | 2610 / 3686 (70.81%) |
| [C06 / M4A1 / 18 / none](c06/m4a1_18_analysis.json) | 3664 | 8.4063 | 9.2239 | 10.7592 | 32.7076 | 2227 / 3663 (60.80%) |
| [C06 / 870 / 6 / none](c06/870_6_analysis.json) | 3695 | 8.3365 | 8.7434 | 9.2011 | 21.2993 | 3258 / 3694 (88.20%) |
| [C06 / 870 / 12 / none](c06/870_12_analysis.json) | 3693 | 8.3387 | 8.8502 | 9.2930 | 26.8357 | 3225 / 3692 (87.35%) |
| [C06 / 870 / 18 / none](c06/870_18_analysis.json) | 3689 | 8.3484 | 8.9942 | 9.4869 | 32.7854 | 3258 / 3688 (88.34%) |
| [C07 / M4A1 / 6 / none](c07_profile_m4a1_6_none/analysis.json) | 3670 | 8.3925 | 9.1268 | 9.8875 | 34.8403 | 2948 / 3669 (80.35%) |
| [C07 / M4A1 / 12 / none](c07_profile_m4a1_12_none/analysis.json) | 3369 | 9.1425 | 10.8397 | 12.3588 | 39.3060 | 2326 / 3368 (69.06%) |
| [C07 / M4A1 / 18 / none](c07_profile_m4a1_18_none/analysis.json) | 3023 | 10.1898 | 12.4931 | 15.1796 | 46.2420 | 1888 / 3022 (62.48%) |
| [C07 / 870 / 6 / none](c07_profile_870_6_none/analysis.json) | 3692 | 8.3404 | 8.7912 | 9.3278 | 34.0832 | 3344 / 3691 (90.60%) |
| [C07 / 870 / 12 / none](c07_profile_870_12_none/analysis.json) | 3663 | 8.4079 | 9.1345 | 9.9475 | 39.1851 | 3382 / 3662 (92.35%) |
| [C07 / 870 / 18 / none](c07_profile_870_18_none/analysis.json) | 2939 | 10.4848 | 13.0128 | 14.6290 | 47.0730 | 2557 / 2938 (87.03%) |
| [C07 / M4A1 / 1 / mixed](c07_profile_m4a1_1_mixed/analysis.json) | 3695 | 8.3349 | 8.6500 | 8.9235 | 17.4733 | 3508 / 3694 (94.96%) |
| [C07 / M4A1 / 2 / mixed](c07_profile_m4a1_2_mixed/analysis.json) | 3693 | 8.3378 | 8.8099 | 9.3043 | 19.6226 | 2941 / 3692 (79.66%) |
| [C07 / M4A1 / 6 / mixed](c07_profile_m4a1_6_mixed/analysis.json) | 3673 | 8.3854 | 9.1124 | 9.9177 | 31.4167 | 2838 / 3672 (77.29%) |
| [C07 / M4A1 / 12 / mixed](c07_profile_m4a1_12_mixed/analysis.json) | 3327 | 9.2590 | 10.9709 | 12.3636 | 39.7144 | 2379 / 3326 (71.53%) |
| [C07 / M4A1 / 18 / mixed](c07_profile_m4a1_18_mixed/analysis.json) | 2880 | 10.6963 | 13.2935 | 15.7497 | 53.0143 | 1865 / 2879 (64.78%) |
| [C07 / Overcurrent / 12 / mixed](c07_profile_overcurrent_12_mixed/analysis.json) | 3374 | 9.1297 | 10.7084 | 11.9486 | 30.3598 | 2333 / 3373 (69.17%) |
| [C07 / 870 / 12 / mixed](c07_profile_870_12_mixed/analysis.json) | 3391 | 9.0860 | 11.0934 | 12.6584 | 46.0084 | 3001 / 3390 (88.53%) |

Paired differences in [comparison.json](comparison.json) are observed C07 minus C06 milliseconds, with both distributions retained. They do not establish a controlled improvement. C07 census state histograms and occupancy denominators are separate from the shared driver counts.

- All engine frame rows and spikes are retained, including initialization, zero duration and unfavorable maxima. No trimming or spike removal.
- Exact-live driver fraction uses only valid RequestedLive rows before replenishment. All invalid boundary rows remain in timing statistics.
- C07 separately retains every physicality census row. CSV-active census includes its unmatched boundary rows and samples after replenishment; no invented alignment to C06.
- None means no explicit fixture falls. Ordinary production contact falls can still occur. Mixed means additional explicit fall attempts, with actual acceptance/state counts reported.
- Alternating candidate order for the six requested pairs reduces simple ordering bias but does not control ambient load, cache state, random streams or population histories.
- Boundary process CPU observations cover the whole child invocation, not the CSV window, and omit new/retired/unreadable processes. Missing observations stay missing.
- 1600x900, cap120, VSync0 and screen percentage100. Capped warm scripted runs include two machines, requested-weapon combat, pickups and disclosed profile health protection; not startup or native play.
- Nested/inclusive CPU and worker scopes are retained separately, never added or subtracted as unique wall cost. No GPU attribution, universal FPS or causal speedup is inferred.
- No continuous playback or audio audition occurred in this curation; voice counters do not prove audibility.

[Payload inventory](inventory.json) binds every copied file and both comparison documents; the inventory does not self-hash.
