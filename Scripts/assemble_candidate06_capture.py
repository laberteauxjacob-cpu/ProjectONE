"""Assemble ONE06CaptureComponent's actual viewport JPEGs and master-output WAV.

Python 3.10+. Supply --ffmpeg as an explicit installed executable path. The
default report is <movie>.assembly.json. --validate-only reads inputs and prints
a compact plan without writing files or invoking FFmpeg. Run encoding separately
from performance measurements. Existing outputs and earlier candidates cannot
be replaced. Path-bearing concat manifests and the full source-frame inventory
remain private under Saved/Candidate06/Assembly.

The original audio timeline is retained. The first image covers audio time zero
until the next callback, and the final image holds through the real WAV endpoint.
30fps output samples those holds; there is no motion interpolation, audio padding,
setpts/tempo correction, or synthetic silence. Full output probing/decoding and
perceptual review are separate operations, not implied by this assembler.
"""
import argparse
import csv
import hashlib
import json
import math
import os
import pathlib
import re
import struct
import subprocess
import tempfile
import uuid
import wave

# Reuse the established timestamp concat, path confinement, JPEG header and
# stable-hash guards. Candidate05's CLI and acceptance rules are not invoked.
from assemble_candidate05_capture import (
    chapter_text, concat_text, digest, finite_number, integer, jpeg_size,
    local_input, read_csv, read_record,
)

ROOT = pathlib.Path(__file__).resolve().parents[1]


def strict_json(path):
    def pairs(items):
        result = {}
        for key, value in items:
            if key in result:
                raise ValueError('Duplicate JSON field in capture metadata: '+key)
            result[key] = value
        return result

    return json.loads(read_record(path), object_pairs_hook=pairs,
                      parse_constant=lambda value: (_ for _ in ()).throw(ValueError('Nonfinite capture JSON value.')))


def number(record, key):
    value = record.get(key)
    if isinstance(value, bool) or not isinstance(value, (int, float)):
        raise ValueError('Capture metadata requires a numeric '+key)
    return finite_number(value, key)


def close_number(record, key, expected, tolerance=1e-7):
    if not math.isclose(number(record, key), expected, rel_tol=0, abs_tol=tolerance):
        raise ValueError('Actual input disagrees with capture metadata: '+key)


def outputs(args):
    folder = args.input.resolve(strict=True)
    if not folder.is_dir():
        raise ValueError('Input must be a completed capture directory.')
    output = args.output.resolve()
    sidecar = output.with_suffix('.assembly.json')
    parts = [part.lower() for part in output.parts]
    if output.suffix.lower() != '.mp4' or 'candidate06' not in parts:
        raise ValueError('Output must be an MP4 inside an explicitly named Candidate06 directory.')
    if any(re.search(r'candidate0[1-5](?!\d)', part) for part in parts):
        raise ValueError('Earlier candidate artifacts are protected.')
    if output.is_relative_to(folder) or sidecar.is_relative_to(folder):
        raise ValueError('Assembled outputs must remain outside the original capture directory.')
    if output.exists() or sidecar.exists() or output.is_symlink() or sidecar.is_symlink():
        raise ValueError('Output or assembly report already exists; choose a new output name.')
    if not args.ffmpeg.is_absolute():
        raise ValueError('--ffmpeg must name an explicit absolute executable path.')
    ffmpeg = args.ffmpeg.resolve(strict=True)
    if not ffmpeg.is_file():
        raise ValueError('--ffmpeg does not identify an existing binary file.')
    return folder, output, sidecar, ffmpeg


def wave_layout(audio):
    """Strict PCM layout with the narrowly identified UE5.7 exporter exception."""
    size = audio.stat().st_size
    fmt = data = None
    with audio.open('rb') as stream:
        header = stream.read(12)
        if len(header) != 12 or header[:4] != b'RIFF' or header[8:] != b'WAVE' or struct.unpack('<I', header[4:8])[0]+8 != size:
            raise ValueError('WAV RIFF length must match the complete actual file.')
        while stream.tell()+8 <= size:
            offset = stream.tell()
            kind, length = struct.unpack('<4sI', stream.read(8))
            end = stream.tell()+length
            if end+(length & 1) > size:
                raise ValueError('WAV chunk extends beyond the actual file.')
            if kind == b'fmt ':
                if fmt is not None or length < 16:
                    raise ValueError('WAV PCM format chunk is duplicated or incomplete.')
                fmt = (offset, length, *struct.unpack('<HHIIHH', stream.read(16)))
            elif kind == b'data':
                if data is not None:
                    raise ValueError('WAV data chunk is duplicated.')
                data = (offset, length)
            stream.seek(end+(length & 1))
        if stream.tell() != size or fmt is None or data is None:
            raise ValueError('WAV has missing chunks or an incomplete trailing chunk.')
    offset, length, codec, channels, rate, byte_rate, block_align, bits = fmt
    if codec != 1 or bits != 16 or not 1 <= channels <= 16 or not 8000 <= rate <= 384000 or data[1] <= 0:
        raise ValueError('Expected nonempty engine PCM16 with valid channels and sample rate.')
    frame_bytes = channels*2
    # Engine/Private/Audio.cpp SerializeWaveFile hardcodes BlockAlign=2,
    # although its ByteRate and actual interleaved PCM account for channels.
    unreal_quirk = channels > 1 and block_align == 2 and offset == 12 and length == 16 and data[0] == 36 and size == 44+data[1]
    if byte_rate != rate*frame_bytes or data[1] % frame_bytes or (block_align != frame_bytes and not unreal_quirk):
        raise ValueError('WAV byte rate, channel alignment or complete sample payload is invalid.')
    return {'header_block_align': block_align, 'computed_frame_bytes': frame_bytes,
            'unreal_block_align_quirk': unreal_quirk, 'pcm_bytes': data[1], 'channels': channels, 'rate': rate}


def prepare(folder, args):
    csv_path = local_input(folder, 'frames.csv')
    metadata_path = local_input(folder, 'capture.json')
    audio = local_input(folder, 'gameplay_master.wav')
    source_files = [{'file': path.name, 'sha256': digest(path), 'bytes': path.stat().st_size}
                    for path in (metadata_path, csv_path, audio)]
    capture = strict_json(metadata_path)
    if not isinstance(capture, dict) or capture.get('schema') != 'one06.engine_capture.v1':
        raise ValueError('Expected ONE06CaptureComponent v1 metadata.')
    if capture.get('status') != 'PASS' or capture.get('failure_reason') != '':
        raise ValueError('Only a completed passing engine capture can be assembled.')
    if capture.get('audio_finalized_stable_nonempty') is not True:
        raise ValueError('Capture did not establish a finalized stable nonempty WAV.')
    for key in ('screenshot_pending', 'image_writer_pending', 'native_input_evidence', 'performance_evidence'):
        if capture.get(key) is not False:
            raise ValueError('Capture requires false '+key)
    if capture.get('frames_csv') != 'frames.csv' or capture.get('audio_file') != 'gameplay_master.wav':
        raise ValueError('Capture metadata names unexpected primary inputs.')
    if not isinstance(capture.get('timebase'), str) or not capture['timebase'].strip():
        raise ValueError('Capture metadata must disclose its audio/frame timebase.')
    rows = read_csv(csv_path, {'file', 'audio_seconds', 'world_seconds', 'phase', 'label', 'engine_frame', 'width', 'height'})
    if not 2 <= len(rows) <= 6000:
        raise ValueError('A capture must contain 2 through 6000 complete actual frames.')
    close_number(capture, 'frames_received', len(rows), 0)
    close_number(capture, 'frames_written', len(rows), 0)
    close_number(capture, 'maximum_frames', 6000, 0)
    close_number(capture, 'maximum_recording_seconds', 180, 0)
    times = [finite_number(row['audio_seconds'], 'audio_seconds') for row in rows]
    world = [finite_number(row['world_seconds'], 'world_seconds') for row in rows]
    engine_frames = [integer(row['engine_frame'], 'engine_frame') for row in rows]
    phases = [integer(row['phase'], 'phase') for row in rows]
    if any(b <= a for a, b in zip(times, times[1:])) or times[-1] >= 180:
        raise ValueError('Audio timestamps must increase strictly within the hard recording limit; no sorting or clamping.')
    if any(b < a for a, b in zip(world, world[1:])):
        raise ValueError('World timestamps move backward; level restarts require a separate capture.')
    if engine_frames[0] < 0 or any(b <= a for a, b in zip(engine_frames, engine_frames[1:])):
        raise ValueError('Actual callback engine frame counters must increase strictly.')
    if phases[0] < 0 or any(b < a for a, b in zip(phases, phases[1:])):
        raise ValueError('Recorded phase IDs must be nonnegative and cannot move backward.')
    close_number(capture, 'first_frame_audio_seconds', times[0])
    close_number(capture, 'last_frame_audio_seconds', times[-1])
    labels = {}
    metadata_phases = capture.get('phases')
    if not isinstance(metadata_phases, list):
        raise ValueError('Capture metadata has no phase-label array.')
    for index, item in enumerate(metadata_phases):
        if not isinstance(item, dict) or number(item, 'phase') != index:
            raise ValueError('Capture phase metadata must have contiguous ordered IDs.')
        label = item.get('label')
        if not isinstance(label, str) or not label or len(label) > 128 or any(ord(c) < 32 for c in label):
            raise ValueError('Capture labels must be nonempty single-line text of at most 128 characters.')
        if re.search(r'(?:[A-Za-z]:[\\/]|/Users/|/home/|\\\\)', label):
            raise ValueError('Do not place private absolute paths in portable chapter labels.')
        labels[index] = label
    if any(labels.get(phase) != row['label'] for phase, row in zip(phases, rows)):
        raise ValueError('Frame labels disagree with their actual capture phase metadata.')
    layout = wave_layout(audio)
    with wave.open(str(audio), 'rb') as wav:
        if wav.getcomptype() != 'NONE' or wav.getsampwidth() != 2 or not 1 <= wav.getnchannels() <= 16:
            raise ValueError('Expected actual engine PCM16 audio with 1 through 16 channels.')
        rate, count, channels = wav.getframerate(), wav.getnframes(), wav.getnchannels()
        if not 8000 <= rate <= 384000 or count <= 0:
            raise ValueError('Engine WAV sample rate/count is invalid.')
        pcm_bytes = count*channels*2
        actual_bytes = 0
        while block := wav.readframes(65536):
            actual_bytes += len(block)
        if actual_bytes != pcm_bytes:
            raise ValueError('WAV PCM payload is truncated.')
    duration = count/rate
    if not times[-1] < duration <= 180:
        raise ValueError('Actual WAV must cover the last callback and remain within the hard recording limit.')
    if (layout['channels'], layout['rate'], layout['pcm_bytes']) != (channels, rate, pcm_bytes):
        raise ValueError('WAV header and independent PCM-reader facts disagree.')
    for key, value in [('audio_bytes', audio.stat().st_size), ('audio_pcm_bytes', pcm_bytes),
                       ('audio_channels', channels), ('audio_sample_rate', rate), ('audio_bits_per_sample', 16)]:
        close_number(capture, key, value, 0)
    close_number(capture, 'audio_duration_seconds', duration, 1/rate)
    close_number(capture, 'audio_header_block_align', layout['header_block_align'], 0)
    close_number(capture, 'audio_computed_frame_bytes', layout['computed_frame_bytes'], 0)
    if capture.get('audio_unreal_block_align_quirk') is not layout['unreal_block_align_quirk']:
        raise ValueError('Actual WAV layout disagrees with the recorded exporter quirk.')
    close_number(capture, 'minimum_real_audio_tail_seconds', .4)
    real_tail = number(capture, 'actual_real_audio_tail_seconds')
    stop_clock = number(capture, 'stop_audio_clock_seconds')
    if real_tail < .4-1e-7 or stop_clock-times[-1] < real_tail-1e-7 or stop_clock >= 180:
        raise ValueError('Capture must retain the actual .4-second stop tail within its hard limit.')
    frames, ledger = [], []
    sizes = set()
    ordered_identity = hashlib.sha256()
    for index, row in enumerate(rows):
        # Component names are exact, single-level and contiguous. The shared
        # resolver additionally rejects symlinks escaping the original folder.
        if row['file'] != f'frame_{index:05d}.jpg':
            raise ValueError('Actual component frame names must be contiguous in original CSV order.')
        frame = local_input(folder, row['file'])
        size = jpeg_size(frame)
        if size != (integer(row['width'], 'width'), integer(row['height'], 'height')):
            raise ValueError('Actual JPEG dimensions disagree with their frame row.')
        sizes.add(size)
        sha = digest(frame)
        identity = [row['file'], row['audio_seconds'], sha]
        ordered_identity.update((json.dumps(identity, ensure_ascii=True, separators=(',', ':'))+'\n').encode())
        ledger.append({'file': row['file'], 'audio_seconds_original': row['audio_seconds'],
                       'sha256': sha, 'bytes': frame.stat().st_size})
        frames.append(frame)
    if len(set(frames)) != len(frames) or len(sizes) != 1:
        raise ValueError('Capture frames must be distinct files with unchanged full viewport dimensions.')
    if {path.resolve() for path in folder.glob('frame_*.jpg')} != set(frames):
        raise ValueError('Extra or missing frame JPEGs disagree with the finalized component count.')
    width, height = next(iter(sizes))
    close_number(capture, 'width', width, 0); close_number(capture, 'height', height, 0)
    if width % 2 or height % 2:
        raise ValueError('yuv420p requires even dimensions; this assembler does not crop or resize actual images.')
    intervals = [(0 if i == 0 else times[i], times[i+1] if i+1 < len(rows) else duration) for i in range(len(rows))]
    changes = [i for i in range(len(rows)) if i == 0 or phases[i] != phases[i-1]]
    chapters = []
    if args.chapters:
        for position, index in enumerate(changes):
            chapters.append({'phase': phases[index], 'title': labels[phases[index]],
                             'start_seconds': 0 if index == 0 else times[index],
                             'end_seconds': times[changes[position+1]] if position+1 < len(changes) else duration})
    summary = {
        'schema': 'one06.capture_assembly.v1', 'candidate': 'Candidate06', 'input_validation': 'PASS',
        'encoded': False, 'full_output_probe_and_decode': 'Not performed by this assembler',
        'capture_kind': args.capture_kind, 'render_mode': args.render_mode,
        'source_revision': args.source_revision, 'source_state': args.source_state,
        'source_binding_limit': 'Runtime kind/revision are operator supplied; encoding does not independently establish package identity or parent-check acceptance.',
        'source': 'Actual full engine viewport JPEG callbacks and actual engine master-output PCM WAV',
        'native_input_evidence': False, 'performance_evidence': False, 'perceptual_audio_review': False,
        'frames': len(rows), 'dimensions': [width, height],
        'first_frame_audio_seconds': times[0], 'last_frame_audio_seconds': times[-1],
        'capture_span_seconds': times[-1]-times[0], 'actual_callback_fps': (len(rows)-1)/(times[-1]-times[0]),
        'audio_duration_seconds': duration, 'actual_real_stop_tail_seconds': real_tail,
        'stop_audio_clock_seconds': stop_clock, 'recorded_audio_after_last_callback_seconds': duration-times[-1],
        'first_image_prepadding_seconds': times[0], 'final_image_hold_seconds': duration-times[-1],
        'maximum_image_hold_seconds': max(end-start for start, end in intervals),
        'audio_sample_rate': rate, 'audio_channels': channels, 'audio_sample_frames': count,
        'source_wav_layout': layout,
        'wav_layout_limit': 'UE5.7 SerializeWaveFile may write blockAlign=2 for stereo. Only its exact 44-byte PCM16 header exception is accepted with correct byteRate and complete channel-aligned payload; original WAV bytes are not changed.',
        'input_files': source_files, 'ordered_frame_identity_sha256': ordered_identity.hexdigest(),
        'frame_identity_method': 'SHA256 of UTF-8 JSON lines [relative filename, original audio_seconds string, JPEG SHA256] in CSV order',
        'input_pixel_validation': 'JPEG signature, complete-file end marker and actual SOF dimensions; full decoding occurs during FFmpeg encoding and independent output decode review.',
        'timebase_limit': capture.get('timebase'),
        'tail_policy': 'A measured real-time stop delay of at least .4 seconds is required separately from positive recorded-WAV coverage after the last callback. CPU and audio-render clocks have an unmeasured start offset; no correction is applied.',
        'frame_treatment': 'First actual image held from audio time zero; subsequent actual images held by original callback timestamps; last image held to unmodified WAV endpoint. Millisecond concat timebase, CFR30 samples may duplicate/drop captured images; no generated pixels, crop, overlays, motion interpolation or retiming to hide stalls.',
        'audio_treatment': 'AAC transcode of the original engine WAV on its original timeline; no replacement, padding, tempo change or synthetic silence. Codec/container endpoint quantization requires separate output probing. This does not establish timbre/localization approval.',
        'video_output_fps': 30, 'chapters': chapters,
        'chapter_timing_limit': 'Chapter boundaries use the first observed frame of each phase, not exact operation boundaries.',
    }
    verify_inputs(folder, source_files+ledger)
    return frames, intervals, audio, summary, ledger


def verify_inputs(folder, entries):
    for entry in entries:
        path = local_input(folder, entry['file'])
        if path.stat().st_size != entry['bytes'] or digest(path) != entry['sha256']:
            raise ValueError('An original capture input changed during validation/assembly; output rejected.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=pathlib.Path, required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    parser.add_argument('--ffmpeg', type=pathlib.Path, required=True)
    parser.add_argument('--capture-kind', choices=('packaged', 'editor-game'), required=True)
    parser.add_argument('--render-mode', choices=('offscreen', 'windowed', 'unknown'), default='unknown')
    parser.add_argument('--source-revision')
    parser.add_argument('--source-state', choices=('exact-commit', 'working-tree', 'unknown'), default='unknown')
    parser.add_argument('--chapters', action='store_true')
    parser.add_argument('--validate-only', action='store_true')
    args = parser.parse_args()
    if args.source_revision and not re.fullmatch(r'[0-9a-f]{40}', args.source_revision):
        parser.error('Source revision must be a full lowercase Git commit.')
    if args.source_state != 'unknown' and not args.source_revision:
        parser.error('Known source state requires the actual built commit or working-tree base commit.')
    try:
        folder, output, sidecar, ffmpeg = outputs(args)
        frames, intervals, audio, summary, ledger = prepare(folder, args)
        if args.validate_only:
            summary.update({'validation_only': True, 'files_written': 0})
            print(json.dumps(summary, indent=2)); return
        scratch_root = ROOT/'Saved/Candidate06/Assembly'
        scratch_root.mkdir(parents=True, exist_ok=True)
        scratch = pathlib.Path(tempfile.mkdtemp(prefix='run_', dir=scratch_root))
        concat = scratch/'frames.ffconcat'
        concat.write_text(concat_text(frames, intervals), encoding='utf-8')
        (scratch/'source_frame_inventory.json').write_text(json.dumps(ledger, indent=2)+'\n', encoding='utf-8')
        command = [str(ffmpeg), '-nostdin', '-n', '-hide_banner', '-loglevel', 'warning', '-xerror',
                   '-f', 'concat', '-safe', '0', '-i', str(concat), '-i', str(audio)]
        if summary['chapters']:
            metadata = scratch/'chapters.ffmetadata'
            metadata.write_text(chapter_text(summary['chapters']), encoding='utf-8')
            command += ['-f', 'ffmetadata', '-i', str(metadata)]
        command += ['-map', '0:v:0', '-map', '1:a:0', '-map_metadata', '-1',
                    '-map_metadata:s:v', '-1', '-map_metadata:s:a', '-1',
                    '-map_chapters', '2' if summary['chapters'] else '-1',
                    '-c:v', 'libx264', '-crf', '20', '-preset', 'medium', '-pix_fmt', 'yuv420p',
                    '-vf', 'tpad=stop_mode=clone:stop_duration=1,fps=30',
                    '-r', '30', '-fps_mode', 'cfr', '-c:a', 'aac', '-b:a', '192k',
                    '-t', f"{summary['audio_duration_seconds']:.9f}", '-movflags', '+faststart']
        output.parent.mkdir(parents=True, exist_ok=True)
        pending = output.parent/(output.stem+'.'+uuid.uuid4().hex+'.partial.mp4')
        try:
            subprocess.run(command+[str(pending)], check=True)
            if not pending.is_file() or pending.stat().st_size <= 0:
                raise ValueError('Encoder returned without a nonempty output.')
            verify_inputs(folder, summary['input_files']+ledger)
            summary.update({'encoded': True, 'encoder_exit_code': 0, 'encoder_binary_sha256': digest(ffmpeg),
                            'output_file': output.name, 'output_sha256': digest(pending), 'output_bytes': pending.stat().st_size})
            # Same-directory hard linking atomically refuses an existing target;
            # unlike replace(), it cannot overwrite a file created during encode.
            os.link(pending, output)
            with sidecar.open('x', encoding='utf-8') as stream:
                stream.write(json.dumps(summary, indent=2)+'\n')
        finally:
            pending.unlink(missing_ok=True)
        print(json.dumps(summary, indent=2))
    except (OSError, ValueError, wave.Error, csv.Error, struct.error, subprocess.CalledProcessError) as error:
        parser.error(str(error))


if __name__ == '__main__':
    main()
