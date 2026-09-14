"""Read-only, review-only adapter for original ONE05MotionCapture recordings.

No metadata is synthesized on disk and no historical script is modified.
Completed gameplay failures are retained; incomplete/truncated capture fails.
The legacy recorder did not serialize the measured stop clock, so its real
stop-tail duration remains unknown instead of borrowing the C06 metadata claim.
"""
import hashlib
import json
import re
import wave

import assemble_candidate05_capture as legacy
import assemble_candidate06_capture as common


def require(condition, message):
    if not condition:
        raise ValueError(message)


def prepare(folder, args):
    require(args.review_only, 'Legacy motion is explicitly private review only')
    require(not (folder/'capture.json').exists(), 'Legacy format cannot substitute for a common capture.json recording')
    paths = [legacy.local_input(folder,name) for name in ('frames.csv','checks.txt','gameplay_master.wav')]
    initial = {path.name:legacy.digest(path) for path in paths}
    csv_path, checks_path, audio = paths
    text = legacy.read_record(checks_path)
    require(text.splitlines() and text.splitlines()[0] == 'Candidate05 scripted production-input motion and explicit attack fixtures',
            'Expected the actual legacy motion recorder report')
    counters = {}
    for key in ('Complete','Checks','Failures','Frames','Headless cursor fallback calls'):
        matches = re.findall(r'^'+re.escape(key)+r': ([0-9]+)\s*$',text,re.M)
        require(len(matches) == 1,'Missing/duplicate legacy completion counter: '+key)
        counters[key] = int(matches[0])
    outcomes = re.findall(r'^(PASS|FAIL) \| ([^\r\n]+)\r?$',text,re.M)
    require(counters['Complete'] == 1 and counters['Checks'] > 0 and len(outcomes) == counters['Checks'] and
            sum(outcome == 'FAIL' for outcome,_ in outcomes) == counters['Failures'], 'Legacy recorder is incomplete or assertion counts disagree')
    require(counters['Headless cursor fallback calls'] == 0,'Legacy motion used headless cursor fallback')
    fields = {'file','audio_seconds','world_seconds','phase','weapon','ammo','reserve','operation','frame'}
    rows = legacy.read_csv(csv_path,fields)
    require(2 <= len(rows) <= 6000 and counters['Frames'] == len(rows),'Legacy frame count differs from completed recorder')
    times = [legacy.finite_number(row['audio_seconds'],'audio_seconds') for row in rows]
    world = [legacy.finite_number(row['world_seconds'],'world_seconds') for row in rows]
    phases = [legacy.integer(row['phase'],'phase') for row in rows]
    engine_frames = [legacy.integer(row['frame'],'frame') for row in rows]
    require(all(b>a for a,b in zip(times,times[1:])) and times[-1] < 180,'Legacy audio callbacks must strictly increase within the bounded recording')
    require(all(b>=a for a,b in zip(world,world[1:])) and phases[0] >= 0 and all(b>=a for a,b in zip(phases,phases[1:])),
            'Legacy gameplay time or phase order moved backward')
    require(engine_frames[0] >= 0 and all(b>a for a,b in zip(engine_frames,engine_frames[1:])),'Legacy actual engine frame identities must strictly increase')
    for row in rows:
        for field in ('weapon','ammo','reserve','operation'): legacy.integer(row[field],field)
    layout = common.wave_layout(audio)
    with wave.open(str(audio),'rb') as wav:
        rate, count, channels = wav.getframerate(),wav.getnframes(),wav.getnchannels()
        require(wav.getcomptype() == 'NONE' and wav.getsampwidth() == 2,'Expected original legacy PCM16 audio')
        actual = 0
        while block := wav.readframes(65536): actual += len(block)
    require(actual == count*channels*2 == layout['pcm_bytes'],'Legacy original PCM payload is incomplete')
    duration = count/rate
    require(times[-1] < duration <= min(180,times[-1]+.5),'Legacy WAV must cover every original callback with an unchanged positive tail of at most 0.5 seconds')
    frames, ledger, sizes = [],[],set(); ordered = hashlib.sha256()
    for index,row in enumerate(rows):
        require(row['file'] == f'frame_{index:05d}.jpg','Legacy original frame names must remain contiguous and ordered')
        path = legacy.local_input(folder,row['file']); sizes.add(legacy.jpeg_size(path)); sha = legacy.digest(path)
        ordered.update((json.dumps([row['file'],row['audio_seconds'],sha],separators=(',',':'))+'\n').encode())
        ledger.append({'file':row['file'],'audio_seconds_original':row['audio_seconds'],'sha256':sha,'bytes':path.stat().st_size}); frames.append(path)
    require(len(sizes) == 1 and {path.resolve() for path in folder.glob('frame_*.jpg')} == set(frames),'Legacy viewport dimensions changed or original frame inventory differs')
    width,height = next(iter(sizes)); require(width%2 == 0 and height%2 == 0,'Legacy original viewport must have even dimensions; no resizing is performed')
    labels = {}; chapters_path = folder/'chapters.csv'
    if chapters_path.is_file():
        lines = legacy.read_record(chapters_path).splitlines()
        require(lines and lines[0] == 'phase,seconds,label','Unknown native legacy chapter schema')
        previous_phase,previous_time = -1,-1.
        for line in lines[1:]:
            # This native final text field is intentionally unquoted and may
            # contain commas. Preserve it whole rather than rewrite its file.
            parts = line.split(',',2); require(len(parts)==3,'Incomplete native chapter row')
            phase = legacy.integer(parts[0],'chapter phase'); timestamp = legacy.finite_number(parts[1],'chapter seconds'); label = parts[2]
            require(phase>previous_phase and timestamp>=previous_time and label and not any(ord(c)<32 for c in label) and
                    not re.search(r'[A-Za-z]:[\\/]|\\\\|/(?:Users|home)/',label),'Invalid or private native legacy chapter label/order')
            labels[phase] = label; previous_phase,previous_time = phase,timestamp
        require(all(phase in labels for phase in phases),'Native legacy chapters do not cover the actual frames')
    require(not args.chapters or labels,'Requested legacy chapters require the original chapter file')
    changes = [i for i in range(len(rows)) if i == 0 or phases[i] != phases[i-1]]
    chapters = [{'phase':phases[i],'title':labels[phases[i]],'start_seconds':0 if i == 0 else times[i],
                 'end_seconds':times[changes[n+1]] if n+1<len(changes) else duration} for n,i in enumerate(changes)] if args.chapters else []
    for name in ('poses.csv','input_events.csv','observations.csv','chapters.csv'):
        if (folder/name).is_file(): paths.append(legacy.local_input(folder,name))
    inputs = [{'file':path.name,'bytes':path.stat().st_size,'sha256':legacy.digest(path)} for path in paths]
    require(all(next(row['sha256'] for row in inputs if row['file']==name)==sha for name,sha in initial.items()),'Legacy recording changed during validation')
    actor = {'kind':'legacy-motion','status':'FAILED' if counters['Failures'] else 'PASS','checks':counters['Checks'],'failures':counters['Failures'],
             'completion_counters':counters,'failed_assertions':[label for outcome,label in outcomes if outcome=='FAIL'],
             'report':next(row for row in inputs if row['file']=='checks.txt'),'scope':'Original legacy actor outcomes retained; valid capture bytes do not turn failed gameplay into passing evidence'}
    summary = {'schema':'one07.legacy_motion_review.v1','input_format':'legacy-motion','input_validation':'PASS','frames':len(rows),'dimensions':[width,height],
               'capture_kind':getattr(args,'capture_kind','unknown'),'render_mode':getattr(args,'render_mode','unknown'),
               'source_revision':getattr(args,'source_revision',None),'source_state':getattr(args,'source_state','unknown'),
               'audio_duration_seconds':duration,'audio_sample_rate':rate,'audio_channels':channels,'audio_sample_frames':count,'source_wav_layout':layout,
               'first_frame_audio_seconds':times[0],'last_frame_audio_seconds':times[-1],'ordered_frame_identity_sha256':ordered.hexdigest(),
               'recorded_audio_after_last_callback_seconds':duration-times[-1],'actual_real_stop_tail_seconds':None,'input_files':inputs,'chapters':chapters,
               'timebase_limit':'Legacy audio_seconds is screenshot callback CPU time relative to StartRecordingOutput; audio-render clock offset is unmeasured. world_seconds is fixture elapsed gameplay time.',
               'chapter_timing_limit':'Original native labels preserved; chapter edges use first observed callback of each phase, not exact operation times.',
               'tail_policy':'All callbacks precede the original WAV endpoint. Legacy records do not serialize the actual stop clock or pending-writer flags; no common-recorder transport claim is borrowed.',
               'frame_treatment':'Original full viewport frames held on original callback timestamps; first image covers audio zero and last covers the actual WAV endpoint. No crop, overlay, fabricated pixels or temporal correction.',
               'audio_treatment':'Original full engine WAV; no trim, gain, replacement, resampling, padding or synthetic silence. Movie output transcodes to AAC.',
               'gameplay_checks':actor,'native_input_evidence':False,'performance_evidence':False,'perceptual_audio_review':False}
    common.verify_inputs(folder,inputs+ledger)
    intervals = [(0 if i==0 else times[i],times[i+1] if i+1<len(rows) else duration) for i in range(len(rows))]
    return frames,intervals,audio,summary,ledger
