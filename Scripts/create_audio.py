"""Original experimental sound design, not recorded weapons or licensed library content.

Broadband impulse + filtered noise + short reflected tails for a contained carbine
shot; a sequence of granular metal impacts for reload. No tonal beep substitutes.
Quality must be judged in game; these remain candidate audio.
Run with Blender --background --python Scripts/create_audio.py or Python 3.
"""
from pathlib import Path
import math, random, struct, wave
ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'ArtSource' / 'Audio'
OUT.mkdir(parents=True, exist_ok=True)
SR = 44100

def save(name, samples):
    peak = max(max(abs(x) for x in samples), .01)
    data = b''.join(struct.pack('<h', int(max(-1,min(1,x/peak*.82))*32767)) for x in samples)
    with wave.open(str(OUT/(name+'.wav')), 'wb') as f:
        f.setnchannels(1); f.setsampwidth(2); f.setframerate(SR); f.writeframes(data)

def noise_burst(length, seed, decay, low=.12):
    rng=random.Random(seed); lp=0.; result=[]
    for i in range(int(length*SR)):
        t=i/SR; n=rng.uniform(-1,1); lp += low*(n-lp)
        env=(1-math.exp(-t*4200))*math.exp(-t/decay)
        result.append((n*.33+lp*2.2)*env)
    return result

shot=noise_burst(.62,701,.025,.07)
# A brief bass pressure wave is blended underneath the broadband muzzle crack.
for i in range(len(shot)):
    t=i/SR
    shot[i] += .7*math.sin(2*math.pi*(82*t-25*t*t))*math.exp(-t/.026)
base=shot[:]
for delay, gain in [(.031,.25),(.067,.14),(.119,.08),(.173,.04)]:
    d=int(delay*SR)
    for i in range(d,len(shot)): shot[i]+=base[i-d]*gain
save('S_CarbineShot',shot)

def sequence(name,duration,events):
    samples=[0.] * int(duration*SR)
    for j,(start,gain,decay) in enumerate(events):
        burst=noise_burst(.16,1200+j,decay,.22)
        offset=int(start*SR)
        for i,v in enumerate(burst):
            if offset+i<len(samples): samples[offset+i]+=v*gain
    save(name,samples)

sequence('S_Reload',1.8,[(.04,.5,.016),(.24,.23,.009),(.68,.38,.025),(1.13,.65,.019),(1.44,.28,.009)])
sequence('S_Empty',.16,[(.002,.5,.007),(.027,.13,.005)])
sequence('S_Impact',.24,[(.002,.6,.022),(.025,.15,.012)])
print('Created original experimental sound design:', OUT)
