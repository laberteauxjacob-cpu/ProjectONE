"""Original Candidate05 facility, infected and stronger experimental firearm audio.

Python standard library. Reuses only Project ONE's editable synthesis primitives
and original physical firearm design, never third-party samples or whole-bank
pitch shifting. --check regenerates expected PCM without changing source files.
Objective checks do not establish perceptual listening or mix approval.
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

sys.dont_write_bytecode=True
import create_candidate03_audio as core
import create_candidate04_audio as prior

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/Audio/Candidate05'
SR=48000
TAU=math.tau


def finish(values, peak_db, loop=False):
    values=core.band(values,35,14500)
    dc=sum(values)/len(values); values=[v-dc for v in values]
    fade=round((.035 if loop else .012)*SR)
    for i in range(fade):
        gain=.5-.5*math.cos(math.pi*i/fade)
        values[i]*=gain; values[-1-i]*=gain
    peak=max(abs(v) for v in values)
    gain=10**(peak_db/20)/max(peak,1e-9)
    return [v*gain for v in values]


def formant(values, frequency, bandwidth):
    """Damped vocal/pipe resonator with deterministic, stable recurrence."""
    radius=math.exp(-math.pi*bandwidth/SR)
    feedback=2*radius*math.cos(TAU*frequency/SR); radius2=radius*radius
    previous=older=0.; out=[]
    for sample in values:
        value=(1-radius)*sample+feedback*previous-radius2*older
        out.append(value); older,previous=previous,value
    return out


def vocal(kind,index):
    seed=505000+{'Breath':100,'Pursuit':200,'Attack':300,'Hit':400,'Death':500}[kind]+index*113
    rng=random.Random(seed)
    seconds={'Breath':1.45,'Pursuit':.92,'Attack':.64,'Hit':.30,'Death':1.45}[kind]*(.91+.045*index)
    n=round(seconds*SR); noise=core.band(core.noise(n,seed),120,6600)
    base={'Breath':77,'Pursuit':89,'Attack':104,'Hit':118,'Death':82}[kind]+(index-2.5)*3.7
    phase=0.; excitation=[]
    for i in range(n):
        t=i/SR; q=t/seconds
        contour=(1+.07*math.sin(TAU*3.2*t))*(1-.42*q if kind=='Death' else 1+.12*math.sin(math.pi*q))
        phase=(phase+base*contour/SR)%1
        pulse=(math.exp(-phase*16)-.063)*2
        # Inhale/exhale has turbulent, irregular glottal effort rather than a
        # single clean oscillator. Attacks have a short audible onset/rasp.
        voiced={'Breath':.24,'Pursuit':.48,'Attack':.82,'Hit':.65,'Death':.70}[kind]
        excitation.append(voiced*pulse+(1-voiced*.6)*noise[i]*(.7+.3*math.sin(TAU*17*t)**2))
    low=formant(excitation,{'Breath':350,'Pursuit':430,'Attack':610,'Hit':740,'Death':380}[kind]+index*23,110)
    mid=formant(excitation,1250+index*67,210)
    upper=formant(excitation,2450+index*80,370)
    values=[]
    for i,(a,b,c) in enumerate(zip(low,mid,upper)):
        t=i/SR; q=t/seconds
        attack=.065 if kind=='Breath' else .015
        env=(-math.expm1(-t/attack))*max(0.,1-q)**(.45 if kind=='Breath' else .8)
        if kind=='Breath': env*=.4+.6*math.sin(math.pi*q)**2
        elif kind=='Pursuit': env*=.5+.5*math.sin(math.pi*min(1.,q*1.5))**2
        values.append((a+.60*b+.22*c)*env)
    level={'Breath':-17,'Pursuit':-15,'Attack':-10.5,'Hit':-13,'Death':-12.5}[kind]
    return finish(values,level),{'kind':'infected','event':kind,'variant':index,'seed':seed,
        'voiced_fundamental_hz':base,'formant_model':'Original noisy glottal excitation through three damped resonators',
        'attack_family':(index-1)//2 if kind=='Attack' else None,'loop':False}


def ambience_loop(motor):
    seconds=11 if motor else 9; n=seconds*SR; seed=51502 if motor else 51501
    air=core.band(core.noise(n,seed),40 if motor else 65,1050 if motor else 1850)
    values=[]
    for i,sample in enumerate(air):
        t=i/SR
        if motor:
            value=.30*sample+.12*math.sin(TAU*53*t)+.025*math.sin(TAU*107*t)
            value*=.75+.10*math.sin(TAU*t/11)+.06*math.sin(TAU*t/2.2)
        else:
            value=sample*(.72+.06*math.sin(TAU*t/9)+.04*math.sin(TAU*t/3))
            value+=.012*math.sin(TAU*93*t)
        values.append(value)
    return finish(values,-23 if motor else -21,True),{'kind':'ambience','event':'motor' if motor else 'ventilation',
        'seed':seed,'loop':True,'seam':'35ms cosine endpoint taper, exact zero endpoints; unchanged spatial source persists'}


def ambience_event(kind,index):
    seed=52500+index*173+(1000 if kind=='Pipe' else 0); n=round((.7 if kind=='Drip' else 1.8)*SR)
    values=[0.]*n
    if kind=='Drip':
        for pulse,offset in enumerate((0.,.064+index*.018)):
            amount=.6 if pulse else 1.
            tone=1600+index*370
            wavelet=[]; phase=0.
            for i in range(round(.22*SR)):
                t=i/SR; phase+=TAU*(tone+(900+index*80)*math.exp(-t/.012))/SR
                wavelet.append(math.sin(phase)*(-math.expm1(-t/.0008))*math.exp(-t/.031)*amount)
            core.add(values,wavelet,offset,.7)
        core.add(values,core.burst(n,seed,1300,12000,.0001,.004),.003,.3)
    else:
        impacts=core.burst(n,seed,200,5500,.0005,.026)
        values=[v*.18 for v in impacts]
        for tone,gain in ((230+index*47,.7),(517+index*71,.3),(1131+index*39,.1)):
            for i in range(n):
                t=i/SR; values[i]+=gain*math.sin(TAU*t*tone)*(1-math.exp(-t/.002))*math.exp(-t/(.14+index*.035))
        core.add(values,core.burst(n,seed+1,900,5000,.06,.23),.035,.12)
    return finish(values,-17 if kind=='Drip' else -20),{'kind':'ambience','event':kind,'variant':index,'seed':seed,'loop':False}


def upgrade(family,index):
    if family=='LastWord': base,physical=prior.pistol(index)
    else: base,_,physical=core.design('Carbine' if family=='Overcurrent' else 'Shotgun',index)
    seconds={'LastWord':.46,'Overcurrent':.32,'Gravebreaker':.68}[family]
    n=max(len(base),round(seconds*SR)); out=list(base)+[0.]*(n-len(base))
    seed=53500+{'LastWord':0,'Overcurrent':1000,'Gravebreaker':2000}[family]+index*97
    noise=core.band(core.noise(n,seed),1800,10500)
    start={'LastWord':5700,'Overcurrent':7200,'Gravebreaker':4400}[family]+index*91
    end={'LastWord':2100,'Overcurrent':3600,'Gravebreaker':1300}[family]+index*41
    decay={'LastWord':.060,'Overcurrent':.031,'Gravebreaker':.10}[family]
    energy=[]; phase=0.
    for i in range(n):
        t=i/SR; phase+=TAU*(end+(start-end)*math.exp(-t/.018))/SR
        envelope=(-math.expm1(-t/.00045))*math.exp(-t/decay)
        grain=(.64+.36*math.sin(TAU*(68 if family=='Overcurrent' else 31)*t)**2)
        tone=(math.sin(phase)+.32*math.sin(phase*1.414)+.14*math.sin(phase*2.073))
        energy.append((.42*tone*grain+.45*noise[i])*envelope)
    # Deliberately audible spectral layer over an unchanged firearm excitation.
    # Overcurrent's energized tail is short enough for discrete 11.5Hz reports.
    core.add(out,energy,.00035,.95 if family!='Gravebreaker' else 1.3)
    if family=='LastWord':
        core.add(out,core.band(energy,2400,6200),.073+index*.001,.28)
        core.add(out,core.band(energy,3000,6800),.122+index*.001,.10)
    elif family=='Gravebreaker':
        core.add(out,core.burst(n,seed+12,1200,7800,.006,.092),.042,.32)
        core.add(out,energy,.096+index*.002,.18)
    return finish(out,-5),{'kind':'shot','family':family,'variant':index,'physical_design':physical,
        'energy_seed':seed,'energy_start_hz':start,'energy_end_hz':end,'energy_decay_seconds':decay,
        'physical_pitch_ratio':1.0,'independent_energy_layer':True,'loop':False}


def designs():
    for motor in (False,True):
        values,info=ambience_loop(motor); yield 'S_AmbientMotorLoop' if motor else 'S_AmbientVentLoop',values,info
    for kind in ('Drip','Pipe'):
        for index in range(1,4):
            values,info=ambience_event(kind,index); yield f'S_Ambient{kind}_{index:02d}',values,info
    for kind in ('Breath','Pursuit','Attack','Hit','Death'):
        for index in range(1,7 if kind=='Attack' else 5):
            values,info=vocal(kind,index); yield f'S_Zombie{kind}_{index:02d}',values,info
    for family in ('LastWord','Overcurrent','Gravebreaker'):
        for index in range(1,7):
            values,info=upgrade(family,index); yield f'S_{family}Shot_{index:02d}',values,info


def main():
    parser=argparse.ArgumentParser(description=__doc__); parser.add_argument('--check',action='store_true'); args=parser.parse_args()
    if not args.check: OUT.mkdir(parents=True,exist_ok=True)
    manifest={'candidate':'05','provenance':'Original Project ONE procedural sound design; no third-party recording, sample pack or Project Zero input.',
        'source_generators':['Scripts/create_candidate05_audio.py','Scripts/create_candidate04_audio.py','Scripts/create_candidate03_audio.py'],
        'status':'Authored sound sources; waveform/format checks do not establish perceptual listening or in-engine mix approval.',
        'mix_defaults':{'one.Audio.Weapons':1.,'one.Audio.Zombies':.8,'one.Audio.Ambience':.55},'events':{}}
    for name,values,info in designs():
        pcm=array.array('h',(round(v*32767) for v in values))
        if sys.byteorder!='little': pcm.byteswap()
        expected=pcm.tobytes(); path=OUT/(name+'.wav')
        if not args.check:
            with wave.open(str(path),'wb') as wav:
                wav.setnchannels(1); wav.setsampwidth(2); wav.setframerate(SR); wav.writeframes(expected)
        with wave.open(str(path),'rb') as wav:
            assert (wav.getnchannels(),wav.getsampwidth(),wav.getframerate())==(1,2,SR)
            assert wav.readframes(wav.getnframes())==expected
        metrics=core.metrics(core.read_pcm(path)); assert metrics['full_scale_samples']==0 and metrics['nonzero_samples']>100
        assert metrics['peak_dbfs']<-4.99
        manifest['events'][name]={'source':path.relative_to(ROOT).as_posix(),'asset':'/Game/ONE/Audio/Candidate05/'+name,
            'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),**info,**metrics}
    assert len(manifest['events'])==48 and len({e['sha256'] for e in manifest['events'].values()})==48
    target=OUT/'manifest.json'
    if args.check: assert json.loads(json.dumps(manifest))==json.loads(target.read_text())
    else: target.write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print('CANDIDATE05_AUDIO_SOURCE_CHECK_PASS 48 original PCM sources' if args.check else 'CANDIDATE05_AUDIO_SOURCE_GENERATED 48 original PCM sources')


if __name__=='__main__': main()
