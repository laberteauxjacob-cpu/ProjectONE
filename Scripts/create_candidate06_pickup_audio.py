"""Exactly three intentional stylized pickup cues; existing sound banks unchanged.
Deterministic original synthesis, not recordings. --check verifies without writes.
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
import create_candidate03_audio as core
import create_candidate05_audio as shared

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource/Audio/Candidate06'
SR = 48000
TAU = 2 * math.pi


def tone(values, start, length, frequency, gain, decay=.15, end_frequency=None):
    offset = round(start * SR)
    phase = 0.
    for index in range(min(round(length * SR), len(values) - offset)):
        t = index / SR
        pitch = frequency if end_frequency is None else end_frequency + (frequency - end_frequency) * math.exp(-t / .11)
        phase += TAU * pitch / SR
        envelope = (-math.expm1(-t / .003)) * math.exp(-t / decay)
        values[offset + index] += gain * envelope * (math.sin(phase) + .16 * math.sin(phase * 2.001))


def design(kind):
    duration = {'InstaKill': .86, 'DoublePoints': .74, 'MaxAmmo': .88}[kind]
    seed = {'InstaKill': 606101, 'DoublePoints': 606202, 'MaxAmmo': 606303}[kind]
    values = [0.] * round(duration * SR)
    if kind == 'InstaKill':
        tone(values, 0, .76, 430, .8, .22, 105)
        tone(values, .075, .66, 645, .35, .18, 157.5)
        core.add(values, core.burst(len(values), seed, 900, 4800, .008, .085), .02, .13)
        motif = 'two descending low electronic skull pulses and a short filtered edge'
    elif kind == 'DoublePoints':
        for start, pitch, gain in ((0, 659.25, .7), (.09, 987.77, .58), (.19, 1318.51, .7), (.28, 1975.53, .4)):
            tone(values, start, .45, pitch, gain, .15)
        motif = 'two ascending paired notes; gold doubling motif'
    else:
        for start, pitch in ((0, 240), (.105, 360), (.21, 540)):
            tone(values, start, .52, pitch, .7, .12)
            core.add(values, core.burst(round(.10 * SR), seed + round(start * 1000), 1300, 6800, .001, .017), start, .065)
        tone(values, .30, .55, 1080, .25, .20)
        motif = 'three stepped electronic latches and a refill confirmation note'
    return shared.finish(values, -9.), {'effect': kind, 'seed': seed, 'motif': motif,
        'loop': False, 'intentional_stylized_synthesis': True, 'recorded_source': False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if not args.check:
        OUT.mkdir(parents=True, exist_ok=True)
    manifest = {'candidate': '06', 'scope': 'Exactly three stylized collection cues; existing weapon/zombie audio unchanged.',
        'provenance': 'Original Project ONE mathematical design; no external samples, recordings or Project Zero input.',
        'source_generators': ['Scripts/create_candidate06_pickup_audio.py', 'Scripts/create_candidate05_audio.py', 'Scripts/create_candidate03_audio.py'],
        'auditory_approval': False, 'mix': {'collection_gain': .7, 'shared_group': 'Action (existing maximum seven voices)'}, 'events': {}}
    for kind in ('InstaKill', 'DoublePoints', 'MaxAmmo'):
        name = 'S_Pickup06_' + kind
        values, description = design(kind)
        pcm = array.array('h', (round(sample * 32767) for sample in values))
        if sys.byteorder != 'little':
            pcm.byteswap()
        expected = pcm.tobytes()
        path = OUT / (name + '.wav')
        if not args.check:
            with wave.open(str(path), 'wb') as output:
                output.setnchannels(1); output.setsampwidth(2); output.setframerate(SR); output.writeframes(expected)
        with wave.open(str(path), 'rb') as actual:
            assert (actual.getnchannels(), actual.getsampwidth(), actual.getframerate()) == (1, 2, SR)
            assert actual.readframes(actual.getnframes()) == expected
        metrics = core.metrics(core.read_pcm(path))
        assert metrics['full_scale_samples'] == 0 and metrics['nonzero_samples'] > 100
        assert -9.01 <= metrics['peak_dbfs'] <= -8.99
        manifest['events'][name] = {'source': path.relative_to(ROOT).as_posix(),
            'asset': '/Game/ONE/Audio/Candidate06/' + name, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest(), **description, **metrics}
    assert len(manifest['events']) == 3 and len({event['sha256'] for event in manifest['events'].values()}) == 3
    target = OUT / 'manifest.json'
    if args.check:
        assert json.loads(json.dumps(manifest)) == json.loads(target.read_text(encoding='utf-8'))
    else:
        target.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    print('ONE06_PICKUP_AUDIO_SOURCE_CHECK_PASS 3 original PCM cues' if args.check else 'ONE06_PICKUP_AUDIO_SOURCE_GENERATED 3 original PCM cues')


if __name__ == '__main__':
    main()
