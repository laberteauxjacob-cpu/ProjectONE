# Candidate05 input and combat rules

These rules supersede Candidate03/Candidate04 sprint-cancel and deferred-fire
behavior. Historical pass reports describe their own preserved builds.

## Aim and firing

The character-centered cursor direction controls facing and shot intent.
Inside a four-centimeter center radius, the last valid direction remains stable.
For an infected region under the cursor, the camera ray selects an interior
point halfway between entry and exit through the same collision primitive.
The reverse hit must retain the same bone; otherwise the original entry point
remains. This preserves the cursor ray rather than aiming at an actor center.

A target ahead of the evaluated muzzle retains its actual convergence point,
subject to the 45-degree yaw and 35-degree pitch bounds. Only a target at or
behind the muzzle uses a virtual point farther forward. Collision still checks
the physical shoulder-to-muzzle segment first. If it is clear, each pellet also
checks a short shoulder-origin contact segment toward the selected height,
with matching pellet spread, ending at the actual barrel's forward plane.
A clear contact segment falls through to the ordinary muzzle ray.

Each pellet resolves only one of these paths. World geometry blocks either
prefix, including low cover crossed by the downward contact segment even when
the muzzle itself clears that cover. This conservative rule prevents shooting
through nearby cover. Prefix hits terminate that pellet; ordinary forward
Last Word rays can still penetrate one additional victim at reduced damage.
There is no radius-based target search or extra damage from combining paths.
An impact behind the muzzle produces no backward tracer.

Pistol and shotgun presses are assessed immediately. An eligible short tap gets
exactly one post-pose dispatch, including a tap released before that dispatch.
A rejected cooldown, pump, equip, reload or handoff press never waits for a later
state. The dispatch rechecks the same owned instance and current eligibility.

M4A1 holds establish an automatic burst at a configured 0.100-second interval
(600 rounds/minute). Overcurrent retains its 15-percent improvement:
0.0869565 seconds (690 rounds/minute). Frame quantization may vary individual
intervals. Only an established burst carries its fractional timing remainder;
a hitch never causes multiple owed discharges in one frame. Release stops the
burst. Reload, equip, pause, input flush, handoff and death disarm it; continuing
requires release and a fresh eligible press.

## Reload commitment

Magazine reloads finish once started. Shift, firing, slot selection, cycling,
repeated R, acquisition, upgrade handoff and sandbox refill cannot cancel or
bank a switch through them. Movement and sprint remain available. Earned
magazine-out and magazine-in events still occur once. Death, reset and explicit
lifecycle cleanup can invalidate the operation and its remaining callbacks.

The equipped empty weapon reloads automatically when reserve exists and any
pump obligation has completed. Holstered weapons do not reload themselves.
Dry-fire feedback requires a deliberate press with both loaded and reserve
ammunition empty, and is rate limited to avoid repeated clicks every frame.

Shotgun shell transfers are earned only at their authored events. A press during
shell loading with an earned shell and no pump obligation closes the reload;
the support hand returns before another eligible press can shoot. That closing
press does not become a delayed shot. An unearned shell cannot be fired, and
every required pump still completes.

## Feedback and attacks

Damage returns an explicit live hit, new kill, corpse hit or rejected outcome.
Only live hits and new kills refresh the hit marker, with a distinct kill shape.
Corpse contact cannot repeat a kill, health transaction or score award. Pellets
aggregate into one transaction per victim per discharge. Last Word's ordinary
forward ray permits one additional victim; prefix hits and world cover stop
penetration.

Minor directional reactions remain additive to pursuit and committed attacks.
Heavy reactions keep their threshold and cooldown. Three attack families use
authored windup, a bounded swept step, one contact and recovery. Their heading
commits at attack start. Walls, a missed arc, missing required limbs and death
prevent inappropriate contact. Base attack damage remains 19, initial range
88 cm and player protection 0.55 seconds. Player walk/run remain 225/370 cm/s;
infected shamble/pursuit remain 100/195 cm/s.

## UI and lifecycle

H opens the help/sandbox tray; F1 retains its encounter-mode toggle. Mouse UI
actions require a press and release on the same currently enabled button in
the same context. Those events are consumed before the firing action mapping.
Pause, death, reset and UI transitions flush held gameplay input. The pointer
replaces the gameplay crosshair while a menu or tray is active.

The box reel observes the paid machine transaction, actual preview family and
cycle count. Its ready state shows the same physical reward; collection still
requires a nearby fresh F hold. The existing 950/5000 prices, five/nine-second
processing, two owned slots, reserved instance and one upgrade tier remain.

Weapon, zombie and ambient group gains are editable through `one.Audio.Weapons`,
`one.Audio.Zombies` and `one.Audio.Ambience`. Held upgrade effects use one mesh and
one bounded nonshadow light attached to the actual weapon. Base, holstered,
unowned and handoff-suppressed states disable them.

Implementation, runtime tests, recordings, motion review, auditory review and
release verification are reported separately in the Candidate05 pass report.
