"""Measure a genuine gameplay master WAV; no perceptual-quality verdict.

Python 3 standard library only. Example from the project checkout:
  python Scripts/audit_gameplay_audio.py --source Packaged/Candidate02/Windows/ProjectONE/Saved/Candidate02/Comparison/gameplay_master.wav

Phase ranges use seconds in the recorded WAV, not game/world time. Override with
--phase name:start:end (repeatable). Missing requested coverage is reported and
can make the process fail with --require-phase-coverage. Source/output paths are
stored relative to the checkout; no machine or account paths enter the report.
"""
from pathlib import Path
import argparse
import array
import datetime
import hashlib
import io
import json
import math
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]


def relative_path(value):
    path = Path(value)
    path = path.resolve() if path.is_absolute() else (ROOT/path).resolve()
    try:
        relative = path.relative_to(ROOT).as_posix()
    except ValueError:
        raise argparse.ArgumentTypeError('Use a path inside the project checkout.') from None
    return path, relative


def phase_spec(value):
    try:
        name, start, end = value.split(':')
        start, end = float(start), float(end)
        if not name or not math.isfinite(start+end) or start < 0 or end <= start:
            raise ValueError
    except ValueError:
        raise argparse.ArgumentTypeError('Phase must be name:start:end with 0 <= start < end.') from None
    return {'name': name, 'start_seconds': start, 'end_seconds': end}


def dbfs(amplitude):
    return round(20*math.log10(amplitude), 6) if amplitude > 0 else None


def decode_pcm(raw, width):
    if width == 3:
        return array.array('i', (int.from_bytes(raw[index:index+3], 'little', signed=True) for index in range(0, len(raw), 3)))
    values = array.array({1: 'B', 2: 'h', 4: 'i'}[width])
    values.frombytes(raw)
    if sys.byteorder != 'little' and width > 1:
        values.byteswap()
    if width == 1:
        return array.array('h', (value-128 for value in values))
    return values


def audit(source, phases, window_ms, silence_dbfs, event_dbfs, merge_gap_ms):
    source_bytes = source.read_bytes()
    with wave.open(io.BytesIO(source_bytes), 'rb') as wav:
        channels, width, rate, declared_frames, compression, _ = wav.getparams()
        if compression != 'NONE' or width not in (1, 2, 3, 4):
            raise ValueError('The auditor requires uncompressed 8/16/24/32-bit PCM WAV.')
        raw = wav.readframes(declared_frames)
    if channels < 1 or rate < 1 or len(raw) != declared_frames*channels*width:
        raise ValueError('WAV data is truncated or has invalid format parameters.')
    if declared_frames == 0:
        raise ValueError('The WAV has no sample frames.')
    values = decode_pcm(raw, width)
    scale = 2**(width*8-1)
    silence_limit = scale*10**(silence_dbfs/20)
    near_clip_limit = scale*10**(-.1/20)
    duration = declared_frames/rate

    def metrics(first_frame, end_frame):
        frames = end_frame-first_frame
        if frames <= 0:
            return None
        sums = [0]*channels
        squares = [0]*channels
        peaks = [0]*channels
        clips = [0]*channels
        nonzero = [0]*channels
        near_clips = [0]*channels
        zero_frames = silent_frames = longest_zero = longest_silent = zero_run = silent_run = 0
        for frame in range(first_frame, end_frame):
            frame_peak = 0
            for channel in range(channels):
                value = values[frame*channels+channel]
                magnitude = abs(value)
                sums[channel] += value
                squares[channel] += value*value
                peaks[channel] = max(peaks[channel], magnitude)
                frame_peak = max(frame_peak, magnitude)
                clips[channel] += int(value <= -scale or value >= scale-1)
                near_clips[channel] += int(magnitude >= near_clip_limit)
                nonzero[channel] += int(value != 0)
            zero = frame_peak == 0
            silent = frame_peak <= silence_limit
            zero_frames += zero
            silent_frames += silent
            zero_run = zero_run+1 if zero else 0
            silent_run = silent_run+1 if silent else 0
            longest_zero = max(longest_zero, zero_run)
            longest_silent = max(longest_silent, silent_run)
        peak = max(peaks)/scale
        rms = math.sqrt(sum(squares)/(frames*channels))/scale
        per_channel = []
        for index in range(channels):
            channel_rms = math.sqrt(squares[index]/frames)/scale
            dc = sums[index]/frames/scale
            per_channel.append({'channel_index': index, 'sample_peak_dbfs': dbfs(peaks[index]/scale),
                'rms_dbfs': dbfs(channel_rms), 'dc_normalized': round(dc, 10), 'absolute_dc_dbfs': dbfs(abs(dc)),
                'full_scale_sample_count': clips[index], 'nonzero_sample_count': nonzero[index]})
        return {'duration_seconds': round(frames/rate, 8), 'frame_count': frames, 'channel_sample_count': frames*channels,
            'sample_peak_normalized': round(peak, 10), 'sample_peak_dbfs': dbfs(peak), 'rms_normalized': round(rms, 10),
            'rms_dbfs': dbfs(rms), 'crest_factor_db': round(20*math.log10(peak/rms), 6) if rms else None,
            'sample_peak_headroom_db': round(-20*math.log10(peak), 6) if peak else None,
            'full_scale_sample_count': sum(clips), 'near_full_scale_samples_above_minus_0_1_dbfs': sum(near_clips),
            'nonzero_sample_count': sum(nonzero), 'nonzero_sample_percent': round(100*sum(nonzero)/(frames*channels), 6),
            'exact_zero_frame_count': zero_frames, 'longest_exact_zero_seconds': round(longest_zero/rate, 8),
            'silent_frame_count_at_threshold': silent_frames, 'silent_frame_percent_at_threshold': round(100*silent_frames/frames, 6),
            'longest_silence_seconds_at_threshold': round(longest_silent/rate, 8), 'channels': per_channel}

    window_frames = max(1, round(window_ms*rate/1000))
    windows = []
    for first in range(0, declared_frames, window_frames):
        end = min(first+window_frames, declared_frames)
        total_squared = peak_integer = 0
        for index in range(first*channels, end*channels):
            value = values[index]
            total_squared += value*value
            peak_integer = max(peak_integer, abs(value))
        rms = math.sqrt(total_squared/((end-first)*channels))/scale
        windows.append([round(first/rate, 8), round(end/rate, 8), dbfs(rms), dbfs(peak_integer/scale)])

    # Threshold crossings locate measurable energy bursts, not inferred shots.
    # Closely spaced sounds can share a burst; quiet sounds can fall below it.
    events = []
    for window in windows:
        if window[2] is None or window[2] < event_dbfs:
            continue
        if not events or window[0]-events[-1]['end_seconds'] > merge_gap_ms/1000+1e-8:
            events.append({'start_seconds': window[0], 'end_seconds': window[1],
                'peak_window_start_seconds': window[0], 'peak_window_rms_dbfs': window[2],
                'sample_peak_dbfs': window[3], 'active_window_count': 1})
        else:
            event = events[-1]
            event['end_seconds'] = window[1]
            event['active_window_count'] += 1
            event['sample_peak_dbfs'] = max(event['sample_peak_dbfs'], window[3])
            if window[2] > event['peak_window_rms_dbfs']:
                event['peak_window_rms_dbfs'] = window[2]
                event['peak_window_start_seconds'] = window[0]
    for event in events:
        event['duration_seconds'] = round(event['end_seconds']-event['start_seconds'], 8)

    measurements = []
    for spec in phases:
        start, end = spec['start_seconds'], spec['end_seconds']
        first = min(declared_frames, round(start*rate))
        last = min(declared_frames, round(end*rate))
        complete = duration+1/rate >= end
        covered = max(0, last-first)/rate
        measurement = dict(spec)
        measurement.update(coverage='complete' if complete else ('partial' if covered else 'missing'),
            available_start_seconds=round(first/rate, 8), available_end_seconds=round(last/rate, 8),
            requested_duration_seconds=end-start, covered_duration_seconds=round(covered, 8),
            covered_fraction=round(covered/(end-start), 8), metrics=metrics(first, last),
            overlapping_energy_event_indices=[index for index, event in enumerate(events) if event['start_seconds'] < last/rate and event['end_seconds'] > first/rate])
        measurements.append(measurement)

    overall = metrics(0, declared_frames)
    warnings = []
    if any(phase['coverage'] != 'complete' for phase in measurements):
        warnings.append('One or more requested phase windows extend past the recorded WAV; those phase metrics cover only available samples.')
    if overall['full_scale_sample_count']:
        warnings.append('Full-scale integer samples occur; count alone cannot establish where clipping entered the signal chain.')
    if not overall['nonzero_sample_count']:
        warnings.append('The recording is entirely digital silence.')
    comparison = None
    by_name = {phase['name']: phase for phase in measurements}
    carbine, shotgun = by_name.get('carbine'), by_name.get('shotgun')
    if carbine and shotgun and carbine['metrics'] and shotgun['metrics']:
        left, right = carbine['metrics'], shotgun['metrics']
        comparison = {'phase_windows_fully_covered': carbine['coverage'] == shotgun['coverage'] == 'complete',
            'shotgun_minus_carbine_rms_db': round(right['rms_dbfs']-left['rms_dbfs'], 6) if left['rms_dbfs'] is not None and right['rms_dbfs'] is not None else None,
            'shotgun_minus_carbine_sample_peak_db': round(right['sample_peak_dbfs']-left['sample_peak_dbfs'], 6) if left['sample_peak_dbfs'] is not None and right['sample_peak_dbfs'] is not None else None,
            'interpretation_limit': 'These compare recorded mix windows, including firing cadence, distance, other events and silence. They are not isolated weapon loudness or perceived quality measurements.'}
    return {'status': 'MEASURED', 'source_sha256': hashlib.sha256(source_bytes).hexdigest(), 'source_file_bytes': len(source_bytes),
        'format': {'encoding': 'integer PCM', 'sample_rate_hz': rate, 'channels': channels, 'bits_per_sample': width*8,
                   'frame_count': declared_frames, 'duration_seconds': round(duration, 8)},
        'overall': overall, 'phases': measurements, 'phase_comparison': comparison,
        'energy_events': events, 'energy_event_count': len(events),
        'envelope': {'window_columns': ['start_seconds', 'end_seconds', 'rms_dbfs_all_channels', 'maximum_sample_peak_dbfs'],
                     'actual_window_frames': window_frames, 'actual_window_ms': window_frames/rate*1000, 'windows': windows},
        'warnings': warnings,
        'measurement_limits': ['No perceptual audition was performed; available audio-input tooling reported unsupported input.',
            'Sample peaks are not oversampled true peaks, LUFS, or a guarantee against intersample clipping.',
            'Full-scale sample counts use signed PCM endpoint values; near-full-scale counts use -0.1 dBFS.',
            'Silence uses the maximum absolute channel sample per frame at the configured threshold; exact-zero counts are separate.',
            'Energy events group RMS-threshold windows with the configured merge gap; they are not verified shot or animation event counts.',
            'Phase labels are supplied capture annotations; no automatic identification of weapon type or event synchrony is claimed.',
            'Null dBFS means exactly zero measured amplitude, mathematically negative infinity.']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', required=True, type=relative_path)
    parser.add_argument('--output', default='Evidence/Candidate02/audio_metrics.json', type=relative_path)
    parser.add_argument('--phase', action='append', type=phase_spec, help='Repeat name:start:end; defaults to carbine:2:5 and shotgun:9:13')
    parser.add_argument('--window-ms', type=float, default=10)
    parser.add_argument('--silence-threshold-dbfs', type=float, default=-60)
    parser.add_argument('--event-threshold-dbfs', type=float, default=-38)
    parser.add_argument('--event-merge-gap-ms', type=float, default=50)
    parser.add_argument('--require-phase-coverage', action='store_true')
    args = parser.parse_args()
    if not 0 < args.window_ms <= 1000 or not 0 <= args.event_merge_gap_ms <= 1000:
        parser.error('Window must be (0,1000] ms and merge gap [0,1000] ms.')
    if not -160 <= args.silence_threshold_dbfs <= 0 or not -160 <= args.event_threshold_dbfs <= 0:
        parser.error('Thresholds must be finite and within -160..0 dBFS.')
    phases = args.phase or [phase_spec('carbine:2:5'), phase_spec('shotgun:9:13')]
    if len({phase['name'] for phase in phases}) != len(phases):
        parser.error('Phase names must be unique.')
    source, source_relative = args.source
    output, output_relative = args.output
    if source == output:
        parser.error('Output must differ from the source recording.')
    report = audit(source, phases, args.window_ms, args.silence_threshold_dbfs, args.event_threshold_dbfs, args.event_merge_gap_ms)
    report.update(generated_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(), source=source_relative,
        command_parameters={'script': 'Scripts/audit_gameplay_audio.py', 'source': source_relative, 'output': output_relative,
            'phases': phases, 'window_ms': args.window_ms, 'silence_threshold_dbfs': args.silence_threshold_dbfs,
            'event_threshold_dbfs': args.event_threshold_dbfs, 'event_merge_gap_ms': args.event_merge_gap_ms,
            'require_phase_coverage': args.require_phase_coverage})
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2, allow_nan=False)+'\n', encoding='utf-8')
    print(json.dumps({'report': output_relative, 'duration_seconds': report['format']['duration_seconds'],
        'sample_peak_dbfs': report['overall']['sample_peak_dbfs'], 'rms_dbfs': report['overall']['rms_dbfs'],
        'full_scale_sample_count': report['overall']['full_scale_sample_count'],
        'phase_coverage': {phase['name']: phase['coverage'] for phase in report['phases']}, 'warnings': report['warnings']}))
    if args.require_phase_coverage and any(phase['coverage'] != 'complete' for phase in report['phases']):
        return 2
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
