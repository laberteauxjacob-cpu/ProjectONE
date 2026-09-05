"""Encode actual timestamped gameplay JPEGs and Unreal master-output WAV.

Python 3, FFmpeg on PATH or --ffmpeg. No synthesized/interpolated video frames.
The first JPEG is held over the short initial capture latency. Encoding is
separate from benchmark runs. Source captures remain local under Saved.
"""
import argparse
import csv
import json
import pathlib
import subprocess
import wave

p = argparse.ArgumentParser()
p.add_argument('--input', type=pathlib.Path, required=True)
p.add_argument('--output', type=pathlib.Path, required=True)
p.add_argument('--ffmpeg', default='ffmpeg')
args = p.parse_args()
rows = list(csv.DictReader((args.input/'frames.csv').open()))
if len(rows) < 2:
    raise SystemExit('At least two actual captures are required.')
times = [float(r['audio_seconds']) for r in rows]
audio = args.input/'gameplay_master.wav'
with wave.open(str(audio)) as wav:
    duration = wav.getnframes()/wav.getframerate()
if duration < times[-1]-.15 or duration > times[-1]+.5:
    raise SystemExit('Audio and captured frame timelines disagree; recapture without omitted silent buffers.')
concat = args.input/'frames.ffconcat'
lines = ['ffconcat version 1.0']
for i, row in enumerate(rows):
    frame = (args.input/row['file']).resolve()
    if not frame.is_file():
        raise SystemExit('Missing actual capture: '+str(frame))
    start = 0 if i == 0 else times[i]
    end = times[i+1] if i+1 < len(rows) else duration
    lines.extend(["file '"+frame.as_posix().replace("'", "'\\''")+"'", f'duration {max(.001,end-start):.6f}'])
lines.append(lines[-2])
concat.write_text('\n'.join(lines)+'\n')
args.output.parent.mkdir(parents=True, exist_ok=True)
subprocess.run([args.ffmpeg,'-y','-hide_banner','-loglevel','warning','-f','concat','-safe','0','-i',str(concat),'-i',str(audio),'-c:v','libx264','-crf','20','-preset','medium','-pix_fmt','yuv420p','-r','30','-c:a','aac','-b:a','192k','-t',str(duration),'-movflags','+faststart',str(args.output)], check=True)
report = {'source':'Actual packaged gameplay screenshot callback plus Unreal master-submix WAV', 'frames':len(rows), 'capture_span_seconds':times[-1]-times[0], 'actual_capture_fps':(len(rows)-1)/(times[-1]-times[0]), 'first_frame_audio_seconds':times[0], 'audio_duration_seconds':duration, 'video_output_fps':30, 'frame_treatment':'Timestamped captured frames held as needed; no AI generation or motion interpolation', 'audio_review':'Engine output captured; perceptual audition not established by encoding'}
args.output.with_suffix('.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
