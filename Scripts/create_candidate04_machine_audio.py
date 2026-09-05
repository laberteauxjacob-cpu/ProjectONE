"""Original Candidate04 mechanical/electrical machine sound design.

Python 3 standard library only. No samples, downloads or external inputs.
WAVs are mono 48kHz PCM16 with fmt/data chunks only. Periodic loop sources
have continuous phase; runtime components own their fades and lifetime.
Measurements verify payloads, headroom and endpoints, not perceptual approval.
"""
from pathlib import Path
import argparse
import array
import hashlib
import json
import math
import random
import wave

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/Machines/Candidate04/Audio'
SR=48000
TAU=math.tau

def noise(n,seed):
    rng=random.Random(seed);return [rng.uniform(-1,1) for _ in range(n)]
def lowpass(a,cutoff):
    k=-math.expm1(-TAU*cutoff/SR);v=0.;out=[]
    for x in a:v+=k*(x-v);out.append(v)
    return out
def band(a,lo,hi):
    hi_values=lowpass(a,hi);lo_values=lowpass(hi_values,lo)
    return [x-y for x,y in zip(hi_values,lo_values)]
def add(dst,src,at=0,gain=1):
    offset=round(at*SR)
    for i,x in enumerate(src[:max(0,len(dst)-offset)]):dst[offset+i]+=x*gain
def contact(seed,weight=1):
    n=round(.23*SR);a=band(noise(n,seed),340/weight,8700);rng=random.Random(seed+4)
    modes=[(f/weight*rng.uniform(.97,1.03),g/(j+1)) for j,(f,g) in enumerate(((431,.7),(793,.6),(1331,.4),(2593,.25)))]
    for i in range(n):
        t=i/SR;attack=1-math.exp(-t/.0004)
        a[i]=a[i]*attack*math.exp(-t/.009)*1.2+sum(g*math.sin(TAU*f*t)*math.exp(-t/(.034*weight)) for f,g in modes)*attack
    return a
def servo(seconds,seed,f0=180,f1=120):
    n=round(seconds*SR);a=band(noise(n,seed),500,4400);phase=0
    for i in range(n):
        t=i/SR;u=t/seconds;env=min(1,t/.07,(seconds-t)/.08)
        phase+=TAU*(f0+(f1-f0)*u)/SR
        a[i]=max(0,env)*(.09*a[i]+.10*math.sin(phase)+.035*math.sin(phase*3))*(.90+.10*math.sin(TAU*17*t))
    return a
def chime(seconds,notes,seed):
    n=round(seconds*SR);a=[0.]*n
    for j,(frequency,at) in enumerate(notes):
        m=n-round(at*SR);waveform=[]
        for i in range(m):
            t=i/SR;envelope=(1-math.exp(-t/.001))*math.exp(-t/(.19+.05*j))
            waveform.append(envelope*(.32*math.sin(TAU*frequency*t)+.08*math.sin(TAU*frequency*2.71*t)))
        add(a,waveform,at)
    add(a,contact(seed,.8),0,.09)
    return a
def loop(seconds,box=False,processing=False):
    n=round(seconds*SR);a=[];rng=random.Random(404 if box else 408)
    # Integer cycle counts make every sinusoid and modulation periodic.
    freqs=(52,79,131,208) if box else (60,120,181,359)
    freqs=[round(f*seconds)/seconds for f in freqs]
    grains=[(rng.randint(900,2100)/seconds,rng.random()*TAU) for _ in range(14)]
    for i in range(n):
        t=i/SR;slow=.82+.18*math.sin(TAU*t/seconds)
        base=sum((.27/(j+1))*math.sin(TAU*f*t+.25*j)*slow for j,f in enumerate(freqs))
        air=sum(math.sin(TAU*f*t+p) for f,p in grains)/len(grains)*.055
        if processing:
            pulse=(.5+.5*math.sin(TAU*4*t/seconds))**4
            base+=.11*pulse*math.sin(TAU*360*t)+.06*math.sin(TAU*724*t)*(.5+.5*math.sin(TAU*t/seconds))
        a.append(base+air)
    return a

def design():
    cues={}
    def put(name,a,peak,description,looping=False):cues['S_C04_'+name]=(a,peak,description,looping)
    put('BoxIdle',loop(3,box=True),-29,'Quiet periodic containment-field hum, distinct from the processor motor.',True)
    a=[0.]*round(.94*SR)
    add(a,contact(4401,1.5),.02,.65);add(a,contact(4402,.85),.14,.34);add(a,servo(.64,4413,103,181),.23,1.3)
    put('BoxActivate',a,-12,'Two lock releases followed by hydraulic lid movement.')
    for j in range(3):
        a=chime(.17,[(360+j*77,0),(720+j*49,.025)],4420+j)
        put('BoxCycle%02d'%(j+1),a,-16,'Short distinct index cue; called only when the actual preview model changes.')
    a=chime(1.36,[(262,0),(393,.11),(659,.24),(1047,.39)],4440)
    add(a,contact(4441,2.2),0,.28)
    put('BoxReveal',a,-10,'Open ascending experimental reveal with a weighted latch impulse.')
    put('BoxCollect',chime(.49,[(790,0),(1175,.075)],4445),-17,'Short containment release confirmation.')
    a=servo(.70,4450,161,78);add(a,contact(4451,1.55),.55,.62)
    put('BoxClose',a,-13,'Descending hydraulic movement and heavy lid latching.')
    put('UpgradeIdle',loop(2),-31,'Low transformer and bearing presence with integer-period harmonic motion.',True)
    a=servo(1.10,4460,132,233);add(a,contact(4461,.9),.78,.35)
    put('UpgradeIntake',a,-15,'Linear guide motor and cradle arrival contact.')
    a=[0.]*round(.44*SR);add(a,contact(4463,.9),.01,.7);add(a,contact(4464,.65),.12,.48)
    put('UpgradeActivate',a,-13,'Paired pneumatic clamp closure.')
    put('UpgradeProcess',loop(3,processing=True),-18,'Periodic loaded electrical motor; runtime ramps gain during processing.',True)
    a=servo(1.24,4470,219,106);add(a,contact(4471,.95),1.03,.46)
    put('UpgradeOutput',a,-14,'Reverse feed with returning tray bearing contact.')
    a=chime(1.48,[(880,0),(660,.13),(440,.31)],4475);add(a,contact(4476,1.4),0,.27)
    put('UpgradeComplete',a,-10,'Descending three-part resonant completion, distinct from box reveal.')
    a=chime(.52,[(523,0),(784,.06)],4480);add(a,contact(4481,.6),.02,.15)
    put('UpgradeCollect',a,-17,'Mechanical release and short two-tone collection confirmation.')
    a=servo(.76,4485,112,62);add(a,contact(4486,.9),.59,.25)
    put('UpgradeClose',a,-19,'Processor unload and return to idle.')
    return cues

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--check',action='store_true');args=parser.parse_args()
    OUT.mkdir(parents=True,exist_ok=True);events={}
    for name,(values,peak,description,looping) in design().items():
        amplitude=10**(peak/20);gain=amplitude/max(abs(x) for x in values)
        pcm=array.array('h',(round(max(-1,min(1,x*gain))*32767) for x in values))
        path=OUT/(name+'.wav')
        if not args.check:
            with wave.open(str(path),'wb') as w:w.setparams((1,2,SR,0,'NONE','not compressed'));w.writeframes(pcm.tobytes())
        with wave.open(str(path),'rb') as w:
            assert w.getparams()[:3]==(1,2,SR)
            actual=w.readframes(w.getnframes());assert actual==pcm.tobytes(),'Authored PCM changed: '+name
        measured=max(abs(x) for x in pcm)/32768
        events[name]={'source':path.relative_to(ROOT).as_posix(),'asset':'/Game/ONE/Audio/Machines/Candidate04/'+name,
          'duration_seconds':len(pcm)/SR,'looping':looping,'description':description,'peak_dbfs':20*math.log10(measured),
          'rms_dbfs':20*math.log10(math.sqrt(sum(x*x for x in pcm)/len(pcm))/32768),
          'clipped_samples':sum(abs(x)>=32767 for x in pcm),'loop_endpoint_delta_normalized':abs(pcm[-1]-pcm[0])/32768 if looping else None,
          'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'bytes':path.stat().st_size}
    manifest={'candidate':'04','authoring_script':'Scripts/create_candidate04_machine_audio.py','source_kind':'Original deterministic synthesis, no external samples or assets.',
      'format':'48kHz mono PCM16 fmt/data-only WAV','perceptual_audition':'Unavailable; waveform and runtime audibility checks do not approve timbre or mix.',
      'events':events,'loop_ownership':'ONE04MachinePresentation starts/transitions/fades/stops idle and process components; destroys them with machine. Cycle cues occur only on preview changes.'}
    output=OUT/'manifest.json'
    if not args.check:output.write_text(json.dumps(manifest,indent=2)+'\n')
    else:assert json.loads(output.read_text())==manifest
    print('C04 MACHINE AUDIO '+('CHECK' if args.check else 'GENERATION')+' PASS:',len(events),'original WAVs')
if __name__=='__main__':main()
