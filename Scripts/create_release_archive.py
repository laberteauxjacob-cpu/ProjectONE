"""Package the complete cooked Windows runtime, notices and verified manifest.

Python 3 standard library. Does not modify the built executable or original
package. Omits private runtime output, debug symbols and staging manifests.
"""
import argparse
import hashlib
import json
import pathlib
import re
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--package', type=pathlib.Path, required=True)
p.add_argument('--source-revision', required=True)
p.add_argument('--engine-root', type=pathlib.Path, required=True)
p.add_argument('--output', type=pathlib.Path, required=True)
a = p.parse_args()
if not re.fullmatch(r'[0-9a-f]{40}', a.source_revision):
    p.error('Source revision must be a full verified Git commit.')
root = pathlib.Path(__file__).resolve().parents[1]
required = ['ProjectONE.exe', 'ProjectONE/Binaries/Win64/ProjectONE.exe',
            'ProjectONE/Content/Paks/ProjectONE-Windows.pak',
            'ProjectONE/Content/Paks/ProjectONE-Windows.ucas',
            'ProjectONE/Content/Paks/ProjectONE-Windows.utoc',
            'Engine/Extras/Redist/en-us/vc_redist.x64.exe']
for name in required:
    if not (a.package/name).is_file():
        raise SystemExit('Required runtime/prerequisite missing: '+name)
catalog = a.engine_root/'Engine/Source/ThirdParty/Licenses'
if not catalog.is_dir():
    raise SystemExit('Engine third-party notice catalog missing.')
files = {}
# These notices cover unused SDKs/editor importers or the unshipped VS2010
# redistributable. Their upstream documents carry unrelated personal metadata.
# Keep required runtime notices intact; do not redact legal notice bodies.
unused_notices = {
    'MarketplacePlugins/EOSOnlineSubsystem_EpicOnlineServicesSDK_License.pdf',
    'Logitech G SDK License Agreement.pdf',
    'NvidiaGameworks_License.docx',
    'UnrealStudio/DatasmithCADImporter_ThirdPartySoftwareNotices.pdf',
    'UnrealStudio/DatasmithImporter_ThirdPartySoftwareNotices.pdf',
    'VisualC++RedistributableforVisualStudio2010_LICENSE.pdf',
}
for file in sorted(a.package.rglob('*')):
    if not file.is_file():
        continue
    rel = file.relative_to(a.package)
    if any(part.lower() in ('saved', 'intermediate', '.git') for part in rel.parts):
        continue
    if file.suffix.lower() in ('.pdb', '.log', '.dmp', '.obj') or file.name.startswith('Manifest_'):
        continue
    files[rel.as_posix()] = file
for file in sorted(catalog.rglob('*')):
    if file.is_file() and file.relative_to(catalog).as_posix() not in unused_notices:
        files['ThirdPartyNotices/'+file.relative_to(catalog).as_posix()] = file
files['DistributionNotice.md'] = root/'Docs/DistributionNotice.md'
manifest = {'candidate':'Candidate02', 'source_revision':a.source_revision,
            'engine':'5.7.2', 'platform':'Windows x64', 'configuration':'Development',
            'runtime_binary_modified_after_build':False,
            'excluded_unused_component_notices':sorted(unused_notices), 'files':[]}
launch = '''Project ONE — Candidate02

Extract this entire folder and run ProjectONE.exe. Keep all subfolders together.
If Windows reports missing runtime prerequisites, run
Engine/Extras/Redist/en-us/vc_redist.x64.exe, then launch the game.

WASD move; mouse aim; left mouse fire; R reload; Shift run.
1 carbine; 2 pump shotgun; Tab/mouse wheel cycle. Escape pause.
Enter restart and Q quit while paused or after death.
F1 sandbox; F2 +1 infected; F3 +6; F4 refill; F5 reset; F6 clear remains.

Original synthesized audio and character/hand/foot polish remain provisional.
See DistributionNotice.md and ThirdPartyNotices for runtime terms and credits.
Build source: '''+a.source_revision+'''
Full report: https://github.com/laberteauxjacob-cpu/ProjectONE/blob/main/Docs/Passes/Candidate02.md
'''
a.output.parent.mkdir(parents=True, exist_ok=True)
prefix = 'ProjectONE-Candidate02-Windows/'
with zipfile.ZipFile(a.output, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6, allowZip64=True) as z:
    for name, file in sorted(files.items()):
        data = file.read_bytes()
        manifest['files'].append({'path':name, 'bytes':len(data), 'sha256':hashlib.sha256(data).hexdigest()})
        z.writestr(prefix+name, data)
    z.writestr(prefix+'PLAY.txt', launch.encode('utf-8'))
    z.writestr(prefix+'build_manifest.json', json.dumps(manifest, indent=2).encode())
with zipfile.ZipFile(a.output) as z:
    bad = z.testzip()
    if bad:
        raise SystemExit('Archive integrity failure: '+bad)
checksum = hashlib.sha256(a.output.read_bytes()).hexdigest()
a.output.with_suffix('.sha256').write_text(checksum+'  '+a.output.name+'\n')
summary = {'archive':a.output.name, 'bytes':a.output.stat().st_size, 'sha256':checksum,
           'source_revision':a.source_revision, 'files':len(files)+2,
           'archive_crc_verified':True, 'runtime_binary_modified_after_build':False}
a.output.with_suffix('.json').write_text(json.dumps(summary, indent=2)+'\n')
print(json.dumps(summary, indent=2))
