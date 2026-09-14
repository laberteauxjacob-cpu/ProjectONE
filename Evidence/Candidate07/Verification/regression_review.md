# Candidate07 completed behavioral regressions

Source S: `3d75c2faa0cecef6075f00755d8e4f5dd2562817`. Five completed packaged runs were reviewed from immutable matrix revision 18. **5,941 assertions passed; zero failed.** The matrix was still running at that snapshot.

| Native fixture | Passing assertions | Observed scope | Log warnings |
| --- | ---: | --- | ---: |
| Combat | 2,119 | Six weapons, 13 scenes each; 78 discharges / 260 projectiles | 19 |
| Projected aim | 2,708 | 480 discharges: six weapons × eight headings × ten cases | 21 |
| Grouping | 920 | 18 trials / nine pairs; 108 discharges / 234 projectiles | 19 |
| Survival | 99 | Regeneration, three pickups, traced modifiers, timing and cleanup | 19 |
| Progression | 95 | Box/upgrade lifecycle, rollback, expiry, death and real restart | 39 |

Every original assertion was replayed against its native report, engine stream and completion. Result/log/numeric artifact hashes match the immutable ledger evidence. The JSON retains full S, exact bindings and the runner's six common runtime identities. This review did not reread large runtime or capture payloads. Inspected neutral source bytes match the recorded snapshot; the working checkout differs only in LF/CRLF transport where recorded.

## Observed behavior

Combat used logical cursor projection and controller LMB input with no aim override. Modifier-only checks did not fire; Ctrl took priority. Actual discharges divide into 60 torso at 125 cm, 12 head at 168 cm and six low at 65 cm above the player's floor. Sampled-ray, cover, damage, penetration and per-victim award checks passed. In this run's normal 112 HP three-target crowd, the 870 killed the first target and left 40/112 HP; Gravebreaker killed the first two and left the third at **51.160 HP**.

Projected aim used the ordinary torso plane. The cumulative counter advances from 1 through 480, once per row; all pose/discharge frame identities match. Close, center and cover cases use normal standing targets. This does not test all aiming planes or floor-level corpse usability.

Grouping retains 234 projectile rows and 117 contact rows; every projectile reached its wall. Paired empty/enemy initial origins and directions have zero recorded error; all 18 trials recover to zero bloom. At walls 300/650/1,000 cm from the player, empty-scene Y/Z diagonal spans are 2.393/6.260/10.128 cm for M4A1 three-shot bursts, 5.340/13.972/22.605 cm for twelve-shot bursts and 36.273/97.772/159.271 cm for three 870 shells. Fixed seed, pose, direct refill and high-HP setup are declared; width is a bounding-box diagonal, not a fitted circle or natural-play accuracy measure.

Survival checks the accepted-damage 15-second delay, protected/rejected-damage exclusion, 200-maximum recovery at 20 HP per gameplay second, pause/death clocks, three pickup kinds, wall rejection, refresh/expiry/cap and cleanup. Actual collected Insta-Kill and Double Points precede a normal torso pistol trace into a healthy 112 HP target: one new body kill and **220 points**. Forced probes do not statistically measure the 1% production drop probability.

Progression measured **9.0002 seconds** of processing. It preserves same-slot return, usable second slot, family exclusion, earned reload/pump state, once-only technical refund, permanent expiry, death cancellation and prior-run invalidation after an actual level restart.

## Warnings and limits

All five logs contain zero Error/Fatal severity events. The 117 warnings comprise 108 missing weapon_r/hand_l lookups on unassigned skeletal components, six RecastNavMesh/CrowdManager initialization warnings, two material warnings and one expected machine-recovery probe. These are retained; the runs are not warning-free.

**M_EyeDark lacks instanced-static-mesh usage and falls back to the default material** in the aim log. ONE06ImpactSubsystem uses it for finite world-impact cylinder marks, with collision/navigation/shadows disabled. It is not the Candidate07 infected-eye material. No visual judgment of that fallback was made.

The separately retained fixed-low-plane corpse miss remains unresolved. These projected-torso and standing head/low trials do not prove mouse targeting of flattened bodies below 65 cm. Fixtures use scripted input, not native human play. No pixels, continuous playback, audio/audition or performance were reviewed here. Later matrix entries are excluded.

[regression_review.json](regression_review.json) retains exact bindings and metrics; raw host paths and logs remain private.
