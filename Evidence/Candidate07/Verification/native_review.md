# Candidate07 native capture attempt: blocked

**Status: BLOCKED_NATIVE_CAPTURE. Zero native gameplay input actions; zero usable images; normal Quit unverified.** This result is excluded from native pass counts.

The verified package from source `3d75c2faa0cecef6075f00755d8e4f5dd2562817` launched at 10:06:57.771 UTC on 14 September 2026 with the Containment sandbox map and requested 1600×900 window settings. The engine log confirms map initialization. The owner found the exact game window, but initial selection/activation/capture failed with `GetCursorPos` access denied (`0x80070005`). One fresh-selection retry failed in `IGraphicsCaptureItemInterop.CreateForMonitor` (`0x80070057`). Those are owner tool-console observations, not screenshots or an independently inspected game view.

After the blocked attempts, the owner verified the test process identity and start time and terminated that process at 10:09:07.479 UTC. No in-game Quit was exercised. The six runtime payload hashes were independently rechecked after termination and match the passing build receipt and prelaunch identities; the neutral source checkout remains clean at exact S.

[The machine-readable review](native_input.json) binds the actual launch, termination, log, pre/post runtime and build receipts without publishing host paths or process identifiers. No image, accepted native control, visual/audio approval, performance result, completed release audit or publication verification is claimed by this attempt.
