# Candidate04 weapon audio source

This folder contains 24 original shot variations: six each for M1911, Last Word, Overcurrent and Gravebreaker. Four additional files provide pistol empty, magazine-out, magazine-in and slide events. M4A1 and Remington 870 retain the original Candidate03 base shot banks; their existing mechanical events remain separate.

Generate with `python Scripts/create_candidate04_audio.py`, then verify with `python Scripts/create_candidate04_audio.py --check`. Use Python 3. The generator and its referenced Candidate03 synthesis primitives are the editable source; no recordings, third-party samples or external services are inputs. Upgraded sounds add independently synthesized transients and tails to the firearm layers without pitch-shifting a whole recording. Shotgun shots contain no delayed pump action.

All exports are 48 kHz mono PCM16. `manifest.json` records each source/asset mapping, SHA-256, profile and measured signal properties. The check regenerates and compares every PCM sample. Source headroom and variation checks do not certify perceived sound quality; listening and in-game mix judgment remain separate.

`Scripts/import_candidate04_audio.py` imports these 28 files into `/Game/ONE/Audio/Weapons/Candidate04`. Unreal commandlet import requires `-AllowCommandletAudio`. Run imports only through the project's coordinated engine workflow.
