# Candidate07 performance review

Candidate07 has higher measured frame costs in all six matched requested workloads, with the largest differences at 18 enemies. These are observed regressions on this host, not a controlled attribution to one system. The 120 FPS cap does not establish sustained 120 FPS or a speedup.

The evidence covers 13 final Candidate07 packaged runs at source `3d75c2faa0cecef6075f00755d8e4f5dd2562817` and six newly measured preserved Candidate06 runs at `c2c422012f4f219fb5e63ac24294a6ba8428f999`. This review checked [the comparison](comparison.json), [all 19 original summaries](README.md) and their reported populations. The [payload inventory](inventory.json) identifies the retained analyses and numeric timelines.

| Matched requested workload, explicit falls `none` | Mean change ms | p95 change ms | p99 change ms | Maximum change ms |
| --- | ---: | ---: | ---: | ---: |
| M4A1 / 6 | +0.0534 | +0.2718 | +0.4136 | +13.4605 |
| M4A1 / 12 | +0.7895 | +1.7849 | +2.1506 | +12.6765 |
| M4A1 / 18 | +1.7835 | +3.2691 | +4.4205 | +13.5344 |
| 870 / 6 | +0.0038 | +0.0478 | +0.1268 | +12.7839 |
| 870 / 12 | +0.0691 | +0.2844 | +0.6545 | +12.3494 |
| 870 / 18 | +2.1364 | +4.0186 | +5.1421 | +14.2876 |

Changes are Candidate07 minus Candidate06. [M4A1 / 18](c07_profile_m4a1_18_none/analysis.json) has a 10.1898 ms mean and 15.1796 ms p99, against [Candidate06's](c06/m4a1_18_analysis.json) 8.4063 and 10.7592 ms. [870 / 18](c07_profile_870_18_none/analysis.json) has a 10.4848 ms mean and 14.6290 ms p99, against [Candidate06's](c06/870_18_analysis.json) 8.3484 and 9.4869 ms.

Seven additional mixed cases—M4A1 at 1/2/6/12/18, Overcurrent at 12 and 870 at 12—have no Candidate06 mixed counterpart. They describe additional workloads and do not isolate the cost of falling or recovery.

| Recorded group | Mean range ms | p99 range ms | Per-run maximum range ms | Rows >16.7 / >33.3 / >50 / >100 ms |
| --- | --- | --- | --- | --- |
| Candidate06, six runs | 8.3365–8.4063 | 9.2011–10.7592 | 21.2993–32.7854 | 8 / 0 / 0 / 0 |
| Candidate07, six `none` runs | 8.3404–10.4848 | 9.3278–15.1796 | 34.0832–47.0730 | 31 / 6 / 0 / 0 |
| Candidate07, seven `mixed` runs | 8.3349–10.6963 | 8.9235–15.7497 | 17.4733–53.0143 | 28 / 3 / 1 / 0 |

All **66,511 engine rows** remain: 44,389 Candidate07 and 22,122 Candidate06. Each run's initial row lacks valid legacy driver counters but remains in timing statistics. These actual runs have no zero-duration rows. All 19 maxima occur at frame 0; none was trimmed. Across all runs, 67 rows exceed 16.7 ms, nine exceed 33.3 ms and one exceeds 50 ms; none exceeds 100 ms. Threshold counts overlap. All nine rows above 33.3 ms are initial rows, but there are also **48 noninitial rows above 16.7 ms**, including 46 in Candidate07. The highest sample is [M4A1 / 18 mixed](c07_profile_m4a1_18_mixed/analysis.json), frame 0 at **53.0143 ms**; its mean/p99 are **10.6963/15.7497 ms**.

Requested enemy counts are not continuously maintained populations. The shared driver samples before replenishment: its exact-live fraction ranges from **62.48%–94.96%** in Candidate07 and **60.80%–88.34%** in Candidate06. Valid denominators and full histograms remain in the reports. Candidate07's separate census samples after replenishment and retains **79,190 rows**, including **44,402 CSV-active rows**. All 44,389 engine frames match census identities; 13 extra terminal census rows are also retained. The two sampling points and denominators must not be conflated.

`none` means no explicit fixture falls. The 870 `none` runs still contain natural living falls: **2,498 / 2,824 / 2,021** CSV-active rows contain a fallen actor at requested counts 6/12/18, with maxima of **1/4/4** simultaneously fallen. These are not exclusively standing-actor benchmarks. They contain no CSV-active get-up samples; that limited observation does not prove recovery failure.

All seven mixed cases contain get-up samples. They record **29 accepted explicit falls from 34 attempts**: 2/2, 2/2, 6/6, 6/6 and 4/6 for M4A1 at 1/2/6/12/18; 5/6 for Overcurrent / 12; and 4/6 for 870 / 12. State occupancy counts mean a row contains at least one actor in a state. States can coexist, so these are neither disjoint time fractions nor completed-recovery totals. Reported maxima across Candidate07 include four fallen, two getting up, 14 dead and 414 simulated/awake infected leader-mesh bodies. The last count excludes separately tracked debris and other world bodies. Voice counters do not establish audibility.

Every run records **1600×900, cap 120, VSync 0 and screen percentage 100**. The six pairs match CPU, OS, engine version, build configuration and device profile. The 38 before/after hardware inventories agree: Ryzen 9 5950X, 32 logical processors, RTX 3090 present in the GPU inventory and Windows 11 Pro build 26200; virtual-display adapters are also present. These reports do not provide GPU timing attribution.

Runs were recording-free, capped and scripted, with requested-weapon combat, machine/pickup activity, corpse/debris physics and disclosed health protection. Both machines were active for 3.2073–3.2278 profile seconds. Candidate order alternates within the six pairs, but cache state, random streams, actual population histories and background load remain uncontrolled. Anonymous matched-process boundary CPU-seconds per wall-second range from 0.05287 to 0.16552, highest in Candidate07 M4A1 / 18 `none`. This covers the whole child invocation, omits new/retired/unreadable processes and is not total host utilization or a basis for subtracting background cost.

There is one run per case. No confidence interval, causal system cost, universal frame rate or performance improvement follows. Inclusive CPU scopes and worker scopes remain separate. These measurements do not constitute native free-play, continuous visual review, audio audition or user acceptance.
