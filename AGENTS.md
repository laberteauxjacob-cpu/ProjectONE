# Project ONE working rules

- Inspect the actual source, assets and `Docs/CurrentState.md` before changing behavior. Reports are evidence, not a substitute for source inspection.
- Continue this repository and its Blender/Unreal pipeline. Preserve working behavior, clean commits, candidate tags and previous packaged builds. Never overwrite an existing rollback tag or unrelated work.
- Keep all Project Zero assets excluded and leave Project Zero untouched. Reusing Project ONE's own accepted assets is encouraged.
- Keep each pass scoped to the user's requested milestone. Proposed weapons, enemies, perks and maps stay proposals until requested.
- Update CurrentState, the relevant pass report, asset inventory/provenance and README after meaningful changes. Record actual controls and limitations.
- Separate implementation, builds, automated checks, visual inspection, audio review and packaged testing. Never claim a check, audition or inspection that did not occur.
- Use portable paths. Keep private logs, credentials, local-only diagnostics, rebuildable output and packaged builds out of source history. Review outgoing files AND history before publication. Store required binary source assets with Git LFS and verify uploaded objects.
- Never select a broad code/art license, buy content, enable paid storage or raise spending limits without explicit approval. Account/repository visibility follows the user's request.
- Publish completed, technically verified passes to the authorized repository, respecting protections and others' changes. Use explicit branch and tag pushes. NEVER push `candidate01-local` or `codex/first-milestone`, and never use `--all`, `--mirror`, blanket `--tags`, or force-push; those local refs retain pre-publication history.
- Preserve previous public candidate tags. Use the default branch for the latest technically verified candidate when repository rules allow; otherwise identify the published branch/PR precisely.
- Report repository URL, exact review commit/tag/compare links, package source revision, release/checksum, fresh-clone verification and any local-only work. Documentation/evidence commits can follow the built source commit; do not require a commit to contain its own hash.
- Technical verification is separate from user approval of visual direction. Stop after the requested pass and give one focused recommendation.
- Build public playable releases from a fresh checkout in a neutral build path, inspect embedded compiler diagnostics for personal paths, include the Windows prerequisites and required runtime notices, and keep private logs/debug symbols out of release archives.
