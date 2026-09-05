"""Refresh import records for the explicit Candidate03 Stage B animation set.

Run only at the root agent's Unreal process gate:
UnrealEditor-Cmd ProjectONE.uproject -run=pythonscript -script=this_script
 -unattended -nop4 -nullrhi

No FBX reimport or animation setters. UAssetImportData.scripted_add_filename
updates the filename/timestamp/MD5 record, with a portable serialized filename.
Require matching metadata-only sanitation proof before changing a stale hash.
Back up original packages under ignored Saved; compare animation invariants.
Later Stage C/D inventories must be added explicitly, never by broad discovery.
"""
from pathlib import Path
import datetime
import hashlib
import json
import re
import shutil

import unreal as u


ROOT = Path(__file__).resolve().parents[1]
LIB = u.EditorAssetLibrary
MANIFEST = ROOT / 'ArtSource/Characters/Candidate03/inventory.json'
DIRECTIONS = ('F', 'FR', 'R', 'BR', 'B', 'BL', 'L', 'FL')
EXPECTED = {f'A_Response_C03_{gait}_{direction}' for gait in ('Walk', 'Run') for direction in DIRECTIONS}
EXPECTED.update(('A_Response_C03_Turn_L', 'A_Response_C03_Turn_R'))
PRIVATE = re.compile(r'(?i)(?:[a-z]:[\\/]|/(?:Users|home)/)')


def digest(path, algorithm='sha256'):
    return hashlib.new(algorithm, path.read_bytes()).hexdigest()


def bounded(relative, area):
    path = (ROOT / relative).resolve()
    path.relative_to((ROOT / area).resolve())
    return path


def import_info(package):
    text = package.read_bytes().decode('utf-8', errors='ignore')
    match = re.search(r'\[\s*\{\s*"RelativeFilename"[^\x00]+?\}\s*\]', text)
    if not match:
        raise RuntimeError('Serialized import data absent: ' + package.name)
    records = json.loads(match.group())
    if len(records) != 1:
        raise RuntimeError('Expected exactly one FBX import record: ' + package.name)
    return records[0]


def validate_import_path(package, record, source):
    relative = record['RelativeFilename']
    if PRIVATE.search(relative) or (package.parent / relative).resolve() != source:
        raise RuntimeError('Unexpected or nonportable import source path: ' + package.name)


def animation_invariant(asset):
    if not isinstance(asset, u.AnimSequence):
        raise RuntimeError('Expected an animation sequence')
    duration = asset.get_play_length()
    tracks = u.AnimationLibrary.get_animation_track_names(asset)
    poses = []
    for fraction in (0, .25, .5, .75, 1):
        for pose in u.AnimationLibrary.get_bone_poses_for_time(asset, tracks, duration * fraction, False):
            poses.append([pose.translation.x, pose.translation.y, pose.translation.z,
                          pose.rotation.x, pose.rotation.y, pose.rotation.z, pose.rotation.w,
                          pose.scale3d.x, pose.scale3d.y, pose.scale3d.z])
    return {'duration': duration, 'tracks': [str(name) for name in tracks],
            'skeleton': asset.get_editor_property('skeleton').get_path_name(),
            'sampled_pose_sha256': hashlib.sha256(json.dumps(poses).encode()).hexdigest()}


def sanitation_proof(source, old_md5):
    """Require current sanitized bytes and the exact originally imported bytes."""
    relative = source.relative_to(ROOT).as_posix()
    current_sha = digest(source)
    reports = sorted((ROOT / 'Saved/PublicationAudit').glob('Metadata_*/report.json'), reverse=True)
    for report_path in reports:
        report = json.loads(report_path.read_text())
        if not report.get('applied'):
            continue
        for record in report.get('files', []):
            if (record.get('path') != relative or record.get('sanitized_sha256') != current_sha
                    or not record.get('all_other_parsed_properties_identical')):
                continue
            backup = (report_path.parent / 'OriginalMetadata' / relative).resolve()
            backup.relative_to((ROOT / 'Saved/PublicationAudit').resolve())
            if backup.is_file() and digest(backup) == record['original_sha256'] and digest(backup, 'md5') == old_md5:
                return report_path.relative_to(ROOT).as_posix()
    raise RuntimeError('No sanitation proof matching imported and current FBX bytes: ' + relative)


manifest = json.loads(MANIFEST.read_text())
if set(manifest.get('clips', {})) != EXPECTED or len(EXPECTED) != 18:
    raise RuntimeError('Expected exactly the explicit eighteen Stage B clips')
if manifest.get('skeleton') != 'SK_Response_Skeleton':
    raise RuntimeError('Unexpected Stage B skeleton contract')
run = ROOT / 'Saved/Candidate03' / ('ImportMetadata_' + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ'))
run.mkdir(parents=True, exist_ok=False)
targets = []
for name in sorted(EXPECTED):
    source = bounded('ArtSource/Exports/Candidate03/' + name + '.fbx', 'ArtSource/Exports/Candidate03')
    package = bounded('Content/ONE/Animations/' + name + '.uasset', 'Content/ONE/Animations')
    asset_path = '/Game/ONE/Animations/' + name
    if not source.is_file() or not package.is_file():
        raise RuntimeError('Missing Stage B source/package: ' + name)
    asset = LIB.load_asset(asset_path)
    if not asset:
        raise RuntimeError('Cannot load Stage B sequence: ' + name)
    record = import_info(package)
    validate_import_path(package, record, source)
    data = asset.get_editor_property('asset_import_data')
    filenames = list(data.extract_filenames())
    if len(filenames) != 1 or Path(filenames[0]).resolve() != source:
        raise RuntimeError('Extracted import path differs from inventory: ' + name)
    before = animation_invariant(asset)
    if abs(before['duration'] - manifest['clips'][name]['duration']) > .011:
        raise RuntimeError('Animation duration differs from current inventory: ' + name)
    if before['skeleton'] != '/Game/ONE/Characters/SK_Response_Skeleton.SK_Response_Skeleton':
        raise RuntimeError('Animation uses a different skeleton: ' + name)
    current_md5 = digest(source, 'md5')
    stale = record['FileMD5'].lower() != current_md5
    proof = sanitation_proof(source, record['FileMD5'].lower()) if stale else None
    backup = run / 'OriginalPackages' / package.relative_to(ROOT)
    backup.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(package, backup)
    targets.append((name, source, package, asset, data, record, before, current_md5, proof))

# Every source/path/proof/API is checked before the first package mutation.
results = []
for name, source, package, asset, data, record, before, expected_md5, proof in targets:
    if digest(source, 'md5') != expected_md5:
        raise RuntimeError('Source changed after preflight: ' + name)
    changed = record['FileMD5'].lower() != expected_md5
    if changed:
        data.scripted_add_filename(str(source), 0, record.get('DisplayLabelName', ''))
        if animation_invariant(asset) != before:
            raise RuntimeError('Animation invariant changed: ' + name)
        if not LIB.save_loaded_asset(asset, only_if_is_dirty=False):
            raise RuntimeError('Unable to save import metadata: ' + name)
    after = import_info(package)
    validate_import_path(package, after, source)
    if after['FileMD5'].lower() != expected_md5 or animation_invariant(asset) != before:
        raise RuntimeError('Post-save source/art check failed: ' + name)
    results.append({'source': source.relative_to(ROOT).as_posix(), 'asset': '/Game/ONE/Animations/' + name,
                    'changed': changed, 'old_md5': record['FileMD5'], 'new_md5': expected_md5,
                    'relative_import_path': after['RelativeFilename'], 'sanitation_proof': proof,
                    'art_invariants_unchanged': True, 'invariants': before})

report = {'status': 'PASS', 'stage': 'Candidate03 Stage B', 'count': len(results),
          'changed_import_records': sum(record['changed'] for record in results),
          'inventory_sha256': digest(MANIFEST),
          'method': 'UAssetImportData.scripted_add_filename only; no animation reimport or art setter',
          'invariant_scope': 'Duration, skeleton, track names and five sampled poses unchanged; sanitation proof matches all parsed FBX properties except the intended author-path field',
          'assets': results}
output = ROOT / 'Saved/Candidate03/ImportMetadataRefresh.json'
output.write_text(json.dumps(report, indent=2) + '\n')
u.log('CANDIDATE03_IMPORT_METADATA_PASS count=18 changed=' + str(report['changed_import_records']))
