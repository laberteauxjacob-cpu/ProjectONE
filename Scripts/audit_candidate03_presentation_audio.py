"""Measure C03's genuine 16-phase presentation capture; no listening verdict.

python Scripts/audit_candidate03_presentation_audio.py \
  --source Saved/Candidate03/PresentationCapture/gameplay_master.wav \
  --frames Saved/Candidate03/PresentationCapture/frames.csv \
  --observations Saved/Candidate03/PresentationCapture/observations.csv

All inputs and output must remain inside the checkout. C02 tools/evidence are
unchanged. Phase boundaries come from captured audio_seconds, not world time.
Numerical energy markers cannot identify a sound or certify perceived quality.
"""
from pathlib import Path
import argparse
import csv
import datetime
import hashlib
import json
import math

from audit_gameplay_audio import audit, dbfs, relative_path

ROOT=Path(__file__).resolve().parents[1]
MODES=('standing','sideways','diagonal_sprint','rapid_aim')


def read_frames(path):
    with path.open(encoding='utf-8-sig',newline='') as f:
        rows=list(csv.DictReader(f))
    required={'file','audio_seconds','world_seconds','phase','weapon','ammo','operation'}
    if not rows or not required.issubset(rows[0]):
        raise ValueError('frames.csv is empty or lacks required capture columns')
    seen=set()
    for i,r in enumerate(rows):
        for key in ('audio_seconds','world_seconds'):
            r[key]=float(r[key])
            if not math.isfinite(r[key]) or r[key]<0:
                raise ValueError('Invalid timestamp in frames.csv')
        for key in ('phase','weapon','ammo','operation'):r[key]=int(r[key])
        if not 0<=r['phase']<16 or r['weapon'] not in (0,1):
            raise ValueError('Unexpected presentation phase or weapon')
        if Path(r['file']).name!=r['file'] or r['file'] in seen:
            raise ValueError('Frame filenames must be unique basenames')
        seen.add(r['file'])
        if i and (r['audio_seconds']<=rows[i-1]['audio_seconds'] or r['world_seconds']<rows[i-1]['world_seconds'] or r['phase']<rows[i-1]['phase']):
            raise ValueError('Capture timestamps/phases are not ordered')
    return rows


def phase_windows(rows):
    groups={phase:[r for r in rows if r['phase']==phase] for phase in range(16)}
    specs=[]
    annotations=[]
    for phase,rs in groups.items():
        if not rs:continue
        start=rs[0]['audio_seconds']
        after=next((r for r in rows if r['phase']>phase),None)
        end=after['audio_seconds'] if after else rs[-1]['audio_seconds']
        if end<=start:raise ValueError('Phase has no measured duration')
        name=f'{phase:02d}_{"carbine" if phase<8 else "shotgun"}_{"bright" if phase%8<4 else "dim"}_{MODES[phase%4]}'
        specs.append({'name':name,'start_seconds':start,'end_seconds':end})
        drops=[]
        for a,b in zip(rs,rs[1:]):
            if a['weapon']==b['weapon']==phase//8 and b['ammo']<a['ammo']:
                drops.append({'previous_frame_seconds':a['audio_seconds'],'observed_frame_seconds':b['audio_seconds'],'ammunition_decrease':a['ammo']-b['ammo']})
        reload_rows=[r for r in rs if r['operation'] in (4,5,6,7)]
        firing=None
        if drops:
            firing={'start_seconds':max(start,drops[0]['previous_frame_seconds']-.02),
                'end_seconds':min(end,drops[-1]['observed_frame_seconds']+(.3 if phase<8 else .6))}
        reload_window=None
        if reload_rows:
            reload_window={'start_seconds':max(start,reload_rows[0]['audio_seconds']-.05),
                'end_seconds':min(end,reload_rows[-1]['audio_seconds']+.15)}
        annotations.append({'phase':phase,'name':name,'weapon':'carbine' if phase<8 else 'shotgun',
            'lighting':'bright' if phase%8<4 else 'dim','mode':MODES[phase%4],
            'frame_count':len(rs),'start_seconds':start,'end_seconds':end,
            'duration_seconds':end-start,'minimum_expected_seconds':8.5 if phase%4==0 else 4.5,
            'weapon_mismatches_after_equip_warmup':sum(r['weapon']!=phase//8 for r in rs if r['audio_seconds']>=start+.7),
            'ammunition_drop_markers':drops,'firing_window':firing,
            'reload_expected':phase%4==0,'reload_window':reload_window})
    return specs,annotations


def envelope_window(windows,start_seconds,end_seconds,silence_dbfs):
    # Subwindow energy is integrated from the existing 10 ms all-channel RMS
    # envelope. Fractional boundary windows are approximated proportionally.
    start,end=start_seconds,end_seconds
    square_time=duration=0.0
    peak=None
    active=[]
    for a,b,rms_db,peak_db in windows:
        overlap=max(0,min(b,end)-max(a,start))
        if not overlap:continue
        duration+=overlap
        square_time+=overlap*(10**(rms_db/10) if rms_db is not None else 0)
        if peak_db is not None:peak=max(peak if peak is not None else peak_db,peak_db)
        if rms_db is not None and rms_db>silence_dbfs:active.append(rms_db)
    return {'start_seconds':start,'end_seconds':end,'covered_seconds':duration,
        'approximate_rms_dbfs':dbfs(math.sqrt(square_time/duration)) if duration else None,
        'maximum_sample_peak_dbfs':peak,'rms_windows_above_silence_threshold':len(active),
        'active_window_rms_min_max_dbfs':[min(active),max(active)] if active else None}


def source_bank(path):
    manifest=json.loads(path.read_text(encoding='utf-8'))
    events=manifest['events']
    measured={}
    failures=[]
    for name,event in events.items():
        source,relative=relative_path(event['source'])
        measurement=audit(source,[],10,-60,-38,50)
        sha=measurement['source_sha256']
        valid=(sha==event['sha256'] and measurement['overall']['full_scale_sample_count']==0
            and measurement['overall']['nonzero_sample_count']>0
            and measurement['format']['sample_rate_hz']==48000
            and measurement['format']['channels']==1
            and measurement['format']['bits_per_sample']==16)
        if not valid:failures.append(name)
        measured[name]={'source':relative,'sha256':sha,'manifest_sha256_matches':sha==event['sha256'],
            'family':event['family'],'variant':event['variant'],'format':measurement['format'],
            'sample_peak_dbfs':measurement['overall']['sample_peak_dbfs'],
            'rms_dbfs':measurement['overall']['rms_dbfs'],
            'full_scale_sample_count':measurement['overall']['full_scale_sample_count'],
            'nonzero_sample_count':measurement['overall']['nonzero_sample_count'],
            'authored_profile':event['profile']}
    families={}
    for family in ('Carbine','Shotgun'):
        rs=[v for v in measured.values() if v['family']==family]
        # Explicitly ignore seed when counting independent authored profiles.
        profiles={json.dumps({k:v for k,v in r['authored_profile'].items() if k!='seed'},sort_keys=True) for r in rs}
        families[family]={'file_count':len(rs),'unique_pcm_hashes':len({r['sha256'] for r in rs}),
            'unique_nonseed_layer_profiles':len(profiles),
            'source_rms_range_dbfs':[min(r['rms_dbfs'] for r in rs),max(r['rms_dbfs'] for r in rs)] if rs else None}
        if len(rs)!=6 or len(profiles)!=6:failures.append(family+' source-bank coverage')
    return {'status':'PASS' if not failures else 'FAIL','manifest_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
        'failures':failures,'files':measured,'family_markers':families,
        'limit':'Distinct profiles and PCM hashes are numerical provenance markers, not proof of audible variation or realism.'}


def observe_selection(path):
    with path.open(encoding='utf-8-sig',newline='') as f:rows=list(csv.DictReader(f))
    if not rows:return {'status':'UNAVAILABLE','reason':'No observation rows'}
    id_column=next((k for k in ('last_shot_id','shots','total_shots') if k in rows[0]),None)
    index_column=next((k for k in ('shot_sound_index','last_sound_index','sound_index') if k in rows[0]),None)
    if not id_column or not index_column:
        return {'status':'UNAVAILABLE','reason':'Actual shot ID and selected-sound index columns are both required; waveform markers do not substitute.'}
    last_id=0
    last_indices={}
    dispatches=[]
    repeats=[]
    gaps=[]
    invalid=[]
    for r in rows:
        current=int(r[id_column])
        if current==last_id:continue
        if current<last_id:
            invalid.append('Shot IDs decreased');last_id=current;continue
        if current-last_id!=1:gaps.append({'previous_shot_id':last_id,'shot_id':current})
        phase=int(r.get('trial',r.get('phase',-1)))
        weapon=int(r['weapon']) if 'weapon' in r else phase//8
        index=int(r[index_column])
        item={'shot_id':current,'world_seconds':float(r['seconds']),
            'phase':phase,'weapon':weapon,'sound_index':index}
        if weapon not in (0,1) or not 0<=index<6:invalid.append(item)
        if last_indices.get(weapon)==index:repeats.append(item)
        last_indices[weapon]=index
        dispatches.append(item);last_id=current
    return {'status':'FAIL' if repeats or invalid else 'INCOMPLETE' if gaps or not dispatches else 'PASS',
        'source_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'id_column':id_column,'index_column':index_column,
        'unique_observed_dispatch_count':len(dispatches),'id_gaps':gaps,'invalid_rows':invalid,
        'immediate_repeats_per_weapon':repeats,'dispatches':dispatches,
        'limit':'Telemetry validates selection only. It does not establish that each selected SoundWave was audible in the master recording.'}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--source',required=True,type=relative_path)
    p.add_argument('--frames',required=True,type=relative_path)
    p.add_argument('--observations',type=relative_path)
    p.add_argument('--source-manifest',type=relative_path,default='ArtSource/Audio/Candidate03/manifest.json')
    p.add_argument('--output',type=relative_path,default='Evidence/Candidate03/StageC/audio_metrics.json')
    p.add_argument('--silence-threshold-dbfs',type=float,default=-60)
    p.add_argument('--event-threshold-dbfs',type=float,default=-38)
    args=p.parse_args()
    if not -160<=args.silence_threshold_dbfs<=0 or not -160<=args.event_threshold_dbfs<=0:
        p.error('Thresholds must be finite and within -160..0 dBFS')
    source,source_rel=args.source
    frame_path,frame_rel=args.frames
    output,output_rel=args.output
    if output in (source,frame_path,args.source_manifest[0]) or (args.observations and output==args.observations[0]):
        p.error('Output must differ from every input')
    rows=read_frames(frame_path)
    specs,annotations=phase_windows(rows)
    report=audit(source,specs,10,args.silence_threshold_dbfs,args.event_threshold_dbfs,50)
    # This wrapper does not claim an audio-input attempt was made for this file.
    report['measurement_limits'][0]='No perceptual audition was performed by this numerical auditor.'
    failed=[]
    missing=sorted(set(range(16))-{r['phase'] for r in rows})
    if missing:failed.append('Missing phases: '+','.join(map(str,missing)))
    phases={r['name']:r for r in report['phases']}
    windows=report['envelope']['windows']
    for item in annotations:
        measured=phases[item['name']]
        metric=measured['metrics']
        if measured['coverage']!='complete':failed.append(item['name']+' incomplete WAV coverage')
        if not metric or metric['silent_frame_count_at_threshold']==metric['frame_count']:
            failed.append(item['name']+' entirely below silence threshold')
        if item['duration_seconds']<item['minimum_expected_seconds']:failed.append(item['name']+' capture shorter than scenario minimum')
        if item['weapon_mismatches_after_equip_warmup']:failed.append(item['name']+' weapon annotation mismatch')
        for key in ('firing_window','reload_window'):
            value=item[key]
            required=key=='firing_window' or item['reload_expected']
            if value:
                value.update(envelope_window(windows,**value,silence_dbfs=args.silence_threshold_dbfs))
                if required and not value['rms_windows_above_silence_threshold']:failed.append(item['name']+' '+key+' below threshold')
            elif required:failed.append(item['name']+' missing '+key+' annotation')
        # These are measured mixed-audio bursts, not inferred discharges.
        item['energy_burst_count']=len(measured['overlapping_energy_event_indices'])
    if report['overall']['full_scale_sample_count']:failed.append('Master contains full-scale integer samples')
    bank=source_bank(args.source_manifest[0])
    if bank['status']!='PASS':failed.append('Source bank verification failed')
    selection=observe_selection(args.observations[0]) if args.observations else {'status':'UNAVAILABLE','reason':'No observations input supplied'}
    if selection['status']=='FAIL':failed.append('Actual sound-selection telemetry failed')
    family=[]
    for mode in range(8):
        a=next((r for r in annotations if r['phase']==mode),None)
        b=next((r for r in annotations if r['phase']==mode+8),None)
        if a and b and a['firing_window'] and b['firing_window']:
            x,y=a['firing_window']['approximate_rms_dbfs'],b['firing_window']['approximate_rms_dbfs']
            family.append({'carbine_phase':mode,'shotgun_phase':mode+8,
                'shotgun_minus_carbine_firing_window_rms_db':round(y-x,6) if x is not None and y is not None else None})
    report.update(candidate='03',generated_utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
        source=source_rel,frames=frame_rel,frames_sha256=hashlib.sha256(frame_path.read_bytes()).hexdigest(),
        command_parameters={'script':'Scripts/audit_candidate03_presentation_audio.py','source':source_rel,
            'frames':frame_rel,'observations':args.observations[1] if args.observations else None,
            'source_manifest':args.source_manifest[1],'output':output_rel,
            'silence_threshold_dbfs':args.silence_threshold_dbfs,'event_threshold_dbfs':args.event_threshold_dbfs},
        capture_annotations=annotations,source_bank=bank,sound_selection_telemetry=selection,
        mixed_family_window_markers=family,
        required_checks={'status':'FAIL' if failed else 'PASS','failures':failed,
            'missing_phases':missing,'perceptual_quality_approved':False})
    report['measurement_limits'] += [
        'Frames capture the current phase, weapon, operation and ammunition. Discharge times are bounded by observed ammo decreases, not exact audio onset timestamps.',
        'Subwindow RMS uses 10 ms energy windows and approximates boundaries. A non-silent mixed window may include mechanisms, impacts or other sounds.',
        'Family-window differences include cadence, movement and distance. They do not identify weapon timbre or prove audible source variation.',
        'The final phase ends at its last observed frame; recording beyond that frame is included only in overall metrics.',
        'Missing optional sound-selection telemetry is reported as unavailable and does not establish a no-repeat pass.']
    output.parent.mkdir(parents=True,exist_ok=True)
    output.write_text(json.dumps(report,indent=2,allow_nan=False)+'\n',encoding='utf-8')
    print(json.dumps({'report':output_rel,'checks':report['required_checks'],
        'duration_seconds':report['format']['duration_seconds'],'peak_dbfs':report['overall']['sample_peak_dbfs'],
        'selection_telemetry':selection['status']}))
    return 2 if failed else 0


if __name__=='__main__':raise SystemExit(main())
