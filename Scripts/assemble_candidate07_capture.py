"""Candidate07 wrapper for genuine timestamped engine frames and master audio.

Choose --public (passing exact-source packaged run required) or --review-only
(private diagnostic footage, including failed gameplay checks). Both require
finalized valid capture transport: incomplete WAVs/frames/tails are rejected.
--validate-only writes nothing and never invokes FFmpeg. Actual encoding must
run separately from profiles. No output overwrite, retiming, replacement audio,
synthetic silence, continuous-playback or perceptual approval claim.
"""
import argparse
import csv
import json
import os
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import uuid
import wave

import assemble_candidate06_capture as capture_tools
from create_release_archive import REQUIRED

ROOT = Path(__file__).resolve().parents[1]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def identity(path):
    return {'file': path.name, 'bytes': path.stat().st_size, 'sha256': capture_tools.digest(path)}


def portable(value):
    if isinstance(value, dict):
        for item in value.values():
            portable(item)
    elif isinstance(value, list):
        for item in value:
            portable(item)
    elif isinstance(value, str):
        require(not re.search(r'[A-Za-z]:[\\/]|\\\\|/(?:Users|home)/', value), 'Portable sidecar contains a host path')


def output_paths(args):
    folder = args.input.resolve(strict=True)
    require(folder.is_dir(), 'Input must be a finalized engine capture directory')
    output = args.output.resolve()
    sidecar = output.with_suffix('.assembly.json')
    parts = {p.casefold() for p in output.parts}
    require(output.suffix.casefold() == '.mp4' and 'candidate07' in parts, 'Output must be an MP4 inside Candidate07')
    require(not any(re.search(r'candidate0[1-6](?!\d)', p.casefold()) for p in output.parts), 'Earlier candidate outputs are protected')
    require(not args.review_only or 'saved' in parts, 'Review-only footage must remain private under Saved/Candidate07')
    require(not output.is_relative_to(folder), 'Output must remain outside original capture inputs')
    for path in (output, sidecar):
        require(not path.exists() and not path.is_symlink(), 'Existing movie/sidecar must be preserved')
    require(args.ffmpeg.is_absolute(), '--ffmpeg must name an explicit absolute executable')
    ffmpeg = args.ffmpeg.resolve(strict=True)
    require(ffmpeg.is_file(), '--ffmpeg must exist')
    return folder, output, sidecar, ffmpeg


def actor_checks(folder):
    path = folder.parent / 'checks.txt'
    if not path.is_file():
        return {'status': 'NOT_PROVIDED', 'scope': 'No completed Candidate07 actor check file accompanies this capture'}, None
    text = capture_tools.read_record(path)
    matches = re.findall(r'^Checks=(\d+) Failures=(\d+) HealthRestores=(\d+) InputEdges=(\d+) Encounter=([01])\s*$', text, re.M)
    require(len(matches) == 1, 'Actor check report must contain exactly one completed numeric summary')
    checks, failures, restores, edges, encounter = map(int, matches[0])
    outcomes = re.findall(r'^(PASS|FAIL) \| ', text, re.M)
    require(checks > 0 and len(outcomes) == checks and outcomes.count('FAIL') == failures, 'Actor summary disagrees with actual check rows')
    return {'status': 'PASS' if failures == 0 else 'FAILED', 'checks': checks, 'failures': failures,
            'health_restores': restores, 'input_edges': edges, 'encounter': encounter,
            'report': identity(path), 'scope': 'Actual controlled physicality probes' if not encounter else 'Actual scripted ordinary encounter'}, path


def public_binding(args, folder, actor):
    require(args.run_result is not None and args.build_root is not None, 'Public mode requires --run-result and --build-root')
    require(args.capture_kind == 'packaged' and args.source_state == 'exact-commit' and args.source_revision,
            'Public mode requires exact committed packaged source')
    require(actor['status'] == 'PASS', 'Public mode requires completed passing gameplay checks')
    root = args.build_root.resolve(strict=True)
    report = capture_tools.strict_json(args.run_result)
    require(report.get('state') == 'PASS' and report.get('source_commit') == args.source_revision and report.get('exit_code') == 0,
            'Public run report is incomplete, failed or identifies another source')
    for key in ('capture_requested', 'prelaunch_source_runtime_verified', 'postrun_source_runtime_verified'):
        require(report.get(key) is True, 'Run report lacks ' + key)
    fixture = report.get('fixture_folder')
    require(isinstance(fixture, str) and not Path(fixture).is_absolute() and '..' not in Path(fixture).parts,
            'Run fixture path must be portable')
    fixture_path = (root / fixture).resolve()
    require(fixture_path.is_relative_to(root) and folder == fixture_path / 'Media', 'Input differs from the actual run fixture')
    completion = report.get('completion', {})
    require(all(completion.get(k) == actor[k] for k in ('checks', 'failures', 'encounter')), 'Run completion differs from actual actor report')
    for key, filename in (('metadata', 'capture.json'), ('frames_csv', 'frames.csv'), ('audio', 'gameplay_master.wav')):
        path = folder / filename
        row = report.get('capture', {}).get(key, {})
        expected = {'path': path.relative_to(root).as_posix(), 'bytes': path.stat().st_size, 'sha256': capture_tools.digest(path)}
        require(row == expected, 'Actual capture input differs from the passing run: ' + filename)
    runtime = report.get('runtime_files', [])
    require(len(runtime) == len(REQUIRED) and {r['path'] for r in runtime} == set(REQUIRED), 'Run must bind exactly six required runtime identities')
    package = root / 'Packaged/Candidate07/Windows'
    for row in runtime:
        path = (package / row['path']).resolve(strict=True)
        require(path.is_relative_to(package.resolve()) and path.is_file(), 'External or missing runtime')
        require(path.stat().st_size == row['bytes'] and capture_tools.digest(path) == row['sha256'], 'Runtime bytes differ from passing run: ' + row['path'])
    return {'status': 'PASSING_RUN_AND_RUNTIME_BOUND', 'source_commit': args.source_revision,
            'report': identity(args.run_result), 'runtime_files': runtime,
            'limit': 'Passing runner report supplies prior build/source verification; current runtime bytes are independently rehashed here. Encoding is not fresh-build or public-download verification.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    policy = parser.add_mutually_exclusive_group(required=True)
    policy.add_argument('--public', action='store_true')
    policy.add_argument('--review-only', action='store_true')
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--ffmpeg', type=Path, required=True)
    parser.add_argument('--capture-kind', choices=('packaged', 'editor-game'), required=True)
    parser.add_argument('--render-mode', choices=('offscreen', 'windowed', 'unknown'), default='unknown')
    parser.add_argument('--source-revision')
    parser.add_argument('--source-state', choices=('exact-commit', 'working-tree', 'unknown'), default='unknown')
    parser.add_argument('--run-result', type=Path)
    parser.add_argument('--build-root', type=Path)
    parser.add_argument('--chapters', action='store_true')
    parser.add_argument('--validate-only', action='store_true')
    args = parser.parse_args()
    try:
        require(not args.source_revision or re.fullmatch(r'[0-9a-f]{40}', args.source_revision), 'Source revision must be a full lowercase Git commit')
        require(args.source_state != 'exact-commit' or args.source_revision, 'Exact-commit source state requires its actual commit')
        folder, output, sidecar, ffmpeg = output_paths(args)
        frames, intervals, audio, summary, ledger = capture_tools.prepare(folder, args)
        actor, actor_path = actor_checks(folder)
        binding = {'status': 'UNVERIFIED_DIAGNOSTIC_SOURCE', 'source_state': args.source_state,
                   'source_commit': args.source_revision if args.source_state == 'exact-commit' else None,
                   'working_tree_base_commit': args.source_revision if args.source_state == 'working-tree' else None,
                   'limit': 'Operator-supplied source/base label; no runtime or release verification claimed'}
        if args.public:
            binding = public_binding(args, folder, actor)
        elif args.run_result:
            run = capture_tools.strict_json(args.run_result)
            # Preserve literal status/counts without copying private error paths.
            binding['diagnostic_run'] = {'report': identity(args.run_result), 'state': run.get('state'),
                                         'source_commit': run.get('source_commit'), 'completion': run.get('completion'), 'exit_code': run.get('exit_code')}
            if run.get('dll_sha256'):
                require(re.fullmatch(r'[0-9a-f]{64}', run['dll_sha256']), 'Invalid diagnostic DLL hash')
                binding['diagnostic_run']['recorded_dll_sha256'] = run['dll_sha256']
        summary.update(schema='one07.capture_assembly.v1', candidate='Candidate07',
                       status='VALIDATED_PENDING_ENCODE' if args.public else 'REVIEW_ONLY',
                       intended_public_evidence=bool(args.public), public_input_requirements_met=bool(args.public),
                       gameplay_checks=actor, source_runtime_binding=binding, release_verified=False,
                       tools=[identity(Path(__file__)), identity(Path(capture_tools.__file__)), identity(Path(capture_tools.__file__).with_name('assemble_candidate05_capture.py'))])
        if args.review_only:
            summary['diagnostic_limit'] = 'Private WIP or failed-gameplay review footage. Valid capture bytes do not make failed/missing gameplay or unverified source pass. Failed or incomplete capture transport is rejected rather than repaired.'
        watched = [(actor_path, identity(actor_path))] if actor_path else []
        if args.run_result:
            watched.append((args.run_result, identity(args.run_result)))
        def unchanged():
            capture_tools.verify_inputs(folder, summary['input_files'] + ledger)
            for path, expected in watched:
                require(identity(path) == expected, 'Actor/run evidence changed during assembly')
        if args.validate_only:
            unchanged()
            summary.update(validation_only=True, files_written=0)
            portable(summary)
            print(json.dumps(summary, indent=2, allow_nan=False))
            return
        scratch_root = ROOT / 'Saved/Candidate07/Assembly'
        scratch_root.mkdir(parents=True, exist_ok=True)
        scratch = Path(tempfile.mkdtemp(prefix='run_', dir=scratch_root))
        concat = scratch / 'frames.ffconcat'
        concat.write_text(capture_tools.concat_text(frames, intervals), encoding='utf-8')
        (scratch / 'source_frame_inventory.json').write_text(json.dumps(ledger, indent=2) + '\n', encoding='utf-8')
        command = [str(ffmpeg), '-nostdin', '-n', '-hide_banner', '-loglevel', 'warning', '-xerror',
                   '-f', 'concat', '-safe', '0', '-i', str(concat), '-i', str(audio)]
        if summary['chapters']:
            metadata = scratch / 'chapters.ffmetadata'
            metadata.write_text(capture_tools.chapter_text(summary['chapters']), encoding='utf-8')
            command += ['-f', 'ffmetadata', '-i', str(metadata)]
        command += ['-map', '0:v:0', '-map', '1:a:0', '-map_metadata', '-1', '-map_metadata:s:v', '-1', '-map_metadata:s:a', '-1',
                    '-map_chapters', '2' if summary['chapters'] else '-1', '-c:v', 'libx264', '-crf', '20', '-preset', 'medium', '-pix_fmt', 'yuv420p',
                    '-vf', 'tpad=stop_mode=clone:stop_duration=1,fps=30', '-r', '30', '-fps_mode', 'cfr',
                    '-c:a', 'aac', '-b:a', '192k', '-t', f"{summary['audio_duration_seconds']:.9f}", '-movflags', '+faststart']
        output.parent.mkdir(parents=True, exist_ok=True)
        pending = output.parent / (output.stem + '.' + uuid.uuid4().hex + '.partial.mp4')
        (scratch / 'encode_command.private.json').write_text(json.dumps({'argv': command + [str(pending)]}, indent=2) + '\n', encoding='utf-8')
        # Preserve failed encoder output/logs. A zero exit is only an encode result;
        # complete independent probing/decoding and review still follow.
        with (scratch / 'encode.private.log').open('xb') as log:
            code = subprocess.run(command + [str(pending)], stdin=subprocess.DEVNULL, stdout=log, stderr=subprocess.STDOUT, check=False).returncode
        (scratch / 'encode_exit.json').write_text(json.dumps({'exit_code': code}) + '\n', encoding='utf-8')
        require(code == 0 and pending.is_file() and pending.stat().st_size > 0, 'Encoding failed or produced no nonempty output; private diagnostics preserved')
        unchanged()
        summary.update(encoded=True, encoder_exit_code=code, encoder_binary_sha256=capture_tools.digest(ffmpeg),
                       output_file=output.name, output_bytes=pending.stat().st_size, output_sha256=capture_tools.digest(pending),
                       status='ENCODED_PENDING_INSPECTION' if args.public else 'REVIEW_ONLY')
        portable(summary)
        os.link(pending, output)  # Atomic no-overwrite publication to the chosen local filename.
        with sidecar.open('x', encoding='utf-8') as stream:
            json.dump(summary, stream, indent=2, allow_nan=False)
            stream.write('\n')
        pending.unlink()
        print(json.dumps(summary, indent=2, allow_nan=False))
    except (OSError, ValueError, wave.Error, csv.Error, struct.error) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
