"""Assemble real timestamped JPEG callbacks and the Unreal master-output WAV.

Python 3.10+, FFmpeg with libx264/AAC and fps_mode on PATH or --ffmpeg. Run after capture, never alongside
performance runs. No generated pixels, motion interpolation or replacement audio.
--validate-only checks inputs without encoding or writing files. Raw captures and
FFmpeg's path-bearing manifests stay local under Saved. Candidate01, Candidate02 and Candidate03 artifacts are untouched.

FFmpeg references: https://ffmpeg.org/ffmpeg-formats.html#concat and
https://ffmpeg.org/ffmpeg.html#Advanced-options (fps_mode, map_chapters).
"""
import argparse
import csv
import hashlib
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
    with csv_path.open(encoding='utf-8-sig', newline='') as stream:
        reader = csv.DictReader(stream)
        if not reader.fieldnames or len(set(reader.fieldnames)) != len(reader.fieldnames):
            raise ValueError('Frame CSV header is missing or duplicated.')
        if not {'file', 'audio_seconds'}.issubset(reader.fieldnames):
            raise ValueError('Frame CSV requires file and audio_seconds columns.')
        rows = list(reader)
    if len(rows) < 2 or any(None in row or any(value is None for value in row.values()) for row in rows):
        raise ValueError('At least two complete actual frame rows are required.')
    times = [finite_number(row['audio_seconds'], 'audio_seconds') for row in rows]
    if any(b <= a for a, b in zip(times, times[1:])):
        raise ValueError('Audio-clock frame timestamps must be strictly increasing; no silent clamping or sorting.')
    if 'world_seconds' in rows[0]:
        world = [finite_number(row['world_seconds'], 'world_seconds') for row in rows]
        if any(b < a for a, b in zip(world, world[1:])):
            raise ValueError('World timestamps move backward; use a separate capture after a level restart.')
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
    for row, frame in zip(rows, frames):
        if frame.suffix.lower() not in ('.jpg', '.jpeg'):
            raise ValueError('Only the actual JPEG capture inputs are supported.')
        sizes.add(jpeg_size(frame))
        identity = [frame.relative_to(folder).as_posix(), row['audio_seconds'], digest(frame)]
        frame_identity.update((json.dumps(identity, ensure_ascii=True, separators=(',', ':'))+'\n').encode())
    if len(sizes) != 1:
        raise ValueError('Capture dimensions changed during the run; assemble separate clips.')
    width, height = next(iter(sizes))
    if width % 2 or height % 2:
        raise ValueError('yuv420p requires even capture dimensions; this tool does not resize actual frames.')
    phases = None
    if args.chapters:
        if 'phase' not in rows[0]:
            raise ValueError('The selected mode/chapter option requires a phase column.')
        try:
            phases = [int(row['phase']) for row in rows]
        except ValueError:
            raise ValueError('Phase identifiers must be integers.') from None
    changes = [] if phases is None else [i for i in range(len(phases)) if i == 0 or phases[i] != phases[i-1]]
    observed = [] if phases is None else [phases[i] for i in changes]
    labels = {}
    native_chapters = folder/'chapters.csv'
    if args.chapters and native_chapters.is_file():
        with native_chapters.open(encoding='utf-8-sig', newline='') as stream:
            chapter_rows = list(csv.DictReader(stream))
        for row in chapter_rows:
            phase = str(int(row['phase']))
            label = row['label']
            if phase in labels or not label.strip() or any(ord(c) < 32 for c in label):
                raise ValueError('Native chapter IDs must be unique with single-line labels.')
            labels[phase] = label
        if any(str(phase) not in labels for phase in observed):
            raise ValueError('Native chapter labels do not cover every observed phase.')
    if args.phase_labels:
        supplied = json.loads(args.phase_labels.read_text(encoding='utf-8-sig'))
        if not isinstance(supplied, dict) or any(not isinstance(v, str) or not v.strip() or any(ord(c) < 32 for c in v) for v in supplied.values()):
            raise ValueError('Phase labels must be a JSON object of phase ID strings to nonempty single-line titles.')
        labels.update(supplied)
    chapters = []
    if args.chapters:
        for n, index in enumerate(changes):
            start = 0 if index == 0 else times[index]
            end = times[changes[n+1]] if n+1 < len(changes) else duration
            chapters.append({'phase': phases[index], 'title': labels.get(str(phases[index]), f'Phase {phases[index]}'),
                             'start_seconds': start, 'end_seconds': end})
    intervals = [(0 if i == 0 else times[i], times[i+1] if i+1 < len(rows) else duration) for i in range(len(rows))]
    report = {'candidate': 'Candidate04', 'capture_kind': args.capture_kind, 'capture_mode': args.mode,
              'provenance_basis': 'Runtime kind and source identity supplied by operator; encoding does not establish packaged validation or visual/audio approval.',
              'source_revision': args.source_revision, 'source_state': args.source_state,
              'source': 'Actual gameplay JPEG screenshot callbacks plus Unreal master-output WAV',
              'frames': len(rows), 'dimensions': [width, height], 'capture_span_seconds': times[-1]-times[0],
              'actual_capture_fps': (len(rows)-1)/(times[-1]-times[0]),
              'first_frame_audio_seconds': times[0], 'last_frame_audio_seconds': times[-1],
              'audio_duration_seconds': duration, 'final_frame_hold_seconds': duration-times[-1],
              'maximum_frame_hold_seconds': max(end-start for start, end in intervals),
              'source_audio': audio_info, 'frames_csv_sha256': digest(csv_path), 'source_wav_sha256': digest(audio),
              'ordered_frame_identity_sha256': frame_identity.hexdigest(),
              'frame_identity_method': 'SHA256 of UTF-8 JSON lines [relative filename, original audio_seconds string, JPEG SHA256] in CSV order',
              'video_output_fps': 30,
              'frame_treatment': 'Captured images held according to callback timestamps; first image held from audio time zero. CFR output duplicates or drops captured frames on the 30fps grid; no generated pixels, motion interpolation or retiming to hide stalls.',
              'audio_treatment': 'AAC transcode of the supplied engine WAV at original duration; this assembler does not replace, pad, stretch or synthesize audio. The source gameplay may itself use provisional synthesized sound assets.',
              'audio_review': 'Encoding does not establish perceptual audition or audible-phase coverage; run the separate gameplay-audio audit.',
              'observed_phase_order': observed, 'chapters': chapters,
              'chapter_timing': 'First observed frame of each contiguous phase, not an exact engine operation boundary.'}
    if args.chapters and native_chapters.is_file():
        report['native_chapters_sha256'] = digest(native_chapters)
    report['input_provenance'] = 'Recorder observation/input_events.csv distinguish scripted production dispatch from passive native input; this assembler does not infer human operation.'
    return frames, intervals, audio, report


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
    p.add_argument('--mode', choices=('generic',), default='generic')
    p.add_argument('--chapters', action='store_true')
    p.add_argument('--phase-labels', type=pathlib.Path, help='Optional JSON object mapping phase IDs to chapter titles')
    p.add_argument('--source-revision', help='Actual built full commit, or base commit when source-state is working-tree')
    p.add_argument('--source-state', choices=('exact-commit', 'working-tree', 'unknown'), default='unknown')
    p.add_argument('--replace', action='store_true', help='Replace intended C04 MP4/report only')
    p.add_argument('--validate-only', action='store_true', help='Read/validate inputs and print provenance plan; no writes or encoder process')
    args = p.parse_args()
    if args.source_revision and not re.fullmatch(r'[0-9a-f]{40}', args.source_revision):
        p.error('Source revision must be a full lowercase Git commit.')
    if args.source_state == 'exact-commit' and not args.source_revision:
        p.error('Exact-commit provenance requires the actual built revision.')
    if args.phase_labels and not args.chapters:
        p.error('--phase-labels requires --chapters.')
    output = args.output.resolve()
    if output.suffix.lower() != '.mp4' or any(part.lower() in ('candidate01', 'candidate02', 'candidate03') for part in output.parts):
        p.error('Choose an MP4 destination outside preserved Candidate01/02/03 artifacts.')
    sidecar = output.with_suffix('.json')
    if args.phase_labels and args.phase_labels.resolve() in (output, sidecar):
        p.error('Output must not replace phase-label input.')
    if not args.validate_only and not args.replace and (output.exists() or sidecar.exists()):
        p.error('Output or provenance report exists; use --replace only for the intended C04 destination.')
    try:
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
    scratch_root = ROOT/'Saved/Candidate04/Assembly'
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
        report.update({'encoded': True, 'encoder_exit_code': 0, 'output_sha256': digest(pending), 'output_bytes': pending.stat().st_size})
        pending.replace(output)
        sidecar.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    finally:
        pending.unlink(missing_ok=True)
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
