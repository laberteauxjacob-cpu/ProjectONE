"""Refresh only twelve accepted FBX import records after privacy sanitation.

Run with UnrealEditor-Cmd -run=pythonscript -AllowCommandletAudio.
The FBX sanitizer proved all nonmetadata FBX properties unchanged. This script
does not import or rebuild art. Installed UE5.7 AssetImportData.cpp confirms
scripted_add_filename updates SourceData filename/timestamp/MD5 only. Original
packages are backed up under ignored Saved before metadata is saved.
"""
from pathlib import Path
import datetime, hashlib, json, re, shutil
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
LIB = u.EditorAssetLibrary
EXPECTED = {
    'A_Infected_Attack', 'A_Infected_AttackOneArm', 'A_Infected_Death',
    'A_Infected_Hit', 'A_Infected_Idle', 'A_Infected_Run', 'A_Infected_Walk',
    'SK_Infected', 'SK_Infected_ArmL', 'SK_Infected_Head',
    'SM_Infected_ArmL', 'SM_Infected_Head',
}
accepted = json.loads((ROOT/'Evidence/final_source_sync.json').read_text())['sources']
entries = [entry for entry in accepted if Path(entry['source']).stem in EXPECTED]
if len(entries) != 12 or {Path(entry['source']).stem for entry in entries} != EXPECTED:
    raise RuntimeError('Expected exactly the twelve sanitized Candidate01 FBXs')
run = ROOT/'Saved/PublicationAudit'/('ImportMetadata_'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ'))
run.mkdir(parents=True, exist_ok=False)


def xyz(value):
    return [value.x, value.y, value.z]


def transform_values(value):
    return xyz(value.translation) + [value.rotation.x, value.rotation.y, value.rotation.z, value.rotation.w] + xyz(value.scale3d)


def invariant(asset):
    """Read-only visible asset invariants; the sole mutation targets import data."""
    result = {'class': asset.get_class().get_name()}
    if isinstance(asset, u.AnimSequence):
        length = asset.get_play_length()
        tracks = u.AnimationLibrary.get_animation_track_names(asset)
        result.update(duration=length, skeleton=asset.get_editor_property('skeleton').get_path_name(),
                      tracks=[str(name) for name in tracks])
        poses = []
        for fraction in (0, .25, .5, .75, 1):
            poses.extend(transform_values(pose) for pose in u.AnimationLibrary.get_bone_poses_for_time(asset, tracks, length*fraction, False))
        result['sampled_pose_sha256'] = hashlib.sha256(json.dumps(poses).encode()).hexdigest()
    elif isinstance(asset, u.SkeletalMesh):
        bounds = asset.get_imported_bounds()
        result.update(skeleton=asset.get_editor_property('skeleton').get_path_name(),
                      bounds_origin=xyz(bounds.origin), bounds_extent=xyz(bounds.box_extent), radius=bounds.sphere_radius,
                      materials=[(str(slot.material_slot_name), slot.material_interface.get_path_name() if slot.material_interface else None) for slot in asset.materials])
    elif isinstance(asset, u.StaticMesh):
        bounds = asset.get_bounding_box()
        result.update(bounds_min=xyz(bounds.min), bounds_max=xyz(bounds.max), triangles=asset.get_num_triangles(0),
                      materials=[(str(slot.material_slot_name), slot.material_interface.get_path_name() if slot.material_interface else None) for slot in asset.static_materials])
    else:
        raise RuntimeError('Unexpected target asset class: '+asset.get_path_name())
    return result


def import_info(package):
    text = package.read_bytes().decode('utf-8', errors='ignore')
    match = re.search(r'\[\s*\{\s*"RelativeFilename"[^\x00]+?\}\s*\]', text)
    if not match:
        raise RuntimeError('Serialized import metadata missing: '+package.name)
    return json.loads(match.group())


targets = []
for entry in entries:
    source = ROOT/entry['source']
    package = ROOT/('Content/'+entry['asset'][len('/Game/'):] + '.uasset')
    asset = LIB.load_asset(entry['asset'])
    if not asset or not source.is_file() or not package.is_file():
        raise RuntimeError('Missing accepted source/package: '+entry['source'])
    data = asset.get_editor_property('asset_import_data')
    filenames = list(data.extract_filenames())
    if len(filenames) != 1 or Path(filenames[0]).name != source.name:
        raise RuntimeError('Unexpected multi-source import data: '+entry['asset'])
    before_metadata = import_info(package)
    if len(before_metadata) != 1:
        raise RuntimeError('Unexpected source records: '+entry['asset'])
    before = invariant(asset)  # All APIs must work before any asset mutation.
    backup = run/'OriginalPackages'/package.relative_to(ROOT)
    backup.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(package, backup)
    targets.append((entry, source, package, asset, data, before_metadata, before))

records = []
for entry, source, package, asset, data, before_metadata, before in targets:
    label = before_metadata[0].get('DisplayLabelName', '')
    data.scripted_add_filename(str(source.resolve()), 0, label)
    after = invariant(asset)
    if before != after:
        raise RuntimeError('Art invariant changed before save: '+entry['asset'])
    if not LIB.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError('Unable to save import metadata: '+entry['asset'])
    after_metadata = import_info(package)
    actual = after_metadata[0]
    expected_md5 = hashlib.md5(source.read_bytes()).hexdigest()
    if actual['FileMD5'].lower() != expected_md5:
        raise RuntimeError('Refreshed source MD5 mismatch: '+entry['asset'])
    if re.search(r'(?i)(?:[a-z]:[\\/]|/(?:Users|home)/)', actual['RelativeFilename']):
        raise RuntimeError('Import metadata path was not made portable: '+entry['asset'])
    records.append({'source': entry['source'], 'asset': entry['asset'], 'old_md5': before_metadata[0]['FileMD5'],
                    'new_md5': expected_md5, 'relative_import_path': actual['RelativeFilename'],
                    'art_invariants_unchanged': True, 'invariants': after})

report = {'status': 'PASS', 'count': len(records), 'method': 'UAssetImportData.scripted_add_filename only; no asset reimport or mesh/animation/material setters',
          'invariant_scope': 'Material bindings, mesh bounds, static triangle counts, animation durations/tracks and five sampled poses; FBX semantic identity established by preceding sanitation audit',
          'assets': records}
output = ROOT/'Saved/Candidate02/SanitizedImportMetadata.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, indent=2))
u.log('SANITIZED IMPORT METADATA PASS:12 accepted asset source records updated; no art reimport')
