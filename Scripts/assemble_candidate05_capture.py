"""Assemble real timestamped JPEG callbacks and the Unreal master-output WAV.

Python 3.10+, FFmpeg with libx264/AAC and fps_mode on PATH or --ffmpeg. Run after capture, never alongside
performance runs. No generated pixels, motion interpolation or replacement audio.
--validate-only checks inputs without encoding or writing files. Raw captures and
FFmpeg's path-bearing manifests stay local under Saved/Candidate05/Assembly.
Candidate01 through Candidate04 artifacts are protected. Only completed, passing
Candidate05 motion/presentation/manual captures are accepted. All callbacks must
precede the WAV endpoint; this tool has no clipping or audio-padding option.

FFmpeg references: https://ffmpeg.org/ffmpeg-formats.html#concat and
https://ffmpeg.org/ffmpeg.html#Advanced-options (fps_mode, map_chapters).
"""
import argparse
import csv
import hashlib
import io
import json
import math
import pathlib
import re
import shutil
import struct
import subprocess
import tempfile
import uuid
import wave

ROOT = pathlib.Path(__file__).resolve().parents[1]


def read_record(path):
    """FFileHelper emits UTF-8 or BOM-marked UTF-16 depending on its contents."""
    data = path.read_bytes()
    return data.decode('utf-16' if data.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')


def read_csv(path, required):
    reader = csv.DictReader(io.StringIO(read_record(path), newline=''))
    if not reader.fieldnames or len(set(reader.fieldnames)) != len(reader.fieldnames):
        raise ValueError('CSV header is missing or duplicated: '+path.name)
    if not required.issubset(reader.fieldnames):
        raise ValueError('Required recorder columns are missing: '+path.name)
    rows = list(reader)
    if any(None in row or any(value is None for value in row.values()) for row in rows):
        raise ValueError('A recorder CSV row has missing or extra cells: '+path.name)
    return rows


def integer(value, label):
    if not re.fullmatch(r'-?\d+', value):
        raise ValueError(label+' must be an integer.')
    return int(value)


def completion(path, mode, frame_count):
    record = read_record(path)
    heading = ('Candidate05 scripted production-input motion and explicit attack fixtures' if mode == 'motion'
               else 'Candidate05 machine/inventory presentation')
    if not record.splitlines() or record.splitlines()[0] != heading:
        raise ValueError('checks.txt does not identify the requested Candidate05 recorder.')
    passive = 'Passive native-input recorder:' in record
    if mode != 'motion' and passive != (mode == 'manual'):
        raise ValueError('Requested presentation/manual mode disagrees with checks.txt.')
    if 'Profile: recording disabled.' in record:
        raise ValueError('A non-recording profile run cannot be assembled as video.')
    result = {}
    keys = ['Complete', 'Checks', 'Failures', 'Frames']
    keys += ['Headless cursor fallback calls'] if mode == 'motion' else ['Profile actual frames']
    for key in keys:
        matches = re.findall(r'^'+re.escape(key)+r': ([0-9]+)\s*$', record, re.MULTILINE)
        if len(matches) != 1:
            raise ValueError('Expected exactly one final recorder counter: '+key)
        result[key] = int(matches[0])
    passed = len(re.findall(r'^PASS \| .+$', record, re.MULTILINE))
    failed = len(re.findall(r'^FAIL \| .+$', record, re.MULTILINE))
    if result['Complete'] != 1 or result['Failures'] != 0 or result['Checks'] <= 0:
        raise ValueError('Only completed, passing capture reports can be assembled.')
    if passed+failed != result['Checks'] or failed != result['Failures']:
        raise ValueError('Assertion rows disagree with final checks/failures counters.')
    if result['Frames'] != frame_count:
        raise ValueError('Final recorder frame count disagrees with frames.csv.')
    if mode == 'motion' and result['Headless cursor fallback calls'] != 0:
        raise ValueError('Motion capture reported a headless cursor fallback.')
    if mode != 'motion' and result['Profile actual frames'] != 0:
        raise ValueError('Video capture unexpectedly includes profile samples.')
    return result


def output_paths(args):
    output = args.output.resolve()
    folder = args.input.resolve()
    lower_parts = [part.lower() for part in output.parts]
    if output.suffix.lower() != '.mp4' or any(re.search(r'candidate0[1-4](?!\d)', part) for part in lower_parts):
        raise ValueError('Choose a Candidate05 MP4 outside preserved Candidate01 through Candidate04 artifacts.')
    if 'candidate05' not in lower_parts:
        raise ValueError('Output must be inside an explicitly named Candidate05 directory.')
    if output.is_relative_to(folder):
        raise ValueError('Keep assembled outputs outside the original capture folder.')
    sidecar = output.with_suffix('.json')
    if not args.validate_only and not args.replace and (output.exists() or sidecar.exists()):
        raise ValueError('Output or report exists; --replace applies only to the intended Candidate05 output pair.')
    return output, sidecar


def digest(path):
    before = path.stat()
    result = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            result.update(block)
    after = path.stat()
    if (before.st_size, before.st_mtime_ns) != (after.st_size, after.st_mtime_ns):
        raise ValueError('An input changed during validation; wait for capture to finish.')
    return result.hexdigest()


def jpeg_size(path):
    """Validate JPEG signature/size; FFmpeg's -xerror does final pixel decoding."""
    with path.open('rb') as stream:
        if stream.read(2) != b'\xff\xd8':
            raise ValueError('A listed frame is not JPEG data.')
        while True:
            marker = stream.read(1)
            if marker != b'\xff':
                raise ValueError('Malformed JPEG marker before image dimensions.')
            while marker == b'\xff':
                marker = stream.read(1)
            if not marker or marker[0] in (0xd9, 0xda):
                raise ValueError('JPEG dimensions are missing.')
            if marker[0] in (0xd8, 0x01) or 0xd0 <= marker[0] <= 0xd7:
                continue
            raw_length = stream.read(2)
            if len(raw_length) != 2:
                raise ValueError('Truncated JPEG header.')
            length = int.from_bytes(raw_length, 'big')
            if length < 2:
                raise ValueError('Invalid JPEG segment length.')
            if marker[0] in (0xc0, 0xc1, 0xc2):
                data = stream.read(5)
                if len(data) != 5:
                    raise ValueError('Truncated JPEG size.')
                height, width = struct.unpack('>HH', data[1:])
                stream.seek(-2, 2)
                if stream.read(2) != b'\xff\xd9' or min(width, height) <= 0:
                    raise ValueError('Truncated JPEG or invalid dimensions.')
                return width, height
            stream.seek(length-2, 1)


def finite_number(value, label):
    number = float(value)
    if not math.isfinite(number) or number < 0:
        raise ValueError(label+' must be finite and nonnegative.')
    return number


def local_input(folder, name):
    if not name or any(ord(char) < 32 for char in name):
        raise ValueError('Empty or control-character input filename.')
    portable = pathlib.PurePosixPath(name.replace('\\', '/'))
    if portable.is_absolute() or pathlib.PureWindowsPath(name).drive or '..' in portable.parts:
        raise ValueError('Frame paths must be relative and contained in the capture folder.')
    path = (folder/pathlib.Path(*portable.parts)).resolve()
    if not path.is_relative_to(folder) or not path.is_file():
        raise ValueError('A listed capture input is missing or resolves outside its folder.')
    if any(char in path.as_posix() for char in '\r\n\x00'):
        raise ValueError('Input path cannot be represented in the concat manifest.')
    return path


def prepare(args):
    folder = args.input.resolve()
    csv_path = local_input(folder, 'frames.csv')
    audio = local_input(folder, 'gameplay_master.wav')
    checks_path = local_input(folder, 'checks.txt')
    initial_hashes = {path.name: digest(path) for path in (csv_path, audio, checks_path)}
    required = {'file', 'audio_seconds', 'world_seconds', 'phase', 'weapon', 'ammo', 'reserve', 'operation'}
    if args.mode == 'motion':
        required.add('frame')
    rows = read_csv(csv_path, required)
    if len(rows) < 2:
        raise ValueError('At least two complete actual frame rows are required.')
    counters = completion(checks_path, args.mode, len(rows))
    times = [finite_number(row['audio_seconds'], 'audio_seconds') for row in rows]
    if any(b <= a for a, b in zip(times, times[1:])):
        raise ValueError('Audio-clock frame timestamps must be strictly increasing; no silent clamping or sorting.')
    world = [finite_number(row['world_seconds'], 'world_seconds') for row in rows]
    if any(b < a for a, b in zip(world, world[1:])):
        raise ValueError('Recorder elapsed timestamps move backward; use a separate capture after a level restart.')
    phases = [integer(row['phase'], 'phase') for row in rows]
    for row in rows:
        for field in ('weapon', 'ammo', 'reserve', 'operation'):
            integer(row[field], field)
    if args.mode == 'motion':
        frame_counters = [integer(row['frame'], 'frame') for row in rows]
        if any(value < 0 for value in frame_counters) or any(b <= a for a, b in zip(frame_counters, frame_counters[1:])):
            raise ValueError('Motion callback engine frame counters must be nonnegative and strictly increasing.')
    with wave.open(str(audio), 'rb') as wav:
        if wav.getcomptype() != 'NONE' or wav.getframerate() <= 0 or wav.getnframes() <= 0:
            raise ValueError('A nonempty PCM engine WAV is required.')
        audio_info = {'sample_rate': wav.getframerate(), 'channels': wav.getnchannels(),
                      'sample_width_bytes': wav.getsampwidth(), 'sample_frames': wav.getnframes()}
        duration = wav.getnframes()/wav.getframerate()
        # wave permits a short underlying data payload; do not accept a truncated capture.
        expected = wav.getnframes()*wav.getnchannels()*wav.getsampwidth()
        actual = 0
        while block := wav.readframes(65536):
            actual += len(block)
        if actual != expected:
            raise ValueError('WAV sample payload is truncated.')
    if not times[-1] < duration <= times[-1]+.5:
        raise ValueError('WAV duration and final frame disagree; recapture rather than padding or stretching audio.')
    frames = [local_input(folder, row['file']) for row in rows]
    if len(set(frames)) != len(frames):
        raise ValueError('Duplicate input frame paths are not independent captures.')
    sizes = set()
    frame_identity = hashlib.sha256()
    frame_ledger = []
    for row, frame in zip(rows, frames):
        if frame.suffix.lower() not in ('.jpg', '.jpeg'):
            raise ValueError('Only the actual JPEG capture inputs are supported.')
        sizes.add(jpeg_size(frame))
        identity = [frame.relative_to(folder).as_posix(), row['audio_seconds'], digest(frame)]
        frame_identity.update((json.dumps(identity, ensure_ascii=True, separators=(',', ':'))+'\n').encode())
        frame_ledger.append({'file': identity[0], 'audio_seconds_original': identity[1],
                             'sha256': identity[2], 'bytes': frame.stat().st_size})
    if len(sizes) != 1:
        raise ValueError('Capture dimensions changed during the run; assemble separate clips.')
    width, height = next(iter(sizes))
    if width % 2 or height % 2:
        raise ValueError('yuv420p requires even capture dimensions; this tool does not resize actual frames.')
    changes = [i for i in range(len(phases)) if i == 0 or phases[i] != phases[i-1]]
    observed = [phases[i] for i in changes]
    labels = {}
    native_chapters = folder/'chapters.csv'
    if args.chapters:
        native_chapters = local_input(folder, 'chapters.csv')
        chapter_time = 'seconds' if args.mode == 'motion' else 'world_seconds'
        chapter_rows = read_csv(native_chapters, {'phase', chapter_time, 'label'})
        previous_time = -1.0
        for row in chapter_rows:
            phase = str(integer(row['phase'], 'chapter phase'))
            label = row['label']
            if phase in labels or not label.strip() or any(ord(c) < 32 for c in label) or re.search(r'(?:[A-Za-z]:[\\/]|/Users/|/home/|\\\\)', label):
                raise ValueError('Native chapter IDs must be unique with single-line labels.')
            chapter_timestamp = finite_number(row[chapter_time], chapter_time)
            if chapter_timestamp < previous_time:
                raise ValueError('Native chapter timestamps move backward.')
            previous_time = chapter_timestamp
            labels[phase] = label
        if any(str(phase) not in labels for phase in observed):
            raise ValueError('Native chapter labels do not cover every observed phase.')
    chapters = []
    if args.chapters:
        for n, index in enumerate(changes):
            start = 0 if index == 0 else times[index]
            end = times[changes[n+1]] if n+1 < len(changes) else duration
            chapters.append({'phase': phases[index], 'title': labels[str(phases[index])],
                             'start_seconds': start, 'end_seconds': end})
    intervals = [(0 if i == 0 else times[i], times[i+1] if i+1 < len(rows) else duration) for i in range(len(rows))]
    source_files = [csv_path, audio, checks_path]
    # Keep raw observations private, but bind them to the recording without
    # copying their contents or machine paths into the portable sidecar.
    for name in ('input_events.csv', 'poses.csv', 'observations.csv', 'chapters.csv'):
        if (folder/name).is_file():
            source_files.append(local_input(folder, name))
    input_files = [{'file': path.relative_to(folder).as_posix(), 'sha256': digest(path),
                    'bytes': path.stat().st_size} for path in source_files]
    hashes = {entry['file']: entry['sha256'] for entry in input_files}
    if any(hashes[name] != original for name, original in initial_hashes.items()):
        raise ValueError('Recorder inputs changed while being validated; wait for capture to finish.')
    report = {'candidate': 'Candidate05', 'capture_kind': args.capture_kind, 'capture_mode': args.mode,
              'render_mode': args.render_mode,
              'provenance_basis': 'Runtime kind and source identity supplied by operator; encoding does not establish packaged validation or visual/audio approval.',
              'source_revision': args.source_revision, 'source_state': args.source_state,
              'source': 'Actual gameplay JPEG screenshot callbacks plus Unreal master-output WAV',
              'frames': len(rows), 'dimensions': [width, height], 'capture_span_seconds': times[-1]-times[0],
              'actual_capture_fps': (len(rows)-1)/(times[-1]-times[0]),
              'first_frame_audio_seconds': times[0], 'last_frame_audio_seconds': times[-1],
              'audio_duration_seconds': duration, 'final_frame_hold_seconds': duration-times[-1],
              'maximum_frame_hold_seconds': max(end-start for start, end in intervals),
              'source_audio': audio_info, 'frames_csv_sha256': hashes['frames.csv'], 'source_wav_sha256': hashes['gameplay_master.wav'],
              'checks_txt_sha256': hashes['checks.txt'], 'completion_counters': counters,
              'input_files': input_files, 'source_frames': frame_ledger,
              'ordered_frame_identity_sha256': frame_identity.hexdigest(),
              'frame_identity_method': 'SHA256 of UTF-8 JSON lines [relative filename, original audio_seconds string, JPEG SHA256] in CSV order',
              'video_output_fps': 30,
              'endpoint_policy': 'Strict: every original callback is retained in the source sequence and precedes the unmodified WAV endpoint; final hold is positive and at most 0.5 seconds. No terminal clipping.',
              'frame_treatment': 'Captured images held according to callback timestamps; first image held from audio time zero. CFR output duplicates or drops captured frames on the 30fps grid; no generated pixels, motion interpolation or retiming to hide stalls.',
              'audio_treatment': 'AAC transcode of the supplied engine WAV at original duration; this assembler does not replace, pad, stretch or synthesize audio. The source gameplay may itself use provisional synthesized sound assets.',
              'audio_review': 'Encoding does not establish perceptual audition or audible-phase coverage; run the separate gameplay-audio audit.',
              'observed_phase_order': observed, 'chapters': chapters,
              'chapter_timing': 'First observed frame of each contiguous phase, not an exact engine operation boundary.'}
    if args.chapters:
        report['native_chapters_sha256'] = hashes['chapters.csv']
    report['input_provenance'] = 'Recorder observation/input_events.csv distinguish scripted production dispatch from passive native input; this assembler does not infer human operation.'
    return frames, intervals, audio, report


def verify_unchanged(folder, report):
    for entry in report['input_files']+report['source_frames']:
        path = local_input(folder, entry['file'])
        if path.stat().st_size != entry['bytes'] or digest(path) != entry['sha256']:
            raise ValueError('An original capture input changed during assembly; output is rejected.')


def concat_text(frames, intervals):
    lines = ['ffconcat version 1.0']
    for frame, (start, end) in zip(frames, intervals):
        # FFmpeg concat quoting, not shell quoting. Per-file framerate supplies a
        # millisecond demuxer timebase; output is explicitly sampled at30fps.
        lines.extend(["file '"+frame.as_posix().replace("'", "'\\''")+"'", 'option framerate 1000', f'duration {end-start:.6f}'])
    lines.extend(["file '"+frames[-1].as_posix().replace("'", "'\\''")+"'", 'option framerate 1000'])
    return '\n'.join(lines)+'\n'


def chapter_text(chapters):
    lines = [';FFMETADATA1']
    for chapter in chapters:
        title = re.sub(r'([\\=;#])', r'\\\1', chapter['title'])
        lines.extend(['[CHAPTER]', 'TIMEBASE=1/1000000', f"START={round(chapter['start_seconds']*1000000)}",
                      f"END={round(chapter['end_seconds']*1000000)}", 'title='+title])
    return '\n'.join(lines)+'\n'


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--input', type=pathlib.Path, required=True)
    p.add_argument('--output', type=pathlib.Path, required=True)
    p.add_argument('--ffmpeg', default='ffmpeg')
    p.add_argument('--capture-kind', choices=('packaged', 'editor-game'), required=True)
    p.add_argument('--mode', choices=('motion', 'presentation', 'manual'), required=True)
    p.add_argument('--render-mode', choices=('offscreen', 'windowed', 'unknown'), default='unknown',
                   help='Operator-supplied rendering mode; does not establish native human input')
    p.add_argument('--chapters', action='store_true', help='Opt in only to validated original chapters.csv labels')
    p.add_argument('--source-revision', help='Actual built full commit, or base commit when source-state is working-tree')
    p.add_argument('--source-state', choices=('exact-commit', 'working-tree', 'unknown'), default='unknown')
    p.add_argument('--replace', action='store_true', help='Replace intended Candidate05 MP4/report only')
    p.add_argument('--validate-only', action='store_true', help='Read/validate inputs and print provenance plan; no writes or encoder process')
    args = p.parse_args()
    if args.source_revision and not re.fullmatch(r'[0-9a-f]{40}', args.source_revision):
        p.error('Source revision must be a full lowercase Git commit.')
    if args.source_state != 'unknown' and not args.source_revision:
        p.error('Known source provenance requires the actual built revision or working-tree base revision.')
    try:
        output, sidecar = output_paths(args)
        frames, intervals, audio, report = prepare(args)
    except (OSError, ValueError, wave.Error, csv.Error, struct.error) as error:
        p.error(str(error))
    if args.validate_only:
        report.update({'encoded': False, 'validation_only': True, 'files_written': 0})
        print(json.dumps(report, indent=2))
        return
    ffmpeg = shutil.which(args.ffmpeg)
    if not ffmpeg:
        p.error('FFmpeg was not found; supply its installed executable with --ffmpeg.')
    # Path-bearing concat/metadata files are deliberately local-only, never sidecars.
    scratch_root = ROOT/'Saved/Candidate05/Assembly'
    scratch_root.mkdir(parents=True, exist_ok=True)
    scratch = pathlib.Path(tempfile.mkdtemp(prefix='run_', dir=scratch_root))
    concat = scratch/'frames.ffconcat'; concat.write_text(concat_text(frames, intervals), encoding='utf-8')
    command = [ffmpeg, '-nostdin', '-y', '-hide_banner', '-loglevel', 'warning', '-xerror',
               '-f', 'concat', '-safe', '0', '-i', str(concat), '-i', str(audio)]
    if report['chapters']:
        metadata = scratch/'chapters.ffmetadata'; metadata.write_text(chapter_text(report['chapters']), encoding='utf-8')
        command.extend(['-f', 'ffmetadata', '-i', str(metadata)])
    command.extend(['-map', '0:v:0', '-map', '1:a:0', '-map_metadata', '-1', '-map_metadata:s:v', '-1', '-map_metadata:s:a', '-1',
                    '-map_chapters', '2' if report['chapters'] else '-1',
                    '-c:v', 'libx264', '-crf', '20', '-preset', 'medium', '-pix_fmt', 'yuv420p', '-r', '30', '-fps_mode', 'cfr',
                    '-c:a', 'aac', '-b:a', '192k', '-t', f"{report['audio_duration_seconds']:.9f}", '-movflags', '+faststart'])
    output.parent.mkdir(parents=True, exist_ok=True)
    pending = output.parent/(output.stem+'.'+uuid.uuid4().hex+'.partial.mp4')
    try:
        subprocess.run(command+[str(pending)], check=True)
        if not pending.is_file() or pending.stat().st_size == 0:
            raise RuntimeError('Encoder returned without a playable output payload.')
        verify_unchanged(args.input.resolve(), report)
        report.update({'encoded': True, 'encoder_exit_code': 0, 'output_sha256': digest(pending), 'output_bytes': pending.stat().st_size})
        pending.replace(output)
        sidecar.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    finally:
        pending.unlink(missing_ok=True)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
