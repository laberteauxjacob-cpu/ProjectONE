"""Refresh exact C07 hashes only after a proven metadata-only sanitation report.

This never treats a changed geometry or sound file as an approved source.
Historical input hashes that do not match the report are retained.
"""
from pathlib import Path
import argparse
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('report', type=Path)
args = parser.parse_args()
report = json.loads(args.report.read_text(encoding='utf-8'))
assert report['applied']
mapping = {}
for record in report['files']:
    path = (ROOT / record['path']).resolve()
    assert path.is_relative_to(ROOT / 'ArtSource/Characters/Candidate07') or path.is_relative_to(ROOT / 'ArtSource/Exports/Candidate07')
    assert hashlib.sha256(path.read_bytes()).hexdigest() == record['sanitized_sha256']
    if record['changed']:
        assert any(record.get(k) is True for k in ('decoded_nonmetadata_bytes_identical', 'all_nontext_chunks_identical', 'all_other_parsed_properties_identical'))
        mapping[record['original_sha256']] = record['sanitized_sha256']

def replace(value):
    if isinstance(value, dict):
        return {k: replace(v) for k, v in value.items()}
    if isinstance(value, list):
        return [replace(v) for v in value]
    return mapping.get(value, value) if isinstance(value, str) else value

changed = []
for path in (ROOT / 'ArtSource/Characters/Candidate07').rglob('*.json'):
    old = json.loads(path.read_text(encoding='utf-8'))
    new = replace(old)
    if old != new:
        path.write_text(json.dumps(new, indent=2) + '\n', encoding='utf-8')
        changed.append(path.relative_to(ROOT).as_posix())
print(json.dumps({'metadata_only_hash_refresh': changed, 'hashes': len(mapping)}))
