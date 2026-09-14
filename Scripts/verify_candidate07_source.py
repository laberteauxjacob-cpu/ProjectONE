"""Read-only Candidate07 source/LFS gate for an already cloned public checkout.

No fetch, build, import, runtime test or publication is performed. The supplied
committed SourceAssets manifest must enumerate every Candidate07 LFS payload;
this gate verifies those bytes, not metadata privacy or legal conclusions.
"""
import argparse
import datetime
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess

REMOTE = 'https://github.com/laberteauxjacob-cpu/ProjectONE'


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    result = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            result.update(block)
    return result.hexdigest()


def portable(root, name):
    require(isinstance(name, str) and name and '\\' not in name and ':' not in name and not any(ord(c) < 32 for c in name),
            'Expected a portable repository-relative path')
    relative = PurePosixPath(name)
    require(not relative.is_absolute() and '..' not in relative.parts and str(relative) == name,
            'Noncanonical or escaping repository path: ' + name)
    path = (root / name).resolve()
    require(path.is_relative_to(root) and path.is_file(), 'Missing or external file: ' + name)
    return path


def verify(root, source, manifest_name):
    def git(*args, input_bytes=None):
        return subprocess.check_output(['git', *args], cwd=root, input=input_bytes)

    require(not any(p.casefold() in {'users', 'onedrive', '.codex'} for p in root.parts),
            'Use a neutral public build checkout')
    require(git('rev-parse', 'HEAD').decode().strip() == source, 'HEAD differs from expected source')
    require(not git('status', '--porcelain').strip(), 'Source checkout is not clean')
    require(git('remote', 'get-url', 'origin').decode().strip().removesuffix('.git') == REMOTE,
            'Unexpected origin URL')
    require(not os.environ.get('GIT_ALTERNATE_OBJECT_DIRECTORIES'), 'Environment object alternates are forbidden')
    alternates = Path(git('rev-parse', '--git-path', 'objects/info/alternates').decode().strip())
    if not alternates.is_absolute():
        alternates = root / alternates
    require(not alternates.exists() or not alternates.read_bytes().strip(), 'Local object alternates are forbidden')
    names = git('ls-files', '-z').decode('utf-8').split('\0')[:-1]
    require(names and len(set(n.casefold() for n in names)) == len(names), 'Missing or colliding tracked paths')
    for name in names:
        portable(root, name)
    attributes = git('check-attr', '--cached', '-z', '--stdin', 'filter',
                     input_bytes=('\0'.join(names) + '\0').encode()).decode().split('\0')[:-1]
    require(len(attributes) == len(names) * 3, 'Incomplete Git filter attributes')
    lfs_names = [attributes[i] for i in range(0, len(attributes), 3) if attributes[i + 2] == 'lfs']
    require(lfs_names, 'No tracked LFS payloads found')
    # The batch contains pointer blobs only, never the binary LFS payloads.
    batch = git('cat-file', '--batch', input_bytes=''.join(source + ':' + n + '\n' for n in lfs_names).encode())
    offset = 0
    rows = []
    for name in lfs_names:
        end = batch.find(b'\n', offset)
        require(end >= offset, 'Missing committed LFS object header: ' + name)
        header = batch[offset:end].split()
        require(len(header) == 3 and header[1] == b'blob', 'Missing committed LFS object: ' + name)
        size = int(header[2])
        require(0 < size < 1024, 'LFS-filtered source is not a pointer: ' + name)
        start = end + 1
        pointer = batch[start:start + size]
        require(batch[start + size:start + size + 1] == b'\n', 'Truncated LFS batch object: ' + name)
        offset = start + size + 1
        match = re.fullmatch(rb'version https://git-lfs.github.com/spec/v1\n(?:ext-[^\n]+\n)*oid sha256:([0-9a-f]{64})\nsize ([0-9]+)\n?', pointer)
        require(match is not None, 'Invalid committed LFS pointer: ' + name)
        expected_hash, expected_size = match[1].decode(), int(match[2])
        path = portable(root, name)
        require(path.stat().st_size == expected_size and digest(path) == expected_hash, 'LFS payload mismatch: ' + name)
        rows.append({'path': name, 'bytes': expected_size, 'sha256': expected_hash})
    require(offset == len(batch), 'Unexpected trailing Git batch content')
    manifest_path = portable(root, manifest_name)
    require(manifest_name in names, 'SourceAssets manifest must be committed')
    manifest = json.loads(manifest_path.read_text(encoding='utf-8-sig'))
    require(manifest.get('candidate') == '07' and manifest.get('status') == 'SOURCE_PAYLOADS_RECORDED',
            'Candidate07 SourceAssets checkpoint is incomplete')
    payloads = manifest.get('payloads', [])
    require(payloads and manifest.get('payload_count') == len(payloads), 'SourceAssets count does not match its rows')
    indexed = {r['path']: r for r in rows}
    listed = [r['path'] for r in payloads]
    require(len(set(n.casefold() for n in listed)) == len(listed), 'Duplicate source payload row')
    require({n for n in lfs_names if '/Candidate07/' in n} <= set(listed), 'SourceAssets omits Candidate07 LFS payloads')
    for row in payloads:
        require(row['path'] in indexed, 'SourceAssets payload must be tracked with LFS: ' + row['path'])
        require(all(row[k] == indexed[row['path']][k] for k in ('bytes', 'sha256')), 'SourceAssets hash/size differs: ' + row['path'])
    pattern = re.compile(r'IMPLEMENT_SIMPLE_AUTOMATION_TEST\s*\([^,]+,\s*"(ProjectONE\.[^"]+)"')
    registrations = []
    for name in names:
        if name.startswith('Source/ProjectONE/') and name.endswith('.cpp'):
            registrations.extend(pattern.findall((root / name).read_text(encoding='utf-8-sig')))
    require(registrations and len(registrations) == len(set(registrations)), 'Missing or duplicate source test registrations')
    require(not git('status', '--porcelain').strip(), 'Source changed during verification')
    return {'schema': 1, 'candidate': '07', 'status': 'SOURCE_PAYLOADS_VERIFIED', 'source_commit': source,
            'verified_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(), 'remote': REMOTE,
            'checks': {'exact_clean_head': True, 'no_object_alternates': True, 'tracked_files': len(names),
                       'lfs_files': len(rows), 'lfs_bytes': sum(r['bytes'] for r in rows),
                       'all_lfs_hashes_and_sizes_match': True, 'candidate07_payloads': len(payloads)},
            'source_asset_manifest': {'path': manifest_name, 'bytes': manifest_path.stat().st_size, 'sha256': digest(manifest_path)},
            'lfs_files': rows, 'engine_automation_tests_expected': sorted(registrations),
            'build_verified': False, 'runtime_verified': False, 'release_verified': False,
            'limits': ['No engine tests were executed; names are current simple source registrations.',
                       'Remote URL and checkout identity do not prove that an external public download occurred.',
                       'Metadata privacy, import correspondence, source licensing and outgoing history require their separate reviews.']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--source', required=True)
    parser.add_argument('--manifest', default='Evidence/Candidate07/SourceAssets.json')
    parser.add_argument('--output', type=Path, required=True, help='New private JSON under a Saved directory; never overwritten')
    args = parser.parse_args()
    require(re.fullmatch(r'[0-9a-f]{40}', args.source), 'Expected a full source commit')
    output = args.output.resolve()
    require('saved' in {p.casefold() for p in output.parts} and output.suffix == '.json', 'Use a private Saved JSON report')
    require(not output.exists(), 'Preserve the existing verification report')
    report = verify(args.root.resolve(), args.source, args.manifest)
    output.parent.mkdir(parents=True, exist_ok=True)
    with output.open('x', encoding='utf-8') as stream:
        json.dump(report, stream, indent=2)
        stream.write('\n')
    print(json.dumps({'status': report['status'], 'source_commit': args.source, **report['checks'], 'release_verified': False}))


if __name__ == '__main__':
    main()
