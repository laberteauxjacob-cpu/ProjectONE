"""Original Candidate03 shot design. Python 3 standard library only.

Run: python Scripts/create_candidate03_audio.py
Verify existing exports: python Scripts/create_candidate03_audio.py --check
No recordings, external samples, services or previous sound-bank inputs.
Layer parameters below are the editable source. Candidate02 remains untouched.
Objective metrics do not certify perceived timbre or mix quality.
"""
from pathlib import Path
import argparse
import array
import hashlib
import json
import math
import random
import sys
import wave

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource/Audio/Candidate03'
SR = 48000
TAU = 2 * math.pi

# Six independent voicings per family: attack tilt, pressure-band centre/decay,
# grit decay, small action-contact delay, early reflection times and tail tone.
# Changes are deliberately temporal/spectral as well as independently seeded.
PROFILES = {
    'Carbine': [
        (2050, 510, .020, .027, .042, [ .017, .036, .071 ], 3500),
        (2380, 565, .018, .025, .048, [ .020, .044, .078 ], 4100),
        (1870, 455, .023, .031, .046, [ .015, .040, .085 ], 3100),
        (2200, 610, .019, .024, .039, [ .022, .048, .073 ], 3850),
        (1950, 535, .022, .029, .051, [ .018, .033, .081 ], 3300),
        (2480, 485, .021, .026, .044, [ .024, .041, .090 ], 4300),
    ],
    'Shotgun': [
        ( 920, 245, .047, .063, .011, [ .032, .069, .123 ], 2150),
        (1150, 280, .044, .058, .014, [ .027, .075, .139 ], 2550),
        ( 780, 225, .053, .067, .009, [ .038, .082, .131 ], 1950),
        (1040, 315, .042, .057, .016, [ .030, .064, .148 ], 2700),
        ( 860, 265, .050, .064, .012, [ .035, .088, .155 ], 2300),
        (1240, 235, .046, .060, .018, [ .024, .072, .142 ], 2900),
    ],
}


def lowpass(values, cutoff):
    alpha = -math.expm1(-TAU * cutoff / SR)
    previous = 0.0
    out = []
    for v in values:
        previous += alpha * (v - previous)
        out.append(previous)
    return out


def band(values, low, high):
    upper = lowpass(values, high)
    lower = lowpass(upper, low)
    return [a-b for a, b in zip(upper, lower)]


def noise(length, seed):
    rng = random.Random(seed)
    return [rng.uniform(-1, 1) for _ in range(length)]


def burst(length, seed, low, high, attack, decay):
    values = band(noise(length, seed), low, high)
    return [v * (-math.expm1(-i / SR / attack)) * math.exp(-i / SR / decay)
            for i, v in enumerate(values)]


def add(dest, src, time=0, gain=1):
    offset = round(time * SR)
    for i in range(min(len(src), len(dest)-offset)):
        dest[offset+i] += src[i] * gain


def contact(seed, large=False):
    """Small, short inharmonic contact; kept below the discharge pressure."""
    n = round(.09 * SR)
    values = burst(n, seed, 600 if large else 1450, 8200, .00012, .0035)
    rng = random.Random(seed+33)
    for k, freq in enumerate((710, 1193, 2081, 3709)):
        f = freq * rng.uniform(.91, 1.08) * (.66 if large else 1)
        decay = (.011 if large else .008) / (1+k*.3)
        for i in range(n):
            t = i/SR
            values[i] += .07/(1+k) * math.sin(TAU*f*t) * math.exp(-t/decay)
    return values


def design(family, index):
    sg = family == 'Shotgun'
    seed = (93000 if sg else 83000) + index*137
    edge, body_hi, pressure_decay, grit_decay, action_at, reflections, tail_hi = PROFILES[family][index-1]
    duration = .66 if sg else .42
    n = round(duration*SR)
    layers = {key:[0.0]*n for key in ('attack','body','mechanical','tail')}

    # Rifle: brief narrow crack. Shotgun: an irregular broader turbulent front.
    crack = burst(n, seed, edge, 12800 if not sg else 10300, .000075,
                  (.0072 if sg else .0042) * (1+.035*(index-3)))
    add(layers['attack'], crack, gain=1.05 if sg else 1.12)
    micro = [(0.0014+.00012*index, .28), (.0041+.00021*index, .17)] if sg else [(.0007+.00008*index, .17)]
    for j, (time, gain) in enumerate(micro):
        add(layers['attack'], burst(n, seed+11+j, edge*.7, 7600, .00011, .0028), time, gain)
    # A sub-millisecond bipolar pressure front avoids a sustained oscillator.
    width = .00065 if sg else .00034
    for i in range(round(.008*SR)):
        t = i/SR
        q = t/width
        layers['attack'][i] += (.26 if sg else .19) * q*(2-q)*math.exp(-q)

    add(layers['body'], burst(n, seed+23, 65 if sg else 105, body_hi, .00045, pressure_decay), .0008, 3.7 if sg else 2.8)
    grit = burst(n, seed+41, 260 if sg else 440, 3900 if sg else 4800, .00015, grit_decay)
    # Nonperiodic amplitude pockets add a granular pressure texture.
    rng = random.Random(seed+50)
    pockets = [rng.uniform(.68,1.13) for _ in range(256)]
    grit = [v*pockets[min(255,int(i/SR/.0019))] for i,v in enumerate(grit)]
    add(layers['body'], grit, .002 if sg else .0012, 1.35 if sg else .73)

    # Rifle bolt/action contacts only. SG receiver response is near the blast;
    # pump, extraction and lock remain separate timed gameplay events.
    add(layers['mechanical'], contact(seed+71, sg), action_at, .28 if sg else .39)
    if not sg:
        add(layers['mechanical'], contact(seed+79), action_at+.017+.001*index, .15)

    dry = [a+b for a,b in zip(layers['attack'],layers['body'])]
    reflected = lowpass(dry, tail_hi)
    for time, gain in zip(reflections, (.15,.075,.037) if sg else (.105,.048,.022)):
        add(layers['tail'], reflected, time, gain)
    # Filtered, evolving diffuse residual rather than a long copy of one pop.
    scatter = burst(n, seed+103, 230 if sg else 550, tail_hi, .020,
                    .077 if sg else .037)
    add(layers['tail'], scatter, .021 if sg else .012, .19 if sg else .105)
    total = [sum(values[i] for values in layers.values()) for i in range(n)]
    total = band(total, 38, 15000)
    fade = round(.012*SR)
    for i in range(fade): total[-i-1] *= .5-.5*math.cos(math.pi*i/fade)
    # Preserve relative envelope shape; no per-layer peak normalization or
    # hard clipping. Leave at least 4.5 dB source headroom for the engine mix.
    target = 10**((-4.5 if sg else -5.0)/20)
    gain = target / max(abs(x) for x in total)
    total = [x*gain for x in total]
    return total, layers, {'seed':seed,'attack_highpass_hz':edge,
        'pressure_upper_hz':body_hi,'pressure_decay_seconds':pressure_decay,
        'grit_decay_seconds':grit_decay,'mechanical_seconds':action_at,
        'reflection_seconds':reflections,'tail_lowpass_hz':tail_hi,
        'microbursts':micro}


def db(value):
    return 20*math.log10(max(value,1e-12))


def rms(values):
    return math.sqrt(sum(x*x for x in values)/max(1,len(values)))


def read_pcm(path):
    with wave.open(str(path),'rb') as w:
        assert (w.getnchannels(),w.getsampwidth(),w.getframerate()) == (1,2,SR)
        samples = array.array('h',w.readframes(w.getnframes()))
    if sys.byteorder != 'little': samples.byteswap()
    return [v/32768 for v in samples]


def metrics(values):
    n=len(values)
    bands={f'{lo}-{hi}_Hz':db(rms(band(values,lo,hi))) for lo,hi in ((40,250),(250,1200),(1200,5000),(5000,15000))}
    return {'duration_seconds':n/SR,'sample_rate':SR,'channels':1,'pcm_bits':16,
        'peak_dbfs':db(max(abs(v) for v in values)),'rms_dbfs':db(rms(values)),
        'dc_offset':sum(values)/n,'full_scale_samples':sum(abs(v)>=32767/32768 for v in values),
        'nonzero_samples':sum(v!=0 for v in values),
        'window_rms_dbfs':{name:db(rms(values[round(a*SR):round(b*SR)])) for name,a,b in
            [('attack_0_12ms',0,.012),('body_12_45ms',.012,.045),('action_45_95ms',.045,.095),('tail_after_95ms',.095,n/SR)]},
        'filter_band_rms_dbfs':bands,
        'envelope_5ms_rms_dbfs':[round(db(rms(values[i:i+240])),4) for i in range(0,n,240)]}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    OUT.mkdir(parents=True,exist_ok=True)
    manifest={'candidate':'03','status':'Original synthesized candidate; perceptual audition unavailable, sound quality provisional.',
        'provenance':'All layers generated locally by this standard-library script. No sample inputs, recordings, third-party packs, external services or Project Zero content.',
        'runtime_contract':'Six variants per gun. Avoid immediate repeats. Review at pitch 1.0; source profiles already vary timing and spectrum. Retain separate C02 pump/reload/empty events.',
        'events':{}}
    for family in PROFILES:
        for i in range(1,7):
            name=f'S_C03_{family}Shot_{i:02d}'
            path=OUT/(name+'.wav')
            if not args.check:
                values,layers,profile=design(family,i)
                pcm=array.array('h',(round(v*32767) for v in values))
                if sys.byteorder!='little':pcm.byteswap()
                with wave.open(str(path),'wb') as w:
                    w.setnchannels(1);w.setsampwidth(2);w.setframerate(SR);w.writeframes(pcm.tobytes())
            else:
                _,layers,profile=design(family,i)
            values=read_pcm(path)
            m=metrics(values)
            assert m['full_scale_samples']==0 and m['peak_dbfs'] < -4.4
            assert m['nonzero_samples']>SR*.10 and abs(m['dc_offset'])<.0001
            assert m['window_rms_dbfs']['tail_after_95ms']>-80
            manifest['events'][name]={'source':path.relative_to(ROOT).as_posix(),
                'asset':'/Game/ONE/Audio/Weapons/Candidate03/'+name,
                'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
                'family':family,'variant':i,'profile':profile,
                'unnormalized_layer_energy_fraction':{k:sum(x*x for x in v)/sum(sum(x*x for x in a) for a in layers.values()) for k,v in layers.items()},
                **m}
    hashes=[e['sha256'] for e in manifest['events'].values()]
    assert len(set(hashes))==12,'Duplicate exports'
    if args.check:
        previous=json.loads((OUT/'manifest.json').read_text())
        # JSON round trip canonicalizes authored tuple pairs to array values.
        assert json.loads(json.dumps(manifest))==previous,'Generated profile or file metrics differ from the saved manifest'
        print('CANDIDATE03_AUDIO_SOURCE_CHECK_PASS 12 PCM sources; no perceptual approval implied')
    else:
        (OUT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
        print('CANDIDATE03_AUDIO_SOURCE_GENERATED 12 original PCM sources')


if __name__=='__main__':main()
