"""Assemble genuine timestamped Unreal screenshot frames as a silent animated capture.
No synthesized frames; timing comes from runtime frame filenames.
This is a low-frame-rate scripted gameplay capture, not a manual playthrough.
"""
from pathlib import Path
from PIL import Image
import sys, csv, json
root=Path(__file__).resolve().parents[1]
folder=Path(sys.argv[1]) if len(sys.argv)>1 else root/'Saved'/'Validation'
files=sorted(folder.glob('record_*.png'))
if len(files)<2: raise RuntimeError('No runtime frame sequence found')
times=[int(p.stem.split('_')[1]) for p in files]
frames=[]
for p in files:
    im=Image.open(p).convert('RGB')
    im.thumbnail((1200,675),Image.Resampling.LANCZOS)
    frames.append(im)
# GIF durations have 10ms precision; round rather than repeatedly truncating time.
durations=[max(30,min(1000,round((b-a)/10)*10)) for a,b in zip(times,times[1:])]+[130]
out=root/'Evidence'/'ScriptedGameplay.gif'
frames[0].save(out,save_all=True,append_images=frames[1:],duration=durations,loop=0,optimize=False)
requests=[]
if (folder/'frames.csv').exists():
    with (folder/'frames.csv').open() as f:
        requests=[int(row['timestamp_ms']) for row in csv.DictReader(f)]
with Image.open(out) as gif:
    count=gif.n_frames
    duration=0
    for i in range(count):
        gif.seek(i); duration+=gif.info.get('duration',0)
manifest={
    'source':'Actual packaged Unreal framebuffer PNGs, ordinary gameplay camera, scripted driver',
    'source_folder':str(folder.resolve()), 'saved_png_frames':len(files),
    'requested_frames':len(requests), 'requests_without_png':sorted(set(requests)-set(times)),
    'first_timestamp_ms':times[0], 'last_timestamp_ms':times[-1],
    'average_saved_frame_rate':(len(times)-1)*1000/(times[-1]-times[0]),
    'gif_frames':count, 'gif_duration_ms':duration,
    'note':'One screenshot request may replace another in the same engine frame. Only existing genuine PNGs are encoded. No interpolation or synthetic frames. Silent GIF; times rounded to centiseconds.'
}
(root/'Evidence'/'capture_manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(f'{len(frames)} genuine engine frames; {sum(durations)/1000:.2f}s; {out}')
