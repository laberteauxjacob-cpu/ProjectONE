# Current state — Candidate05 validation

Candidate05 continues the Candidate04 arena and ownership/economy foundation.
It replaces deferred firing with eligible post-pose dispatch and replaces
sprint-cancel magazine reloads with committed reloads. The
[current combat rules](CombatRules.md) supersede the historical input rules.

The shared aim solution retains character-centered intent near the cursor while
preserving real shoulder/muzzle collision. M4A1/Overcurrent are configured for
600/690 RPM. Live-hit/new-kill/corpse/rejected outcomes drive separate feedback.
Minor reactions blend over motion; three authored infected attack families use
fixed headings, bounded steps and one contact each.

The HUD uses original rounded glyph artwork and images rendered from actual
weapon assemblies. H opens a grouped tool tray; F1 keeps its mode/reset meaning.
The reel observes the paid box result. Pause/game over use real scene dimming,
mouse buttons and tracked survival time. Original spatial ambience/zombie sounds
and six-variation upgrade shot banks use bounded audio groups. Held
violet/cyan/ember effects follow the effective weapon.

## Verification at this source checkpoint

Local Editor and Win64 Development targets build. All 17 native tests pass:
14 without warnings, three historical fixtures with five teardown warnings.
All eight Candidate05 native tests are warning-free. The initial attack test
world-context failure was corrected in the fixture before the passing rerun.

Weapon checks pass 191 assertions each at requested 30/60/120 FPS, using real
weapon operations and evaluated-pose shots in a headless engine. Their declared
stress/loadout fixtures are not native input or performance proof. The first
four-heading arena aim probe passed 812 assertions; an expanded run passed
1,988. Those runs used an elevated target fixture that concealed real standing
torso height. They are superseded, not proof of corrected close-range aiming.
An actual rendered approach exposed misses. The correction preserves valid
forward convergence, picks inside the same anatomical primitive, and traces
contact per pellet only before the actual barrel plane, with world cover first.
Corrected all-variant standing-height checks pass 2,612 assertions over 480
shots. The projected-cursor version passes 2,564 over another 480 shots in an
actual offscreen viewport. Both use fixed 1/60-second simulation steps with
uncapped execution: collision evidence, not real-time cadence or native input.

All 25 new animations, nine textures and 48 sounds import. The engine asset
verifier confirms their references and all six definitions. Its first
metadata-reader failure was fixed without changing the assets.

Four actual offscreen viewports pass 49 UI assertions each at 1280x720,
1600x900, 1920x1080 and 2560x1080. The updated 1600x900 states were visually
reviewed. A rendered motion run passes 80 checks with 1,208 actual frames and
51.37 seconds of engine audio, after fixes for low pivot clearance and missed
standing-target shots. The [moving review](../Evidence/Candidate05/VisualReview.md)
covers 496 consecutive frames across strides, turns, attacks and hit outcomes.
Mechanical joint motion and similar swipe/rake silhouettes remain limitations.
The combined six-weapon/machine/menu recording passes 93 checks with 4,635
actual frames and 197.29 seconds of engine audio; its review is separate.

Perceptual listening, current-package runtime tests, performance and clean public
checkout/release verification remain pending at this checkpoint. No Candidate05
package or release completion is claimed. Final evidence will record the exact
packaged source separately from the later documentation revision.

## Preserved baseline

The last verified public Candidate04 revision is
[`2a75a4a5c09f52dacf289ced1b91d548ce3f2a3d`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/2a75a4a5c09f52dacf289ced1b91d548ce3f2a3d).
Its packaged source is
[`8055041ebc98a4df7cd8923b05e7b89ad7372e38`](https://github.com/laberteauxjacob-cpu/ProjectONE/commit/8055041ebc98a4df7cd8923b05e7b89ad7372e38).
Its [pass](Passes/Candidate04.md), [evidence](../Evidence/Candidate04/README.md), tag,
release and local package are preserved. Earlier reports describe their own
builds, including the reload policy now superseded by Candidate05.

See the [Candidate05 pass](Passes/Candidate05.md) and
[evidence index](../Evidence/Candidate05/README.md) for current scope and limits.
