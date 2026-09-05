# Candidate02 evidence

Start with the [pass report](../../Docs/Passes/Candidate02.md) for build provenance, scope, results and limits. The reports here come from actual Unreal builds and packaged gameplay; implementation, numerical checks, visual inspection and audio measurement are separate evidence.

| Evidence | Meaning |
| --- | --- |
| [Weapon comparison](WeaponComparison.mp4), [capture provenance](WeaponComparison.json) and [timestamps](comparison_frames.csv) | Genuine normal-camera gameplay frames with recorded engine master audio; held frames, no generated or interpolated motion |
| `carbine_combat.jpg`, `shotgun_pump.jpg`, `shotgun_reload.jpg`, `shotgun_combat.jpg` | Small unchanged frames from that capture |
| [Visual review](visual_review.md) | Specific chronological frame observations and limits |
| [Direct packaged review](manual_playtest.md) | Normal launch and keyboard/mouse controls, including a late spawn defect and its follow-up |
| [Final controls](manual_controls.jpg), [spawn positions](sandbox_spawn_initial.png), [pursuit](sandbox_spawn_pursuit.png) | Genuine final-package views of normal lighting and the corrected floor-spawn regression |
| [Combat checks](combat_checks.txt), [comparison checks](comparison_checks.txt) | Two-weapon runtime integration and moving-fire checks |
| [Legacy combat](legacy_combat_checks.txt), [legacy presentation](presentation_checks.txt) | Retained gameplay, attacks, movement and cleanup regressions |
| [Native tests](native_tests.json) | Three damage-state tests; packaged checks exercise actual meshes and traces |
| `benchmark_6.txt`, `benchmark_12.txt`, `benchmark_18.txt` | Separate recorded enemy-count performance runs with settings and cap |
| [Audio measurements](audio_metrics.json) | PCM levels, coverage and energy; no perceptual audition claim |
| [Fresh build](fresh_build.json), [fresh runtime audit](fresh_release_audit.json) | Published-source/LFS retrieval, installed-toolchain build and package integrity |
| [Release archive](release_archive.json), [release audit](release_audit.json), [SHA-256](ProjectONE-Candidate02-Windows.sha256) | Exact downloadable archive, content hashes and privacy/notice review |
| [Archive extraction and launch](release_smoke.json) | Manifest verification and observed startup; firewall-prompt and clean-machine limits |
| [Source synchronization](source_sync.json), [import](WeaponAssetImport.json), [source rig QA](WeaponSourceValidation.json), [metadata refresh](SanitizedImportMetadata.json) | Source-to-import correspondence and numerical checks; not artistic approval |

Raw engine/build logs, uncurated captures, crash/debug files, temporary checkouts and local audit backups remain excluded from public source history. The complete required source/assets are published; installed Unreal/Blender/compiler dependencies are documented separately.
