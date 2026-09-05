# Candidate03 — player playtest corrections

Work in progress on `codex/candidate03`, from preserved `candidate02`
(`aa4d55d04bf375bbef41362af77eec10d9ea224f`). Candidate02's package is retained.
The consolidated Candidate03 assignment governs this pass; no duplicate pass,
new map, weapon roster, perk system or infected archetype is being introduced.

## Stage record

- A: actual source, branch, package and direct gameplay baseline inspected.
  See [baseline observations](../../Evidence/Candidate03/baseline.md).
- B: movement, turning, reload intent and weapon access implemented and under
  the final rendered turn-sequence review. Editable tuning: walk 225 cm/s,
  sprint 370, infected pursuit 195. All 48 directional speed trials passed
  (both weapons, eight directions, walk/sprint/reload walk), alongside 85
  weapon checks including frame-spaced production input bindings.
- C: weapon flash, audio and visible case improvements pending Stage B checks.
- D: ragdoll, broader regional severing, bleeding and contact pending earlier gates.
- E: combined packaged validation, motion/audio evidence and publication pending.

The final report will distinguish implementation, automatic checks, motion
inspection, perceptual listening and ordinary packaged input. No Candidate03
package or public release is claimed at this stage.

## Stage B implementation and evidence

The eight walk and eight run clips use actual speed and authored stride length;
the old diagonal phase multiplier and sprinting side/back walk substitutions are
removed. Two stepping turn clips, independent upper-body aim and world-space
foot preservation handle stationary turning. Skeletal evaluation waits for the
player's current movement/aim update. The final focused turn capture corrected
an earlier 18 cm one-frame foot snap on abrupt 180-degree aim changes. Continuous
90/180-degree-per-second sweeps, settled jumps and mid-step reversals were
captured separately from the earlier complete directional run.

Shift takes precedence over manual and automatic reload requests, including
while stationary. Reload cancellation processes only legitimately elapsed
ammunition events, clears obsolete presentation and buffered fire, and preserves
shotgun pump/ejection obligations. An empty equipped weapon with reserve reloads
once eligible; held Shift, pause, death and incompatible operations defer it.
The two slots now say **5.56 mm Carbine** and **12-Gauge Pump Shotgun**, with
1/2, Tab and wheel selection displayed together.

These are editor-game integration results, not final packaged results. Native
ordinary input verified weapon selection, mouse fire/aim, reload, pause and
quit. The available native automation delivered W taps within one engine frame
and did not deliver Shift key edges; sustained movement and Shift interruption
are therefore covered by automated production bindings and movement probes,
not claimed as native held-key playtesting. Directional motion review still
identifies deep knee compression and a mechanical cadence as provisional art.
Audio has not been perceptually auditioned.

The unchanged Candidate02 baseline was profiled at 1600x900 with 6/12/18 enemies.
[Timing reports](../../Evidence/Candidate03/Performance/C02/summary.json) retain
the benchmark's screenshot-associated spikes and separate individual Chaos CPU
scopes from frame duration. Candidate03 comparison follows the final package.
