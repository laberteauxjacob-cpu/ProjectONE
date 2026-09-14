"""Candidate07 wrapper for genuine timestamped engine frames and master audio.

Choose --public (passing exact-source packaged run required) or --review-only
(private diagnostic footage, including failed gameplay checks). Both require
finalized valid capture transport: incomplete WAVs/frames/tails are rejected.
--validate-only writes nothing and never invokes FFmpeg. Actual encoding must
run separately from profiles. No output overwrite, retiming, replacement audio,
synthetic silence, continuous-playback or perceptual approval claim.
Runner binding accepts run_candidate07_checks.py's one07.private_runtime.v1;
editor WIP keeps a null source_commit. Public mode rechecks the clean source,
six runtime files and original one07.runtime_build.v1 proof/build logs.
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
import candidate07_legacy_motion as legacy_motion
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
    if re.search(r'^ONE06_COMBAT_COMPLETE\b',text,re.M):
        import run_candidate07_checks as runner
        done=runner.completion(text,'ONE06_COMBAT_COMPLETE',allow_failures=True)
        rows=re.findall(r'^ONE06_COMBAT\s+(PASS|FAIL)\s*\|\s*([^\r\n]*)',text,re.M)
        require(len(rows)==done['checks'] and sum(state=='FAIL' for state,_ in rows)==done['failures'],
                'Native combat text assertion stream disagrees with completion')
        require(set(done)=={'checks','failures'},'Unexpected native combat completion schema')
        return {'kind':'combat','status':'PASS' if done['failures']==0 else 'FAILED',**done,'report':identity(path),
                'scope':'Native ONE06CombatCheck across all six weapon variants; capture counts remain in common recorder metadata'},path
    json_path = folder.parent / 'checks.json'
    if json_path.is_file():
        data = capture_tools.strict_json(json_path)
        require(data.get('schema') == 'one07.portability.v1', 'Unknown adjacent actor JSON schema')
        outcomes = re.findall(r'^(PASS|FAIL) \| ([^\r\n]+)\r?$', text, re.M)
        rows = data.get('assertions', [])
        require(data.get('checks', 0) > 0 and len(rows) == len(outcomes) == data['checks'] and
                sum(outcome == 'FAIL' for outcome, _ in outcomes) == data.get('failures') and
                [('PASS' if row.get('pass') is True else 'FAIL', row.get('label')) for row in rows] == outcomes,
                'Portability text and JSON assertion streams disagree')
        return {'kind':'portability', 'status':'PASS' if data.get('complete') is True and data['failures'] == 0 and data.get('variants_completed') == 3 else 'FAILED',
                'checks':data['checks'], 'failures':data['failures'], 'variants':data.get('variants_completed'),
                'report':identity(path), 'json_report':identity(json_path), 'scope':'Actual three-appearance second-map probes'}, path
    matches = re.findall(r'^Checks=(\d+) Failures=(\d+) HealthRestores=(\d+) InputEdges=(\d+) Encounter=([01])\s*$', text, re.M)
    require(len(matches) == 1, 'Actor check report must contain exactly one completed numeric summary')
    checks, failures, restores, edges, encounter = map(int, matches[0])
    outcomes = re.findall(r'^(PASS|FAIL) \| ', text, re.M)
    require(checks > 0 and len(outcomes) == checks and outcomes.count('FAIL') == failures, 'Actor summary disagrees with actual check rows')
    return {'kind':'encounter' if encounter else 'physicality', 'status': 'PASS' if failures == 0 else 'FAILED', 'checks': checks, 'failures': failures,
            'health_restores': restores, 'input_edges': edges, 'encounter': encounter,
            'report': identity(path), 'scope': 'Actual controlled physicality probes' if not encounter else 'Actual scripted ordinary encounter'}, path


def prepare_input(folder, args):
    """Keep native legacy evidence distinct from the common recorder schema."""
    if args.input_format == 'legacy-motion':
        require(args.review_only, 'Legacy motion is private review only; it cannot satisfy public capture requirements')
        frames, intervals, audio, summary, ledger = legacy_motion.prepare(folder, args)
        actor, actor_path = summary['gameplay_checks'], folder/'checks.txt'
    else:
        frames, intervals, audio, summary, ledger = capture_tools.prepare(folder, args)
        actor, actor_path = actor_checks(folder)
        summary['input_format'] = 'common'
    return frames, intervals, audio, summary, ledger, actor, actor_path


def run_binding(args, folder, actor, summary, ledger, public):
    """Bind the actual runner schema. WIP may be reviewed after later builds."""
    import run_candidate07_checks as runner
    require(args.run_result is not None, 'Run binding requires --run-result')
    report = capture_tools.strict_json(args.run_result)
    require(report.get('schema') == 'one07.private_runtime.v1' and report.get('candidate') == '07' and
            report.get('mode') in ('editor','packaged') and report.get('check') in ('physicality','encounter','portability','combat'),
            'Expected actual C07 capture-capable runner schema')
    require(report.get('recording') is True and report.get('capture_requested') is True, 'Run did not request capture')
    require(report.get('check') == actor.get('kind'), 'Actor report differs from requested check')
    require(args.capture_kind == ('editor-game' if report['mode'] == 'editor' else 'packaged'), 'Operator capture kind differs from runner')
    if report['mode'] == 'editor':
        require(not public and report.get('source_commit') is None and args.source_state != 'exact-commit',
                'Editor WIP remains review-only with null source_commit')
        require(not args.source_revision or args.source_revision == report.get('observed_head'), 'Working-tree base label differs from observed HEAD')
        runtime = report.get('runtime_files',[])
        require(len(runtime) == 2 and {row['path'] for row in runtime} ==
                {Path(report['executable']).name,'Binaries/Win64/UnrealEditor-ProjectONE.dll'}, 'WIP run must record its actual editor and project DLL identities')
    else:
        require(isinstance(report.get('source_commit'),str) and re.fullmatch(r'[0-9a-f]{40}',report['source_commit']), 'Invalid packaged source commit')
        require(not args.source_revision or args.source_revision == report['source_commit'], 'Supplied source differs from packaged runner')
    saved = Path(report['saved_root']).resolve(strict=True)
    fixture = report.get('fixture_folder')
    require(isinstance(fixture,str) and fixture and not Path(fixture).is_absolute() and '..' not in Path(fixture).parts,
            'Run fixture folder must be relative to its recorded Saved root')
    fixture_path = (saved/fixture).resolve(strict=True)
    require(fixture_path.is_relative_to(saved) and folder == fixture_path/'Media', 'Media differs from the actual run fixture')
    artifacts = report.get('artifacts',{}).get('driver',[])
    require(len(artifacts) == 1 and Path(artifacts[0]['folder']).resolve() == fixture_path, 'Ambiguous or mismatched actual actor artifacts')
    cached = {'Media/'+row['file']:{'path':'Media/'+row['file'],'bytes':row['bytes'],'sha256':row['sha256']}
              for row in summary['input_files'] + ledger}
    actual = []
    for path in sorted(fixture_path.rglob('*')):
        if path.is_file():
            require(path.resolve().is_relative_to(fixture_path), 'Actor artifact escapes its fixture')
            relative = path.relative_to(fixture_path).as_posix()
            actual.append(cached.get(relative) or runner.record(path,fixture_path))
    require(actual == artifacts[0]['files'], 'Raw actor/capture file inventory or hashes differ from the runner')
    done = report.get('completion',{})
    require(all(done.get(key) == actor[key] for key in ('checks','failures')) and
            ('frames' not in done if actor['kind']=='combat' else done.get('frames') == summary['frames']),
            'Runner completion disagrees with actual actor/capture counts')
    if actor['kind'] == 'portability': require(done.get('variants') == actor['variants'], 'Portability variant count differs')
    elif actor['kind']!='combat': require(done.get('encounter') == actor['encounter'], 'Encounter discriminator differs')
    log_path = args.run_result.resolve().parent/'engine.log'
    require(runner.record(log_path) == report.get('engine_log'), 'Original runner engine log changed')
    log = runner.readtext(log_path)
    _, _, marker, _ = runner.check_layout(report['check'],'07')
    require(runner.completion(log,marker,allow_failures=True) == done, 'Actual engine completion differs from runner')
    prefix = 'ONE07_PORTABILITY' if actor['kind'] == 'portability' else 'ONE06_COMBAT' if actor['kind']=='combat' else 'ONE07_PHYSICALITY'
    actual_checks = re.findall(r'^'+('ONE06_COMBAT ' if actor['kind']=='combat' else '')+r'(PASS|FAIL) \| ([^\r\n]+)\r?$',runner.readtext(fixture_path/'checks.txt'),re.M)
    log_checks = re.findall(r'\b'+prefix+r'\s+(PASS|FAIL)\s*\|\s*([^\r\n]*)',log)
    require(log_checks == actual_checks, 'Actual actor assertion stream differs from original engine log')
    if actor['kind']=='combat':
        require(re.findall(r'ONE06_CAPTURE_COMPLETE frames=(\d+)',log)==[str(summary['frames'])],
                'Native combat common recorder completion differs from actual frame count')
    captured = report.get('capture',{})
    require(captured.get('schema') == 'one07.capture_transport.v1' and captured.get('status') == 'PASS' and captured.get('path_base') == 'fixture_folder',
            'Runner did not validate complete capture transport')
    for key in ('frames','dimensions','ordered_frame_identity_sha256','audio_duration_seconds','audio_sample_rate','audio_channels','audio_sample_frames',
                'source_wav_layout','actual_real_stop_tail_seconds','recorded_audio_after_last_callback_seconds','first_frame_audio_seconds','last_frame_audio_seconds'):
        require(captured.get(key) == summary[key], 'Actual transport differs from runner: '+key)
    for key,filename in (('metadata','capture.json'),('frames_csv','frames.csv'),('audio','gameplay_master.wav')):
        require(captured.get(key) == cached['Media/'+filename], 'Capture hash differs: '+filename)
    before, after = report.get('source_before',{}), report.get('source_after',{})
    require(before.get('files') and before.get('observed_head') == report.get('observed_head') and
            re.fullmatch(r'[0-9a-f]{40}',str(report.get('observed_head'))), 'Missing observed source/HEAD identity')
    if report.get('inputs_unchanged') is True:
        require(runner.same_runtime_inputs(before,after,report['mode']), 'Run incorrectly reports unchanged source inputs')
    binding = {'status':'DIAGNOSTIC_RUN_ARTIFACTS_BOUND','source_commit':report.get('source_commit'),
               'observed_head':report.get('observed_head'),'report':identity(args.run_result),'state':report.get('state'),'exit_code':report.get('exit_code'),
               'completion':done,'source_snapshot_before_sha256':before.get('snapshot_sha256'),
               'source_snapshot_after_sha256':after.get('snapshot_sha256'),'inputs_unchanged':report.get('inputs_unchanged'),
               'runtime_files':report.get('runtime_files',[]),
               'validation_tool':identity(Path(runner.__file__)),
               'limit':'Actual runner record and original capture/actor hashes are rechecked. WIP runtime/source identities are recorded historical observations, not a compiled-commit or current-binary verification.'}
    if public:
        require(args.build_root is not None and args.capture_kind == 'packaged' and args.source_state == 'exact-commit' and args.source_revision,
                'Public mode requires exact committed packaged source and --build-root')
        require(actor['status'] == 'PASS' and report.get('state') == 'PASS' and report.get('exit_code') == 0 and report['source_commit'] == args.source_revision,
                'Public mode requires a passing actual packaged run at this source')
        for key in ('prelaunch_source_runtime_verified','postrun_source_runtime_verified','inputs_unchanged'):
            require(report.get(key) is True,'Public run lacks '+key)
        root = args.build_root.resolve(strict=True)
        require(root == Path(report['root']).resolve() and before == after and before.get('observed_head') == args.source_revision and
                before.get('git_status') == '' and before.get('files'), 'Public source snapshot is not stable, clean and exact')
        require(runner.source_snapshot(root) == before, 'Current source checkout differs from the verified run')
        runtime = report.get('runtime_files',[])
        require(len(runtime) == len(REQUIRED) and {row['path'] for row in runtime} == set(REQUIRED), 'Public run must bind exactly six runtime files')
        package = Path(report['package_root']).resolve(strict=True)
        require(saved == package/'ProjectONE/Saved' and Path(report['executable']).resolve() == package/'ProjectONE/Binaries/Win64/ProjectONE.exe',
                'Saved root or launched game differs from the verified runtime package')
        require([runner.record(package/path,package) for path in runner.RUNTIME] == runtime, 'Current runtime bytes differ from passing run')
        proof_path = Path(report['build_proof_path']).resolve(strict=True)
        require(runner.record(proof_path) == report.get('build_proof'), 'Fresh-build proof changed after run')
        proof = capture_tools.strict_json(proof_path)
        require(proof.get('schema') == 'one07.runtime_build.v1' and proof.get('status') == 'PASS' and proof.get('candidate') == '07' and
                proof.get('source_commit') == args.source_revision and sorted(proof.get('runtime_files',[]),key=lambda row:row['path']) == sorted(runtime,key=lambda row:row['path']),
                'Public fresh-build proof differs from source/runtime identities')
        require(proof.get('build_logs'), 'Fresh-build proof lacks original build logs')
        for row in proof['build_logs']:
            path=(root/row['path']).resolve(strict=True)
            require(path.is_relative_to(root) and runner.record(path,root) == row, 'Fresh-build log binding differs')
        if actor['kind']=='combat':
            require(report.get('requested')=={'weapon_variants':6,'cases':13},'Public combat did not request all six weapons and thirteen scenes')
            runner.regression.validate('combat',fixture_path,done,log,120,'1600x900')
        else:runner.validate_fixture_completion(done,report['check'],capture=True)
        require(not re.search(r'\bFAIL\s*\||Fatal error:|Assertion failed:|Ensure condition failed:|Result=\{Fail\}',log),
                'Original public run log records an engine/check failure')
        if actor['kind'] == 'portability':
            runner.validate_portability(fixture_path,done,log,[label for _,label in actual_checks],capture=True)
        binding.update(status='PASSING_RUN_SOURCE_RUNTIME_AND_BUILD_BOUND',build_proof=identity(proof_path),
                       limit='Actual captured run, clean exact-source checkout, all six runtime files and fresh-build proof/logs rechecked. No new build, public-download verification, playback or audition is claimed.')
    portable(binding)
    return binding


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    policy = parser.add_mutually_exclusive_group(required=True)
    policy.add_argument('--public', action='store_true')
    policy.add_argument('--review-only', action='store_true')
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--input-format', choices=('common','legacy-motion'), default='common',
                        help='Original ONE05MotionCapture format is explicitly review-only, with all gameplay failures retained')
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
        require(args.input_format != 'legacy-motion' or args.review_only, 'Legacy motion cannot be public capture evidence')
        require(not args.source_revision or re.fullmatch(r'[0-9a-f]{40}', args.source_revision), 'Source revision must be a full lowercase Git commit')
        require(args.source_state != 'exact-commit' or args.source_revision, 'Exact-commit source state requires its actual commit')
        require(args.capture_kind != 'editor-game' or (args.review_only and args.source_state != 'exact-commit'),
                'Editor captures remain review-only working-tree/unknown source, never an exact built commit')
        folder, output, sidecar, ffmpeg = output_paths(args)
        frames, intervals, audio, summary, ledger, actor, actor_path = prepare_input(folder, args)
        binding = {'status': 'UNVERIFIED_DIAGNOSTIC_SOURCE', 'source_state': args.source_state,
                   'source_commit': args.source_revision if args.source_state == 'exact-commit' else None,
                   'working_tree_base_commit': args.source_revision if args.source_state == 'working-tree' else None,
                   'limit': 'Operator-supplied source/base label; no runtime or release verification claimed'}
        if args.public:
            binding = run_binding(args, folder, actor, summary, ledger, True)
        elif args.run_result:
            run = capture_tools.strict_json(args.run_result)
            if args.input_format == 'common' and run.get('schema') == 'one07.private_runtime.v1':
                binding = run_binding(args, folder, actor, summary, ledger, False)
            # Preserve literal status/counts without copying private error paths.
            else:
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
        if args.input_format == 'legacy-motion':
            summary['tools'].append(identity(Path(legacy_motion.__file__)))
        if args.review_only:
            summary['diagnostic_limit'] = 'Private WIP or failed-gameplay review footage. Valid capture bytes do not make failed/missing gameplay or unverified source pass. Failed or incomplete capture transport is rejected rather than repaired.'
        watched = [(actor_path, identity(actor_path))] if actor_path else []
        actor_json = folder.parent/'checks.json'
        if args.input_format == 'common' and actor_json.is_file(): watched.append((actor_json,identity(actor_json)))
        if args.run_result:
            watched.append((args.run_result, identity(args.run_result)))
            original_log=args.run_result.resolve().parent/'engine.log'
            if original_log.is_file(): watched.append((original_log,identity(original_log)))
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
