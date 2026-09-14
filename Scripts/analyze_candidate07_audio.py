"""Measure finalized engine master WAVs and their actual captured phase timeline.

No engine, encoder, playback or audition is invoked. Reuses the accepted capture
input/WAV guards and all-sample PCM auditor. Output is a new private directory.
Optional reference-then-current comparison joins entire PCM recordings without
gain, trimming, silence, resampling, time changes or crossfades. It is an audio
comparison artifact, not a continuous session or synchronized gameplay movie.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
from types import SimpleNamespace
import wave

import assemble_candidate06_capture as capture_tools
import audit_gameplay_audio as pcm_tools


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
        require(not re.search(r'[A-Za-z]:[\\/]|\\\\|/(?:Users|home)/', value), 'Portable report contains a host path')


def write_json(path, report):
    portable(report)
    with path.open('x', encoding='utf-8') as stream:
        json.dump(report, stream, indent=2, allow_nan=False)
        stream.write('\n')


def peak_windows(envelope, threshold):
    """Merge adjacent actual windows, retaining even a single endpoint sample."""
    result = []
    for start, end, rms, peak in envelope['windows']:
        if peak is None or peak < threshold:
            continue
        if not result or abs(start - result[-1]['end_seconds']) > 1e-7:
            result.append({'start_seconds': start, 'end_seconds': end, 'maximum_sample_peak_dbfs': peak, 'window_count': 1})
        else:
            result[-1]['end_seconds'] = end
            result[-1]['maximum_sample_peak_dbfs'] = max(result[-1]['maximum_sample_peak_dbfs'], peak)
            result[-1]['window_count'] += 1
    return result


def analyze(folder, source, run_path=None, build_root=None, review_only=False, source_state='exact-commit', capture_kind='packaged'):
    require(not source or re.fullmatch(r'[0-9a-f]{40}', source), 'Expected a full source/base commit when supplied')
    require(source_state != 'exact-commit' or source, 'Exact-commit source state requires its actual source commit')
    require(review_only or (source and source_state == 'exact-commit' and capture_kind == 'packaged' and run_path and build_root),
            'Public analysis requires exact committed packaged source and a passing run binding')
    folder = folder.resolve(strict=True)
    # prepare() never invokes FFmpeg or writes files. Its original capture
    # schema is shared by C06 and C07; no historical result is relabeled here.
    args = SimpleNamespace(chapters=True, capture_kind=capture_kind, render_mode='unknown',
                           source_revision=source, source_state=source_state)
    _, _, wav, inputs, ledger = capture_tools.prepare(folder, args)
    binding = {'status': 'UNVERIFIED_DIAGNOSTIC_SOURCE', 'source_state': source_state,
               'source_commit': source if source_state == 'exact-commit' else None,
               'working_tree_base_commit': source if source_state == 'working-tree' else None,
               'limit': 'The source label alone does not establish runtime identity; use the separate package report.'}
    if run_path and review_only:
        run = capture_tools.strict_json(run_path)
        diagnostic = {'report': identity(run_path), 'state': run.get('state'), 'exit_code': run.get('exit_code')}
        dll_hash = run.get('dll_sha256')
        if dll_hash:
            require(re.fullmatch(r'[0-9a-f]{64}', dll_hash), 'Invalid diagnostic DLL hash')
            diagnostic['recorded_dll_sha256'] = dll_hash
        diagnostic['limit'] = 'Hash of the supplied diagnostic run record and its recorded DLL identity; no independent binary recheck or exact committed-source claim. Private command paths are omitted.'
        binding['diagnostic_run'] = diagnostic
    elif run_path:
        require(build_root is not None, 'A run result requires its actual build root')
        run = capture_tools.strict_json(run_path)
        require(run.get('state') == 'PASS' and run.get('source_commit') == source and run.get('capture_requested') is True,
                'Run result does not identify a passing capture at this source')
        require(run.get('prelaunch_source_runtime_verified') is True and run.get('postrun_source_runtime_verified') is True,
                'Run result lacks both runtime/source verification gates')
        fixture = run.get('fixture_folder')
        require(isinstance(fixture, str) and fixture and not Path(fixture).is_absolute() and '..' not in Path(fixture).parts,
                'Run result fixture folder is not portable')
        fixture_path = (build_root.resolve() / fixture).resolve()
        require(fixture_path.is_relative_to(build_root.resolve()) and folder == fixture_path / 'Media',
                'Supplied media is not the passing run fixture Media folder')
        binding = {'status': 'PASSING_RUN_REPORT_BOUND', 'source_commit': source, 'report': identity(run_path),
                   'mode': run.get('mode'), 'limit': 'Binds the actual passing runner report; this analyzer does not execute the game or rehash packaged binaries.'}
    phases = [{'name': str(p['phase']) + ': ' + p['title'], 'start_seconds': p['start_seconds'], 'end_seconds': p['end_seconds']}
              for p in inputs['chapters']]
    measured = pcm_tools.audit(wav, phases, 100, -60, -38, 50)
    measured['measurement_limits'][0] = 'This analyzer performs no perceptual audition or playback.'
    require(measured['source_sha256'] == next(x['sha256'] for x in inputs['input_files'] if x['file'] == wav.name),
            'Measured WAV differs from validated capture input')
    require(all(p['coverage'] == 'complete' for p in measured['phases']), 'A phase extends past the actual WAV')
    measured.update(silence_threshold_dbfs=-60, near_clip_threshold_dbfs=-.1,
                    near_clip_100ms_windows=peak_windows(measured['envelope'], -.1))
    measured['near_clip_window_limit'] = 'Each listed 100 ms window contains at least one sample at or above -0.1 dBFS; boundaries are window quantization, not exact clip duration. All signed PCM endpoints are counted separately.'
    measured['phase_energy_policy'] = 'Zero-energy phases are measured, never automatically failed; pause/death and intentionally quiet intervals require context. Threshold bursts are not inferred shots, contacts, voices or exact animation markers.'
    report = {'schema': 'one07.master_audio_analysis.v1', 'status': 'REVIEW_ONLY' if review_only else 'MEASURED',
              'numerical_status': 'MEASURED', 'input_validation': 'PASS', 'capture_kind': capture_kind,
              'source_binding': binding, 'capture_inputs': inputs['input_files'],
              'capture_frame_count': inputs['frames'], 'ordered_frame_identity_sha256': inputs['ordered_frame_identity_sha256'],
              'capture_timebase': inputs['timebase_limit'], 'phase_timebase_limit': inputs['chapter_timing_limit'],
              'recorded_audio_after_last_callback_seconds': inputs['recorded_audio_after_last_callback_seconds'],
              'actual_real_stop_tail_seconds': inputs['actual_real_stop_tail_seconds'],
              'source_wav_layout': inputs['source_wav_layout'], 'measurements': measured,
              'perceptual_audio_review': False, 'release_verified': False,
              'limits': ['Every original integer PCM sample contributes to peak/RMS/full-scale counts; no warmup, quiet phase or loud spike is excluded.',
                         'Sample peaks are not true peak or LUFS. Numerical energy does not establish timbre, spatial localization, realism, event synchronization or user approval.',
                         'Phase boundaries use first observed frame timestamps. CPU/audio-render clock offset is not measured or corrected.']}
    capture_tools.verify_inputs(folder, inputs['input_files'] + ledger)
    portable(report)
    return report, wav


def concatenate(reference, current, output, reports):
    for path, report in zip((reference, current), reports):
        require(capture_tools.digest(path) == report['measurements']['source_sha256'], 'Comparison source changed after measurement')
    readers = [wave.open(str(path), 'rb') for path in (reference, current)]
    try:
        formats = [(r.getnchannels(), r.getsampwidth(), r.getframerate(), r.getcomptype()) for r in readers]
        require(formats[0] == formats[1] and formats[0][3] == 'NONE', 'Comparison requires identical PCM format; no resampling allowed')
        channels, width, rate, _ = formats[0]
        segments = []
        total = 0
        with output.open('xb') as raw_output, wave.open(raw_output, 'wb') as target:
            target.setparams((channels, width, rate, 0, 'NONE', 'not compressed'))
            for label, reader, report in zip(('reference', 'current'), readers, reports):
                digest = hashlib.sha256()
                frames = 0
                while block := reader.readframes(65536):
                    require(len(block) % (channels * width) == 0, 'Incomplete PCM frame')
                    digest.update(block)
                    frames += len(block) // (channels * width)
                    target.writeframesraw(block)
                require(frames == reader.getnframes(), 'Truncated comparison source')
                segments.append({'label': label, 'source_commit': report['source_binding']['source_commit'],
                                 'source_wav_sha256': report['measurements']['source_sha256'],
                                 'pcm_sha256': digest.hexdigest(), 'sample_frames': frames,
                                 'comparison_start_frame': total, 'comparison_end_frame_exclusive': total + frames,
                                 'comparison_start_seconds': total / rate, 'comparison_end_seconds': (total + frames) / rate,
                                 'source_start_seconds': 0, 'source_end_seconds': frames / rate,
                                 'phases': [{'name': p['name'], 'source_start_seconds': p['start_seconds'], 'source_end_seconds': p['end_seconds'],
                                             'comparison_start_seconds': total / rate + p['start_seconds'],
                                             'comparison_end_seconds': total / rate + p['end_seconds']}
                                            for p in report['measurements']['phases']]})
                total += frames
        with wave.open(str(output), 'rb') as joined:
            require(joined.getnframes() == total, 'Comparison sample-frame count differs')
            for segment in segments:
                digest = hashlib.sha256()
                remaining = segment['sample_frames']
                while remaining:
                    block = joined.readframes(min(65536, remaining))
                    require(block, 'Comparison WAV truncated')
                    digest.update(block)
                    remaining -= len(block) // (channels * width)
                require(digest.hexdigest() == segment['pcm_sha256'], 'Comparison changed a source PCM segment')
        for path, report in zip((reference, current), reports):
            require(capture_tools.digest(path) == report['measurements']['source_sha256'], 'Comparison source changed during copy')
        return {'status': 'PCM_SEGMENTS_EXACT', 'output': identity(output), 'segments': segments,
                'policy': 'Complete reference recording then complete current recording, original sample order. No gain, trim, added silence, crossfade, replacement, resampling or time correction. The direct boundary may be discontinuous. These are separate sessions, not a synchronized continuous scene.',
                'perceptual_audio_review': False}
    finally:
        for reader in readers:
            reader.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    policy = parser.add_mutually_exclusive_group(required=True)
    policy.add_argument('--public', action='store_true', help='Require a passing exact-source packaged run binding')
    policy.add_argument('--review-only', action='store_true', help='Private WIP diagnostic measurement; never a source/runtime PASS')
    parser.add_argument('--media-folder', required=True, type=Path)
    parser.add_argument('--source', help='Exact source commit, or optional base commit for explicitly working-tree diagnostics')
    parser.add_argument('--source-state', choices=('exact-commit', 'working-tree', 'unknown'), default='unknown')
    parser.add_argument('--capture-kind', choices=('packaged', 'editor-game'), default='packaged')
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--run-result', type=Path)
    parser.add_argument('--build-root', type=Path)
    parser.add_argument('--reference-media', type=Path)
    parser.add_argument('--reference-source')
    parser.add_argument('--reference-source-state', choices=('exact-commit', 'working-tree', 'unknown'), default='exact-commit')
    parser.add_argument('--reference-capture-kind', choices=('packaged', 'editor-game'), default='packaged')
    parser.add_argument('--reference-run-result', type=Path)
    parser.add_argument('--reference-build-root', type=Path)
    parser.add_argument('--write-comparison-wav', action='store_true')
    args = parser.parse_args()
    require(args.reference_media is not None or not args.reference_source, 'Reference source requires reference media')
    require(args.review_only or bool(args.run_result) == bool(args.build_root), 'Public run result and build root must be supplied together')
    require(args.review_only or bool(args.reference_run_result) == bool(args.reference_build_root), 'Public reference run result and build root must be supplied together')
    require(args.reference_media is not None or not args.reference_run_result, 'Reference run binding requires reference media')
    require(not args.write_comparison_wav or args.reference_media is not None, 'Comparison output requires reference input')
    output = args.output.resolve()
    require('saved' in {p.casefold() for p in output.parts} and not output.exists(), 'Use a new private Saved directory; existing output is preserved')
    for folder in (args.media_folder, args.reference_media):
        if folder:
            require(not output.is_relative_to(folder.resolve()), 'Output must be outside original capture inputs')
    current, current_wav = analyze(args.media_folder, args.source, args.run_result, args.build_root,
                                   args.review_only, args.source_state, args.capture_kind)
    reference = reference_wav = None
    if args.reference_media:
        reference, reference_wav = analyze(args.reference_media, args.reference_source, args.reference_run_result, args.reference_build_root,
                                           args.review_only, args.reference_source_state, args.reference_capture_kind)
    output.mkdir(parents=True, exist_ok=False)
    write_json(output / 'current_audio.json', current)
    if reference:
        write_json(output / 'reference_audio.json', reference)
    summary = {'schema': 1, 'status': 'REVIEW_ONLY' if args.review_only else 'MEASURED', 'source_commit': current['source_binding']['source_commit'],
               'source_state': args.source_state,
               'generated_utc': datetime.datetime.now(datetime.timezone.utc).isoformat(),
               'tools': [identity(Path(__file__)), identity(Path(capture_tools.__file__)), identity(Path(pcm_tools.__file__))],
               'current_report': identity(output / 'current_audio.json'), 'reference_report': identity(output / 'reference_audio.json') if reference else None,
               'perceptual_audio_review': False, 'encoded_or_decoded_movie': False, 'release_verified': False}
    if args.write_comparison_wav:
        summary['comparison'] = concatenate(reference_wav, current_wav, output / 'reference_then_current.wav', (reference, current))
    write_json(output / 'summary.json', summary)
    print(json.dumps({'status': summary['status'], 'source_commit': summary['source_commit'], 'full_scale_samples': current['measurements']['overall']['full_scale_sample_count'],
                      'near_clip_window_ranges': len(current['measurements']['near_clip_100ms_windows']), 'perceptual_audio_review': False}))


if __name__ == '__main__':
    main()
