"""Process verified recorded sources. Requires numpy and PyAV; never downloads.

Run with --decoder-library PATH only when PyAV is installed in a separate tools
directory. --check recomputes the PCM and compares existing files without writes.
Only the intentionally stylized upgrade energy layer uses original synthesis.
"""
from pathlib import Path
import argparse, hashlib, io, json, math, sys, wave

sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
BASE=ROOT/'ArtSource/Audio/Candidate07'
SR=48000

def digest(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def below(base,relative):
    path=(base/relative).resolve()
    if not path.is_relative_to(base.resolve()): raise ValueError('Escaping source/output path')
    return path

def energy_layer(family,index,length):
    """C05 upgrade energy formula, isolated from its old synthetic gun report."""
    import create_candidate03_audio as core
    seconds={'LastWord':.46,'Overcurrent':.32,'Gravebreaker':.68}[family]
    n=max(length,round(seconds*SR)); out=[0.]*n
    seed=53500+{'LastWord':0,'Overcurrent':1000,'Gravebreaker':2000}[family]+index*97
    noise=core.band(core.noise(n,seed),1800,10500)
    start={'LastWord':5700,'Overcurrent':7200,'Gravebreaker':4400}[family]+index*91
    end={'LastWord':2100,'Overcurrent':3600,'Gravebreaker':1300}[family]+index*41
    decay={'LastWord':.060,'Overcurrent':.031,'Gravebreaker':.10}[family]
    energy=[]; phase=0.
    for i in range(n):
        t=i/SR; phase+=math.tau*(end+(start-end)*math.exp(-t/.018))/SR
        envelope=(-math.expm1(-t/.00045))*math.exp(-t/decay)
        grain=.64+.36*math.sin(math.tau*(68 if family=='Overcurrent' else 31)*t)**2
        tone=math.sin(phase)+.32*math.sin(phase*1.414)+.14*math.sin(phase*2.073)
        energy.append((.42*tone*grain+.45*noise[i])*envelope)
    core.add(out,energy,.00035,.95 if family!='Gravebreaker' else 1.3)
    if family=='LastWord':
        core.add(out,core.band(energy,2400,6200),.073+index*.001,.28)
        core.add(out,core.band(energy,3000,6800),.122+index*.001,.10)
    elif family=='Gravebreaker':
        core.add(out,core.burst(n,seed+1,1200,7800,.006,.092),.042,.32)
        core.add(out,energy,.096+index*.002,.18)
    return out

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check',action='store_true')
    parser.add_argument('--decoder-library',type=Path)
    args=parser.parse_args()
    if args.decoder_library: sys.path.insert(0,str(args.decoder_library.resolve()))
    import av
    import numpy as np
    inventory=json.loads((BASE/'source_inventory.json').read_text(encoding='utf-8'))
    recipes=json.loads((BASE/'recipes.json').read_text(encoding='utf-8'))
    cache={}; facts={}
    for key,item in inventory['sources'].items():
        path=below(BASE,item['file'])
        if digest(path)!=item['sha256'] or path.stat().st_size!=item['bytes']: raise ValueError('Source identity mismatch: '+key)
    def decode(key):
        if key in cache: return cache[key]
        item=inventory['sources'][key]; path=below(BASE,item['file']); arrays=[]
        with av.open(str(path)) as container:
            if len(container.streams.audio)!=1: raise ValueError('Expected one source audio stream: '+key)
            stream=container.streams.audio[0]
            facts[key]={'source_sample_rate':stream.codec_context.sample_rate,'source_channels':stream.codec_context.channels,
                'source_codec':stream.codec_context.name,'decoded_rate':SR,'channel_mix':'arithmetic mean; no equal-power downmix gain'}
            converter=av.AudioResampler(format='fltp',rate=SR)
            for frame in container.decode(stream):
                arrays.extend(x.to_ndarray().astype(np.float64).mean(axis=0) for x in converter.resample(frame))
            arrays.extend(x.to_ndarray().astype(np.float64).mean(axis=0) for x in converter.resample(None))
        if not arrays: raise ValueError('Empty source: '+key)
        result=np.concatenate(arrays)
        if not np.isfinite(result).all() or len(result)<32: raise ValueError('Invalid PCM: '+key)
        facts[key]['decoded_duration_seconds']=len(result)/SR
        cache[key]=result; return result
    def layer_pcm(layer):
        original=decode(layer['source'])
        start=round(layer['start_seconds']*SR)
        end=len(original) if layer['end_seconds'] is None else round(layer['end_seconds']*SR)
        if not 0<=start<end<=len(original): raise ValueError('Trim outside source: '+str(layer))
        values=original[start:end].copy()
        rate=layer['playback_rate_edit']
        if rate!=1.:
            values=np.interp(np.arange(max(1,round(len(values)/rate)))*rate,np.arange(len(values)),values)
        values-=values.mean()
        # Smooth roll-offs; this filters recorded samples, it creates no excitation.
        freq=np.fft.rfftfreq(len(values),1/SR)
        low=layer['highpass_hz']; high=layer['lowpass_hz']
        response=(1-np.exp(-np.square(freq/max(1,low))))/np.sqrt(1+(freq/high)**8)
        values=np.fft.irfft(np.fft.rfft(values)*response,n=len(values))
        peak=float(np.max(np.abs(values)))
        if peak<1e-7: raise ValueError('Silent selected source region: '+str(layer))
        values/=peak
        # Event cuts preserve two milliseconds before the measured main onset.
        active=np.flatnonzero(np.abs(values)>.06)
        lead=max(0,int(active[0])-round(.002*SR))
        values=values[lead:]
        fade=min(round(.0005*SR),len(values)//4)
        values[:fade]*=np.linspace(0,1,fade)
        tail=min(round(.012*SR),len(values)//4)
        values[-tail:]*=np.linspace(1,0,tail)
        return values*layer['gain'],lead/SR
    events={}
    for name,recipe in recipes['cues'].items():
        layers=[]; trims=[]
        for layer in recipe['layers']:
            data,lead=layer_pcm(layer); offset=round(layer['offset_seconds']*SR)
            layers.append((offset,data)); trims.append(lead)
        size=max(offset+len(data) for offset,data in layers)
        values=np.zeros(size,dtype=np.float64)
        for offset,data in layers: values[offset:offset+len(data)]+=data
        if recipe['energy']:
            energy=np.asarray(energy_layer(recipe['energy']['family'],recipe['energy']['variant'],len(values)))
            if len(energy)>len(values): values=np.pad(values,(0,len(energy)-len(values)))
            # Recorded report remains the primary full-scale layer; energy is a restrained addition.
            values[:len(energy)]+=.38*energy
        if recipe['loop']:
            cross=min(round(.08*SR),len(values)//4)
            blend=np.linspace(0,1,cross,endpoint=False)
            seam=values[-cross:]*(1-blend)+values[:cross]*blend
            values=np.concatenate([seam,values[cross:-cross]])
        values-=values.mean()
        peak=float(np.max(np.abs(values)))
        values*=10**(recipe['target_peak_dbfs']/20)/peak
        pcm=np.rint(values*32767).astype('<i2')
        if np.any(np.abs(pcm.astype(np.int32))>=32767): raise ValueError('Full-scale sample: '+name)
        buffer=io.BytesIO()
        with wave.open(buffer,'wb') as output:
            output.setnchannels(1); output.setsampwidth(2); output.setframerate(SR); output.writeframes(pcm.tobytes())
        blob=buffer.getvalue(); target=below(BASE,'Processed/'+name+'.wav')
        if args.check:
            if not target.exists() or target.read_bytes()!=blob: raise ValueError('PCM differs: '+name)
        else:
            target.parent.mkdir(parents=True,exist_ok=True); target.write_bytes(blob)
        events[name]={'source':target.relative_to(ROOT).as_posix(),'asset':'/Game/ONE/Audio/Candidate07/'+name,
            'sha256':hashlib.sha256(blob).hexdigest(),'bytes':len(blob),'frames':len(pcm),'duration_seconds':len(pcm)/SR,
            'sample_rate':SR,'channels':1,'pcm_bits':16,'peak_dbfs':20*math.log10(max(float(np.max(np.abs(pcm.astype(np.float64))))/32768,1e-12)),
            'rms_dbfs':20*math.log10(max(float(np.sqrt(np.mean((pcm.astype(np.float64)/32768)**2))),1e-12)),
            'full_scale_samples':0,'loop':recipe['loop'],'recorded_source_layers':[
                {**layer,'source_sha256':inventory['sources'][layer['source']]['sha256'],'measured_leading_trim_seconds':lead}
                for layer,lead in zip(recipe['layers'],trims)],
            'intentional_synthetic_energy':recipe['energy'],'note':recipe['note'],'runtime_pitch':1.}
    manifest={'schema':1,'candidate':'07','status':'SOURCE_PCM_VERIFIED','events':events,'source_decode_facts':facts,
        'toolchain':{'python':sys.version.split()[0],'numpy':np.__version__,'pyav':av.__version__},
        'processor_sha256':digest(Path(__file__)),'recipes_sha256':digest(BASE/'recipes.json'),
        'source_inventory_sha256':digest(BASE/'source_inventory.json'),
        'perceptual_audition':False,'limitations':['No audition, in-engine mix or perceptual realism approval.','Some assets are explicitly documented firearm/object analogues.','MP3 upstream representations and lossy OGG sources are disclosed; PCM processing does not restore lost information.']}
    text=json.dumps(manifest,indent=2,ensure_ascii=False)+'\n'
    if args.check:
        if (BASE/'manifest.json').read_text(encoding='utf-8')!=text: raise ValueError('Manifest differs from actual processing')
    else: (BASE/'manifest.json').write_text(text,encoding='utf-8')
    print(json.dumps({'status':'PASS','check_only':args.check,'originals_verified':len(inventory['sources']),'events':len(events),'full_scale_samples':0,'audition':False}))

if __name__=='__main__': main()
