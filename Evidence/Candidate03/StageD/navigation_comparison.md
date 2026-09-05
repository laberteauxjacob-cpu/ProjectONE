# Navigation collision repair

The repaired live navigation graph connects the original corner, rack and bench fixtures to the main floor. Actor positions and rendered arena geometry were unchanged. These plots show captured Recast polygons, not gameplay screenshots.

| Captured graph | Before | After |
| --- | ---: | ---: |
| Polygons | 62 | 36 |
| Connected floor components | 6 | 1 |
| Separate elevated components | 2 | 0 |
| Main connected floor area | 339.53855 m² | 387.51545 m² |
| Cross-tile connections | 14 | 14 |
| Missing or asymmetric neighbor links | 0 | 0 |
| Serialized navigation radius / height | 35 / 144 cm | 32 / 185 cm |

The collision audit found seven simple hulls per inspected environment asset, including obsolete rotated hulls extending beyond current render bounds. The repair replaced these with one NDOP18 hull from current geometry, refreshed navigation collision and completed a navigation rebuild. The serialized agent settings were also aligned with project configuration; this comparison includes both changes. See the [before audit](EnvironmentCollisionBefore.json) and [repair readback](EnvironmentCollisionRepair.json).

The corner target `(-1140, 920, 98)` and rack target `(-1130, -895, 98)` were in isolated floor components before repair. Both now share the main floor with the unchanged approach reference `(-900, 750, 98)` and bench target `(-435, 10, 98)`. Coordinates are actor world positions in centimetres; polygon membership uses their horizontal position at floor height.

The separate functional run exercised phases 6–10 with `-ONE03PhysicalityCheck -ONE03PhysicalityStartPhase=6 -ONE03SpawnDiagnostics -nullrhi -nosound`. Corner, rack and bench scenarios each spawned 18/18 registered enemies, observed every enemy progress over 100 cm, and recorded real attack contact. Each reported nearest crowd separation 54 cm and maximum overlap 0 cm. The run completed 173 checks with zero failures. This headless result does not establish rendered appearance or audio quality.

[Before graph](navigation_before.json) · [Before plot](navigation_before.png) · [Before metrics](navigation_before_metrics.json) · [After graph](navigation_after.json) · [After plot](navigation_after.png) · [After metrics](navigation_after_metrics.json)
