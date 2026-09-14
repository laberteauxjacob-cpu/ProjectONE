# Retained low-aim compatibility failure

**Known failure; WIP Editor evidence, outside the final packaged PASS totals.**

The actual retained run `Motion_20260914T082618`, capture `20260914_082625_8993FD54`, completed 90 assertions: **89 passed and one failed**, with 1,484 recorded frames and zero headless-cursor fallback calls. Process exit 0 did not make the failed gameplay assertion pass.

Ten ordinary Left Ctrl + LMB corpse discharges (shot counters 5–14) all reported Miss and zero corpse-hit transactions. The low plane and selected cursor stayed at world **Z 68.474 cm**, while the evaluated body-query top ranged **61.743–62.698 cm**. The plane was therefore 5.776–6.731 cm above that bound at the recorded shots. These values retain the log's 0.001 cm precision.

The failed assertion is preserved verbatim:

> Later real Left Ctrl plus LMB corpse trace stayed cosmetic and did not repeat kill feedback; a fixed low-plane miss remains a failure

The ten LMB presses are accepted in the original input CSV, each immediately preceding its discharge frame. Held Ctrl=1, LMB=1, Low selection and each miss are present in the diagnostic stream; matching pose rows retain the same shot/outcome. Original modifier `handled=0` return values remain intact: processed held-key state is independently confirmed at discharge. No control was substituted, region enlarged or ray snapped to make this check pass.

This was a dirty Editor `-game` run, not the final package. **Source commit is null.** Observed HEAD was `9cd876f72a307f905a160bc19b2a965e26f5d26b`; the protected source snapshot `b3ff60488bb7428f27e4cdd7c9eb6f45fc9e335f12e9ed2e4505fa43a42459a2` and Editor module `4cbc9799f842bbdaea7ab028cefac6359e9094c1b66058f2bfbba3f1ff098ed9` were recorded unchanged before/after. That observation does not prove a clean build or bind these samples to final S. The JSON records selected historical source identities, the module identity, exact retained report/log hashes, every original assertion and all ten diagnostic/input correspondences.

This sequence demonstrates an actual fixed-low-plane targeting limitation. Direct analytical region intersection tests do not establish that ordinary mouse aim can reach this pose. The scope is one scripted corpse sequence; no additional image review, continuous playback, frame/audio decode or perceptual audition occurred here. Original frames/WAV remain private and are not repackaged as a passing movie.

[Machine-readable evidence](low_aim_compatibility.json) keeps the failure and binding limits explicit.
