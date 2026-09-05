# Current state — Candidate02

Candidate02 continues the accepted Candidate01 containment arena with two carried weapons and an explicit developer sandbox. Review branch: `codex/candidate02`. The `candidate01` tag and retained Candidate01 Windows package provide rollback points.

**Release record is pending final verification.** Packaged gameplay source revision, final published revision, candidate tag, and download URL must be filled from the completed build and remote checks. They are not established by this development snapshot. See the [Candidate02 pass report](Passes/Candidate02.md); [Candidate02 evidence](../Evidence/Candidate02/) is the planned destination for final checks and genuine gameplay captures. Those links may be populated later in this pass.

## Implemented systems

| Weapon | Loaded / initial reserve | Firing | Reload |
| --- | --- | --- | --- |
| AR-01 carbine | 24 / 192 | Automatic; 0.16-second cadence; 32 base damage | 2.10 seconds; magazine ammunition transfers at 1.20 seconds |
| SG-01 pump shotgun | 6 / 36 | One shot per press; eight 15-damage pellets; 0.22-second recoil followed by 0.56-second pump | 0.35-second opening, 0.90 seconds per shell with insertion at 0.60, then 0.32-second closure |

Definitions expose damage, spread, cadence, capacity, reserves, range falloff, operation timing, animation/audio references, and trauma tuning. Shotgun spread is 4 degrees; damage is full through 5 m and falls to 20% at 14 m. Carbine range is 28 m, with falloff after 14 m. Later rounds resupply both carried slots: 48 carbine rounds and eight shotgun shells, capped at 270 and 60 reserve respectively.

Switching takes 0.36 seconds and changes the visible weapon at 0.18. Each slot retains ammunition and any unfinished pump obligation. Switching cancels unfinished reload events, held fire, audio, and muzzle effects. Ammunition already inserted stays earned. A spent shotgun case can eject only once even if pumping is interrupted. A short fire tap interrupts shell loading, closes the reload, and fires one earned shell when ready. Pause clears queued fire and freezes the operation clock; death cancels operations. Developer refill cancels operations and restores both weapons to ready. Restart reconstructs the encounter and both weapons.

The native animation graph retains directional lower-body locomotion while weapon ready, fire, reload, pump, and equip clips affect the upper body. The shotgun fore-end, held loading shell, seated/held carbine magazine, mechanical sounds, and ammo transfers follow operation events. Shot variants, empty clicks, handling sounds, and flesh/concrete/metal impacts use original Project ONE audio.

There is one infected archetype with navigation, timed right-arm attacks, cooldown-limited hit reactions, and a heavier reaction for substantial nonlethal blasts. Head trauma reaches a lethal sever threshold at 32; removable-arm trauma reaches 50, with arm hits applying 40% health damage. Arm loss can survive and uses the remaining-arm attack. Pellet traces aggregate into one transaction per victim per discharge; duplicate IDs and corpse hits are rejected. A blast can detach at most one part per victim, with lethal head loss taking priority. Intact deaths remain possible. Registered kills award 100 points once.

Blood, corpses, detached parts, and ejected cases are bounded and expire. Detached meshes preserve the animated pose and have no blocking collision. The HUD reads both slots, the equipped weapon, operation progress, health, encounter counts, points, and hit/kill feedback directly from gameplay state.

## Actual controls

| Input | Action |
| --- | --- |
| WASD / mouse | Move / aim independently |
| Left Shift | Run; reload limits movement to walking speed |
| Left mouse / R | Fire / reload when available |
| 1 / 2 | Select carbine / shotgun |
| Tab or either mouse-wheel direction | Cycle the two carried weapons |
| Escape | Pause/resume |
| Enter / Q | Restart / quit while paused or after death |
| F1 | Toggle developer sandbox; reloads the encounter |
| F2 / F3 | In sandbox, spawn one / up to six registered infected |
| F4 | In sandbox, refill and ready both weapons |
| F5 | Reset sandbox encounter, player, score, and ammunition |
| F6 | In sandbox, clear blood, corpses, detached parts, and spent cases; living infected remain |

Normal rounds remain available. Sandbox disables automatic waves, retains the same combat and scoring systems, caps living infected at 18, and draws 0/2/5/10 m lane references in the existing arena. Existing obstacles provide pursuit and corner tests.

## Verification status at this development snapshot

| Area | Recorded status |
| --- | --- |
| UE 5.7.2 editor build | Passed before the final round-resupply edit; rebuild required for that edit |
| Native automation | Three `ProjectONE.Combat` tests passed: monotonic health/death, packet idempotence, and regional trauma boundaries |
| Candidate02 asset import | Passed; new meshes, clips, and event audio imported |
| Runtime integration and Candidate01 regression checks | Underway; final results belong in the pass report |
| Normal-camera motion inspection and gameplay audio audition | Pending final review; event dispatch alone does not establish sound quality |
| Candidate02 packaging, packaged checks, and performance | Underway; no final result claimed here |
| Remote publication, LFS retrieval, fresh-checkout build, prerelease download | Pending final verified references |

## Review map and limits

- [Weapon definitions](../Source/ProjectONE/ONEWeaponTypes.h) and [operation/damage dispatch](../Source/ProjectONE/ONEWeaponComponent.cpp) own the two slots, event clock, sounds, traces, and bounded cases. [Player](../Source/ProjectONE/ONEPlayer.cpp) binds input and attachments; [animation](../Source/ProjectONE/ONEAnimInstance.cpp) blends the authored clips.
- [Infected](../Source/ProjectONE/ONEZombie.cpp) owns regional damage, reaction/attack state, and severing. [Game mode](../Source/ProjectONE/ONEGameMode.cpp) owns authoritative live registration, score, rounds, and sandbox reset. [Blood subsystem](../Source/ProjectONE/ONEBloodSubsystem.cpp) owns presentation limits and cleanup.
- [Candidate02 asset inventory](../ArtSource/Weapons/Candidate02/inventory.json) and [asset notes](../ArtSource/Weapons/Candidate02/README.md) map imported binaries to editable sources and event timings. [Provenance](Provenance.md) and the existing [character](CharacterPipeline.md) and [environment](EnvironmentPipeline.md) records explain the retained pipeline. Project Zero content remains excluded.
- Weapon audio is locally authored synthesis, not a commercial recording pack. Convincing realism and final mix acceptance remain provisional until gameplay audition. Final visual acceptance, grip/contact assessment, and performance are separate from build/unit-test success.
- The existing imported rig reflects source Y: anatomical bone names retain source labels, and strafe assignment compensates at runtime. This pass retains the existing faces, garments, arena, authored death animation, and blood presentation; it does not claim a complete character or effects redesign.
- Future work is proposed only: a focused pass based on the user's Candidate02 combat, animation, and audio feedback. No further weapons, enemy archetypes, perks, or full-map expansion are implemented or scheduled by this record.
