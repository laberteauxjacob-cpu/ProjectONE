"""Archive a complete cooked Windows candidate without modifying runtime bytes.

Candidate04 is the default; earlier candidates must be explicit and existing accepted
archives cannot be replaced. Python 3.10+ standard library. A neutral fresh build,
source-revision verification, binary privacy review and runtime QA are upstream
requirements: this tool does not perform or claim those checks.
"""
import argparse
import hashlib
import json
import pathlib
import re
import tempfile
import zipfile

ROOT = pathlib.Path(__file__).resolve().parents[1]
REQUIRED = ['ProjectONE.exe', 'ProjectONE/Binaries/Win64/ProjectONE.exe',
            'ProjectONE/Content/Paks/ProjectONE-Windows.pak',
            'ProjectONE/Content/Paks/ProjectONE-Windows.ucas',
            'ProjectONE/Content/Paks/ProjectONE-Windows.utoc',
            'Engine/Extras/Redist/en-us/vc_redist.x64.exe']
# These notices cover unused SDKs/editor importers or the unshipped VS2010
# redistributable. Their upstream documents carry unrelated personal metadata.
# Keep required runtime notices intact; do not redact legal notice bodies.
UNUSED_NOTICES = {
    'MarketplacePlugins/EOSOnlineSubsystem_EpicOnlineServicesSDK_License.pdf',
    'Logitech G SDK License Agreement.pdf',
    'NvidiaGameworks_License.docx',
    'UnrealStudio/DatasmithCADImporter_ThirdPartySoftwareNotices.pdf',
    'UnrealStudio/DatasmithImporter_ThirdPartySoftwareNotices.pdf',
    'VisualC++RedistributableforVisualStudio2010_LICENSE.pdf',
}


def inventory(package, catalog):
    for name in REQUIRED:
        if not (package/name).is_file():
            raise ValueError('Required runtime/prerequisite missing: '+name)
    if not catalog.is_dir():
        raise ValueError('Engine third-party notice catalog missing.')
    files = {}
    reserved = {'play.txt', 'build_manifest.json', 'distributionnotice.md'}
    for file in sorted(package.rglob('*')):
        if not file.is_file():
            continue
        rel = file.relative_to(package)
        if any(part.lower() in ('saved', 'intermediate', '.git') for part in rel.parts):
            continue
        if file.suffix.lower() in ('.pdb', '.log', '.dmp', '.obj') or file.name.lower().startswith('manifest_'):
            continue
        if not file.resolve().is_relative_to(package):
            raise ValueError('Package contains an external file link: '+rel.as_posix())
        if rel.as_posix().lower() in reserved or rel.parts[0].lower() == 'thirdpartynotices':
            raise ValueError('Package collides with generated release entries: '+rel.as_posix())
        files[rel.as_posix()] = file
    for file in sorted(catalog.rglob('*')):
        if file.is_file() and file.relative_to(catalog).as_posix() not in UNUSED_NOTICES:
            if not file.resolve().is_relative_to(catalog):
                raise ValueError('Notice catalog contains an external file link.')
            files['ThirdPartyNotices/'+file.relative_to(catalog).as_posix()] = file
    files['DistributionNotice.md'] = ROOT/'Docs/DistributionNotice.md'
    if not files['DistributionNotice.md'].is_file():
        raise ValueError('Distribution notice missing.')
    if len({name.lower() for name in files}) != len(files):
        raise ValueError('Case-insensitive archive entry collision.')
    return files


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--candidate', choices=('Candidate02', 'Candidate03', 'Candidate04'), default='Candidate04')
    p.add_argument('--package', type=pathlib.Path, help='Default: Packaged/<candidate>/Windows')
    p.add_argument('--source-revision', required=True, help='Verified full commit of the actual fresh build')
    p.add_argument('--engine-root', type=pathlib.Path, required=True)
    p.add_argument('--output', type=pathlib.Path, help='Default: Releases/ProjectONE-<candidate>-Windows.zip')
    p.add_argument('--report-ref', help='Public documentation ref; default: lowercase candidate tag')
    p.add_argument('--replace', action='store_true', help='Explicitly replace Candidate04 output and sidecars only')
    p.add_argument('--dry-run', action='store_true', help='Validate inventory and report a plan without writing any files')
    a = p.parse_args()
    if not re.fullmatch(r'[0-9a-f]{40}', a.source_revision):
        p.error('Source revision must be a full verified Git commit.')
    report_ref = a.report_ref or a.candidate.lower()
    if not re.fullmatch(r'[A-Za-z0-9_./-]+', report_ref):
        p.error('Report ref must be a public Git ref or commit.')
    package = (a.package or ROOT/'Packaged'/a.candidate/'Windows').resolve()
    output = (a.output or ROOT/'Releases'/f'ProjectONE-{a.candidate}-Windows.zip').resolve()
    catalog = (a.engine_root/'Engine/Source/ThirdParty/Licenses').resolve()
    other_candidates = {'candidate01', 'candidate02', 'candidate03', 'candidate04'} - {a.candidate.lower()}
    if any(part.lower() in other_candidates for part in package.parts):
        p.error('Package directory names a different candidate; select it explicitly.')
    if output.name != f'ProjectONE-{a.candidate}-Windows.zip':
        p.error('Output filename must identify the selected candidate exactly.')
    if any(part.lower() in other_candidates for part in output.parts):
        p.error('Output directory names a different preserved candidate.')
    if output.is_relative_to(package) or output.is_relative_to(catalog):
        p.error('Output must be outside the input package and notice catalog.')
    outputs = (output, output.with_suffix('.sha256'), output.with_suffix('.json'))
    if not a.dry_run and any(path.exists() for path in outputs):
        if a.candidate != 'Candidate04':
            p.error('Existing earlier release artifacts are preserved; choose a new output directory.')
        if not a.replace:
            p.error('Candidate04 output exists; use --replace only after checking the intended destination.')
    try:
        files = inventory(package, catalog)
    except ValueError as error:
        p.error(str(error))
    if a.dry_run:
        print(json.dumps({'candidate': a.candidate, 'archive': output.name,
                          'source_revision': a.source_revision, 'files': len(files)+2,
                          'required_runtime_files': REQUIRED,
                          'retained_notice_files': sum(name.startswith('ThirdPartyNotices/') for name in files),
                          'excluded_unused_component_notices': sorted(UNUSED_NOTICES),
                          'existing_output_files': sum(path.exists() for path in outputs),
                          'dry_run': True, 'files_written': 0}, indent=2))
        return
    manifest = {'candidate':a.candidate, 'source_revision':a.source_revision,
                'engine':'5.7.2', 'platform':'Windows x64', 'configuration':'Development',
                'runtime_binary_modified_after_build':False,
                'excluded_unused_component_notices':sorted(UNUSED_NOTICES), 'files':[]}
    if a.candidate == 'Candidate02':
        controls = '''WASD move; mouse aim; left mouse fire; R reload; Shift run.
1 carbine; 2 pump shotgun; Tab/mouse wheel cycle. Escape pause.
Enter restart and Q quit while paused or after death.
F1 sandbox; F2 +1 infected; F3 +6; F4 refill; F5 reset; F6 clear remains.'''
        limitations = 'Original synthesized audio and character/hand/foot polish remain provisional.'
    elif a.candidate == 'Candidate03':
        controls = '''WASD move; mouse aim; left mouse button fire; R reload; Shift run.
Shift immediately cancels a reload. An eligible empty weapon reloads automatically
when reserve ammunition is available, after any required shotgun pump.
1 carbine; 2 pump-action shotgun; Tab/mouse wheel cycle. Escape pause.
Enter restart and Q quit while paused or after death.
F1 sandbox; F2 add one infected; F3 add six; F4 refill; F5 reset;
F6 clear remains; F7 toggle bright/dim lighting.'''
        limitations = 'This is a candidate build; technical checks do not imply visual or audio approval.'
    else:
        controls = '''WASD move; mouse aim; left mouse fire; R reload; Shift sprint/cancel reload.
M4A1/Overcurrent are automatic; pistol and shotgun fire once per press.
1/2 select available owned slots; Tab/mouse wheel cycle. Escape pause.
Enter restart and Q quit while paused or dead.
Start: M1911 with 7 loaded / 56 reserve; second slot empty.
Hold F for 0.4 seconds at the focused machine; release before the next action.
Mystery Box: 950 points, five-second model cycle, deliberate collection.
Pack-a-Punch: 5000 points at physical acceptance; nine-second upgrade;
same instance/slot reserved until deliberate collection. One upgrade tier.
F1 sandbox; F2 +1 infected; F3 +6; F4 refill available weapons; F5 reset;
F6 cleanup; F7 bright/dim lighting. T grants 10000 labelled test points.
Z/X/C force next box pistol/M4A1/870; V restores random. Prices stay normal.
After a loaded pistol/rifle reload is cancelled with its magazine removed,
press R to insert a replacement before firing.'''
        limitations = 'Technical checks, saved-frame review and engine audio measurement do not establish user visual approval, perceptual listening or sustained native held-key testing.'
    launch = f'''Project ONE — {a.candidate}

Extract this entire folder and run ProjectONE.exe. Keep all subfolders together.
If Windows reports missing runtime prerequisites, run
Engine/Extras/Redist/en-us/vc_redist.x64.exe, then launch the game.

{controls}

{limitations}
See DistributionNotice.md and ThirdPartyNotices for runtime terms and credits.
Build source: {a.source_revision}
Full report: https://github.com/laberteauxjacob-cpu/ProjectONE/blob/{report_ref}/Docs/Passes/{a.candidate}.md
'''
    output.parent.mkdir(parents=True, exist_ok=True)
    prefix = f'ProjectONE-{a.candidate}-Windows/'
    # Build and CRC-check a new temporary archive before replacing an authorized C04 output.
    with tempfile.NamedTemporaryFile(dir=output.parent, suffix='.partial', delete=False) as temporary:
        pending = pathlib.Path(temporary.name)
    try:
        with zipfile.ZipFile(pending, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as z:
            for name, file in sorted(files.items()):
                data = file.read_bytes()
                manifest['files'].append({'path':name, 'bytes':len(data), 'sha256':hashlib.sha256(data).hexdigest()})
                z.writestr(prefix+name, data)
            z.writestr(prefix+'PLAY.txt', launch.encode('utf-8'))
            z.writestr(prefix+'build_manifest.json', json.dumps(manifest, indent=2).encode())
        with zipfile.ZipFile(pending) as z:
            bad = z.testzip()
            if bad:
                raise ValueError('Archive integrity failure: '+bad)
        checksum = hashlib.sha256(pending.read_bytes()).hexdigest()
        pending.replace(output)
    finally:
        pending.unlink(missing_ok=True)
    output.with_suffix('.sha256').write_text(checksum+'  '+output.name+'\n', encoding='utf-8')
    summary = {'candidate':a.candidate, 'archive':output.name, 'bytes':output.stat().st_size, 'sha256':checksum,
               'source_revision':a.source_revision, 'files':len(files)+2,
               'archive_crc_verified':True, 'runtime_binary_modified_after_build':False}
    output.with_suffix('.json').write_text(json.dumps(summary, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
