"""Original Candidate04 M1911 and upgraded shot banks, plus pistol mechanics.

Python 3 standard library. Run this file, then --check. The editable synthesis
below reuses Project ONE's original C03 filter/contact primitives, never a WAV
recording, sample pack, external service or whole-recording pitch shift.
No engine is launched. Objective checks do not constitute auditory approval.
"""
from pathlib import Path
import argparse
import array
import hashlib
import json
import math
import sys
import wave

sys.dont_write_bytecode = True
import create_candidate03_audio as synth

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource/Audio/Candidate04'
SR = 48000
TAU = math.tau
PISTOL_PROFILES = [
    (1850, 590, .020, .031, [.019, .047, .092]),
    (2130, 650, .018, .034, [.024, .054, .084]),
    (1640, 530, .023, .029, [.017, .041, .101]),
    (2360, 705, .017, .037, [.026, .059, .096]),
    (1940, 610, .021, .032, [.021, .050, .109]),
    (2250, 560, .019, .035, [.028, .044, .089]),
]


def pistol(index):
    edge, body, decay, action, reflections = PISTOL_PROFILES[index-1]
    seed = 104000 + index*193
    n = round(.40*SR)
    front = synth.burst(n, seed, edge, 13500, .00008, .0034)
    values = list(front)
    synth.add(values, synth.burst(n, seed+7, 95, body, .00023, decay), .0006, 2.8)
    synth.add(values, synth.burst(n, seed+11, 420, 4900, .00012, .017+index*.0006), .0011, .65)
    # The small pistol slide and receiver have their own short double contact.
    synth.add(values, synth.contact(seed+17), action, .32)
    synth.add(values, synth.contact(seed+19), action+.014, .13)
    dry = synth.lowpass(values, 3600+index*90)
    for at, gain in zip(reflections, (.12, .047, .022)):
        synth.add(values, dry, at, gain)
    synth.add(values, synth.burst(n, seed+31, 590, 4300, .016, .039), .014, .13)
    return values, {'seed': seed, 'attack_highpass_hz': edge, 'pressure_upper_hz': body,
                    'pressure_decay_seconds': decay, 'slide_contact_seconds': action,
                    'reflection_seconds': reflections}


def energized(base, name, index):
    """Add a separately authored energy transient/tail; keep firearm pitch1."""
    family = {'LastWord': 0, 'Overcurrent': 1, 'Gravebreaker': 2}[name]
    length = (.52, .56, .80)[family]
    n = round(length*SR)
    out = list(base) + [0.]*(n-len(base))
    seed = 144000+family*10000+index*229
    # Variant changes include sweep/tail frequencies and event timing, not just
    # random seed. Energy is short, inharmonic and subordinate to pressure body.
    start = (5100, 4300, 3550)[family] + (index-3)*135
    end = (2250, 1700, 1190)[family] + (index-3)*43
    decay = (.025, .020, .032)[family] + index*.0007
    energy = synth.burst(n, seed, start*.52, 13000, .00016, decay*.72)
    phase = 0.
    for i in range(n):
        t = i/SR
        frequency = end+(start-end)*math.exp(-t/.009)
        phase += TAU*frequency/SR
        env = (-math.expm1(-t/.00022))*math.exp(-t/decay)
        energy[i] += .23*math.sin(phase)*env + .055*math.sin(phase*1.417)*env
    synth.add(out, energy, .00035+index*.00007, .57 if family<2 else .74)
    echo = synth.band(energy, 1200, 5600)
    tail_times = [.041+index*.0019, .073+index*.0013, .119+index*.0021]
    for at, gain in zip(tail_times, (.13, .055, .025)):
        synth.add(out, echo, at, gain)
    return out, {'energy_seed':seed, 'energy_sweep_start_hz':start, 'energy_sweep_end_hz':end,
                 'energy_decay_seconds':decay, 'energy_tail_seconds':tail_times,
                 'firearm_pitch_ratio':1.0, 'pump_in_shot':False}


def master(values, peak_db):
    values = synth.band(values, 38, 15000)
    fade = round(.016*SR)
    for i in range(fade): values[-i-1] *= .5-.5*math.cos(math.pi*i/fade)
    # Remove residual DC without applying per-layer normalization.
    mean = sum(values)/len(values)
    values = [v-mean for v in values]
    gain = 10**(peak_db/20)/max(abs(v) for v in values)
    return [v*gain for v in values]


def designs():
    for family in ('M1911', 'LastWord', 'Overcurrent', 'Gravebreaker'):
        for index in range(1,7):
            if family in ('M1911', 'LastWord'):
                values, profile = pistol(index)
            else:
                values, _, profile = synth.design('Carbine' if family=='Overcurrent' else 'Shotgun', index)
            if family!='M1911':
                values, energy = energized(values, family, index)
                profile = {'firearm':profile, 'energy':energy}
            name = f'S_C04_{family}Shot_{index:02d}'
            yield name, master(values, -5.5 if family=='M1911' else -5.0), {'family':family, 'variant':index, 'profile':profile, 'kind':'shot'}
    for index, (event, duration, tone, decay) in enumerate([
        ('Empty', .12, 2800, .004), ('MagOut', .18, 1250, .009),
        ('MagIn', .19, 1650, .012), ('Slide', .20, 2450, .008)]):
        n=round(duration*SR); seed=188000+index*73
        values=synth.burst(n,seed,700,tone*2,.0002,decay)
        synth.add(values,synth.contact(seed+5),.007+index*.004,.22)
        if event=='Slide':
            synth.add(values,synth.burst(n,seed+7,850,6900,.001,.012),.022,.5)
            synth.add(values,synth.contact(seed+11),.058,.35)
        yield 'S_C04_Pistol'+event, master(values,-11.), {'family':'M1911','kind':'mechanical','event':event,'seed':seed}


def pcm_bytes(values):
    pcm=array.array('h',(round(v*32767) for v in values))
    if sys.byteorder!='little': pcm.byteswap()
    return pcm.tobytes()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    if not args.check: OUT.mkdir(parents=True,exist_ok=True)
    manifest={'candidate':'04','status':'Original authored synthesis; perceptual timbre/mix approval remains provisional.',
        'provenance':'Only editable Project ONE synthesis code and deterministic noise. No imported recordings, services, sample packs or Project Zero assets.',
        'source_generators':['Scripts/create_candidate04_audio.py','Scripts/create_candidate03_audio.py'],
        'runtime_contract':'Six no-immediate-repeat shots per new bank, pitch1.0. Upgrade energy layered independently over firearm body. Shotgun pump remains separate mechanical events.',
        'events':{}}
    for name,values,profile in designs():
        path=OUT/(name+'.wav'); expected=pcm_bytes(values)
        if not args.check:
            with wave.open(str(path),'wb') as wav:
                wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(SR); wav.writeframes(expected)
        with wave.open(str(path),'rb') as wav:
            assert (wav.getnchannels(),wav.getsampwidth(),wav.getframerate())==(1,2,SR)
            assert wav.readframes(wav.getnframes())==expected, 'PCM differs from editable generator '+name
        measured=synth.read_pcm(path); metrics=synth.metrics(measured)
        assert metrics['full_scale_samples']==0 and metrics['peak_dbfs']<=-4.99
        assert metrics['nonzero_samples']>200 and abs(metrics['dc_offset'])<.0001
        manifest['events'][name]={'source':path.relative_to(ROOT).as_posix(), 'asset':'/Game/ONE/Audio/Weapons/Candidate04/'+name,
            'sha256':hashlib.sha256(path.read_bytes()).hexdigest(), **profile, **metrics}
    assert len(manifest['events'])==28
    assert len({e['sha256'] for e in manifest['events'].values()})==28
    destination=OUT/'manifest.json'
    if args.check:
        assert json.loads(json.dumps(manifest))==json.loads(destination.read_text()), 'Manifest differs from authored sources'
        print('CANDIDATE04_AUDIO_SOURCE_CHECK_PASS 28 original sources; no audition claim')
    else:
        destination.write_text(json.dumps(manifest,indent=2)+'\n')
        print('CANDIDATE04_AUDIO_SOURCE_GENERATED 24 shot variants and4 mechanical events')


if __name__=='__main__': main()
