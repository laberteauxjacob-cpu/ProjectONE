"""Offline, source-bound profile analysis. Never launches UE or discards timing rows.

Reuse the committed UE final-header parser. Numeric timelines retain every row
and column; private raw event text/command metadata remain in the bound originals.
"""
from __future__ import annotations
import argparse
import csv
import io
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import analyze_candidate03_performance as engine_csv
from run_candidate07_checks import require, record, readtext, assertions, same_runtime_inputs, RUNTIME

C05='ONECandidate05Presentation/'
C07='ONECandidate07Profile/'
WEAPONS={'M4A1':(0,0),'Overcurrent':(0,1),'870':(1,0)}

def number(value):
    result=float(value) if str(value).strip() else 0.
    require(math.isfinite(result),'Nonfinite numeric sample')
    return result

def integer(value):
    result=number(value)
    require(result==int(result),'Nonintegral counter')
    return int(result)

def safe(value):
    if isinstance(value,dict):
        for key,item in value.items(): safe(key); safe(item)
    elif isinstance(value,(list,tuple)):
        for item in value: safe(item)
    elif isinstance(value,str):
        require(not engine_csv.PRIVATE_VALUE.search(value),'Private path/token in portable output')

def scalar(text,name):
    values=re.findall(r'^'+re.escape(name)+r':\s*([\d.eE+-]+)\s*$',text,re.M)
    require(len(values)==1,'Missing/duplicate report scalar: '+name)
    return number(values[0])

def analyze(result_path):
    result=json.loads(readtext(result_path))
    require(result.get('schema')=='one07.private_runtime.v1' and result.get('state')=='PASS' and
            result.get('check')=='profile' and result.get('inputs_unchanged') is True and result.get('recording') is False,
            'Need a completed, unchanged, recording-free profile run')
    require(result['exit_code']==0,'Nonzero runtime exit')
    before=result['source_before']
    require(before.get('files'),'Empty source/content input snapshot')
    require(same_runtime_inputs(before,result.get('source_after',{}),result['mode']) and result['observed_head']==before['observed_head'] and
            re.fullmatch(r'[0-9a-f]{40}',result['observed_head']), 'Run source snapshot/HEAD binding differs')
    require(before['snapshot_sha256']==hashlib.sha256(json.dumps(before['files'],sort_keys=True).encode()).hexdigest(), 'Input snapshot digest differs')
    require((result['mode']=='editor' and result['source_commit'] is None) or
            (result['mode']=='packaged' and result['source_commit']==result['observed_head'] and not before['git_status'] and result.get('build_proof')),
            'Wrong editor WIP or packaged source identity')
    require(len(result['runtime_files'])==(2 if result['mode']=='editor' else 6),'Runtime identity set is incomplete')
    if result['mode']=='packaged': require({r['path'] for r in result['runtime_files']}==set(RUNTIME),'Packaged runtime identities have wrong paths')
    for artifacts in result['artifacts'].values():
        require(len(artifacts)==1,'Ambiguous actor folder')
        artifact=artifacts[0]; folder=Path(artifact['folder'])
        for expected in artifact['files']:
            path=(folder/expected['path']).resolve()
            require(path.is_relative_to(folder.resolve()) and record(path,folder)==expected,'Raw actor artifact changed')
    require(record(result_path.parent/'engine.log')==result['engine_log'],'Raw runtime log changed')
    driver=Path(result['artifacts']['driver'][0]['folder'])
    text=readtext(driver/'checks.txt'); assertions(driver/'checks.txt',result['completion']['checks'])
    require(scalar(text,'Complete')==1 and scalar(text,'Failures')==0 and scalar(text,'Frames')==0,'Failed or recorded driver')
    require(len(list(csv.reader(io.StringIO(readtext(driver/'frames.csv')))))==1,'Profile frame manifest contains media callbacks')
    files=list((driver/'CSV').glob('*.csv'))+list((driver/'CSV').glob('*.csv.gz'))
    require(len(files)==1,'Expected one complete engine CSV')
    engine_csv.SAFE_METADATA.update({'one_carried_weapon','one_carried_family','one_pickup_fixture','one_media_capture',
                                    'one_requested_enemies','one_c07_companion','one_c07_explicit_falls'})
    capture=engine_csv.read_capture(files[0]); header=capture['header']
    require(len(set(header))==len(header) and header[0]=='EVENTS','Duplicate/invalid final header')
    requested=result['requested']; count=requested['enemies']; weapon=requested['weapon']
    meta=capture['metadata']
    require(meta.get('one_requested_enemies')==str(count) and meta.get('one_carried_weapon')==weapon and
            meta.get('one_media_capture')=='none','CSV workload identity differs')
    require(meta.get('systemresolution.resx')=='1600' and meta.get('systemresolution.resy')=='900','Actual CSV viewport differs')
    require(meta.get('one_scenario')=='candidate06_legacy05_two_machines_production_input_combat','Unexpected measured driver')
    require(scalar(text,'Profile requested live')==count,'Driver requested count differs')
    index={name:i for i,name in enumerate(header)}
    needed=['FrameTime']+[C05+n for n in ('RequestedLive','Live','ProfileSeconds','BoxState','UpgradeState','Shots','EffectiveFamily','EffectiveUpgraded')]
    require(all(name in index for name in needed),'Missing measured timing/workload columns')
    rows=[]; elapsed=0.; events=[]
    for frame,(line,raw) in enumerate(capture['rows']):
        values=[number(raw[i] if i<len(raw) else '') for i in range(1,len(header))]
        mapped=dict(zip(header[1:],values)); ms=mapped['FrameTime']; require(ms>=0,'Negative frame duration')
        rows.append({'frame':frame,'line':line,'start':elapsed,'end':elapsed+ms/1000.,'values':values,'data':mapped})
        elapsed+=ms/1000.; events.append(raw[0])
    valid=[r for r in rows if r['data'][C05+'RequestedLive']==count]
    require(valid and len(valid)==scalar(text,'Profile actual frames'),'Driver frame count differs from complete CSV')
    require(all(r['data'][C05+'RequestedLive'] in (0,count) for r in rows),'A different population request appeared')
    exact=sum(r['data'][C05+'Live']==count for r in valid)
    require(exact==scalar(text,'Frames at exact requested count') and exact>0,'Exact-population count differs')
    previous=0.; both_seconds=0.; both_frames=[]
    for row in valid:
        data=row['data']; now=data[C05+'ProfileSeconds']; require(now>previous,'Driver profile clock is not monotonic')
        require(0<=data[C05+'Live']<=count,'Actual population outside requested bounds')
        if data[C05+'BoxState']==data[C05+'UpgradeState']==2:
            both_seconds+=now-previous; both_frames.append(row)
        previous=now
    require(previous>=25 and abs(both_seconds-scalar(text,'Both active seconds'))<.002,'Profile duration or both-machine overlap differs')
    require(both_seconds>=.5,'Required measured machine overlap absent')
    begin=[i for i,event in enumerate(events) if re.search(r'ONE05_PROFILE_BEGIN requested='+str(count)+r'\b',event)]
    end=[i for i,event in enumerate(events) if 'ONE05_PROFILE_END samples=' in event]
    require(len(begin)==len(end)==1 and begin[0]<end[0],'Missing/ambiguous CSV boundaries')
    starts=[(i,re.search(r'ONE05_UPGRADED_COMBAT_BEGIN phase=(\d+) shots=(\d+)',event)) for i,event in enumerate(events)]
    ends=[(i,re.search(r'ONE05_UPGRADED_COMBAT_END phase=(\d+) shots_committed=(\d+)',event)) for i,event in enumerate(events)]
    starts=[(i,m) for i,m in starts if m]; ends=[(i,m) for i,m in ends if m]
    require(len(starts)==len(ends)==1,'Missing/ambiguous actual combat events')
    start,sm=starts[0]; finish,em=ends[0]; shots=int(em[2]); phase=int(sm[1])
    require(start<finish and phase==int(em[1]) and shots>=(10 if weapon=='870' else 60),'Incomplete requested-weapon combat window')
    family,upgraded=WEAPONS[weapon]
    combat=[r for r in valid if r['data'].get(C05+'Phase')==phase]
    require(combat and all((r['data'][C05+'EffectiveFamily'],r['data'][C05+'EffectiveUpgraded'])==(family,upgraded) for r in combat),'Combat contains a different carried weapon')
    combat_shots=[integer(r['data'][C05+'Shots']) for r in combat]
    require(all(b>=a for a,b in zip(combat_shots,combat_shots[1:])) and min(combat_shots)>=int(sm[2]) and
            max(combat_shots)<=int(sm[2])+shots and max(combat_shots)-int(sm[2])>=2,'Actual shot counters disagree with combat events')
    bindings={'runner_result':record(result_path),'runtime_log':record(result_path.parent/'engine.log'),
              'engine_csv':record(files[0]),'driver_assertions':record(driver/'checks.txt'),'driver_observations':record(driver/'observations.csv'),
              'parser':record(Path(engine_csv.__file__)),'analyzer':record(Path(__file__))}
    census_rows=None; census_header=None; census=None
    if result['candidate']=='07':
        require(meta.get('one_c07_companion')=='all_frames_physicality_census' and meta.get('one_c07_explicit_falls')==requested['falls'],'C07 companion metadata differs')
        folder=Path(result['artifacts']['companion'][0]['folder']); observations=json.loads(readtext(folder/'observations.json'))
        assertions(folder/'checks.txt',result['companion_completion']['checks'])
        require(observations.get('schema')=='one07.profile_companion.v1' and observations.get('complete') is True and observations.get('failures')==0,'Failed/incomplete companion')
        reader=csv.DictReader(io.StringIO(readtext(folder/'timeline.csv'))); census_header=reader.fieldnames
        require(census_header and len(set(census_header))==len(census_header),'Invalid census header')
        census_rows=[]
        for raw in reader:
            require(None not in raw and all(v is not None for v in raw.values()),'Malformed census row')
            census_rows.append({k:number(v) for k,v in raw.items()})
        active=[r for r in census_rows if r['csv_active']==1]
        require(len(census_rows)==observations['retained_rows'] and len(active)==observations['csv_active_rows']==result['companion_completion']['csv_rows'],'Census row counts differ')
        require(all(b['frame']>a['frame'] and b['world_seconds']>=a['world_seconds'] for a,b in zip(census_rows,census_rows[1:])),'Nonmonotonic census identity/clock')
        for row in census_rows:
            require(row['live']==row['standing']+row['fallen']+row['get_up'] and row['simulated_bodies']==row['living_bodies']+row['dead_bodies'],'Census state/body partition differs')
        require(active and all((r['width'],r['height'],r['cap'],r['vsync'],r['screen_percentage'])==(1600,900,120,0,100) for r in active),'Actual per-frame settings differ')
        require(observations['requested_live']==count and observations['weapon']==weapon and observations['fall_mode']==requested['falls'],'Census workload differs')
        for field,column in (('exact_population_rows','live'),('fallen_rows','fallen'),('get_up_rows','get_up'),('dead_rows','dead')):
            actual=sum(r[column]==count if column=='live' else r[column]>0 for r in active)
            require(actual==observations[field],'Census occupancy summary differs: '+field)
        lookup={integer(r['frame']):r for r in active}; matched=set(); unmatched_engine=[]
        mapping={'Live':'live','Standing':'standing','Fallen':'fallen','GetUp':'get_up','Dead':'dead','SimulatedBodies':'simulated_bodies',
                 'AwakeBodies':'awake_bodies','ZombieVoices':'zombie_voices','AmbientVoices':'ambient_voices','ContactVoices':'contact_voices'}
        require(all(C07+k in index for k in ['Frame','RequestedLive',*mapping]),'Missing C07 CSV counters')
        for row in rows:
            frame=integer(row['data'][C07+'Frame'])
            if frame==0: unmatched_engine.append(row['frame']); continue
            require(frame in lookup and frame not in matched,'Census global frame missing/duplicate')
            matched.add(frame)
            require(all(row['data'][C07+key]==lookup[frame][column] for key,column in mapping.items()),'Engine/census frame counters disagree')
        unmatched_census=sorted(set(lookup)-matched)
        require(set(unmatched_engine)<={0,len(rows)-1} and set(unmatched_census)<={integer(active[0]['frame']),integer(active[-1]['frame'])},'Unmatched interior frame; retain and investigate')
        census={'all_rows':len(census_rows),'csv_active_rows':len(active),'exact_requested_fraction':observations['exact_population_rows']/len(active),
                'state_occupancy_rows':{k:sum(r[k]>0 for r in active) for k in ('standing','fallen','get_up','dead')},
                'maxima':{k:max(r[k] for r in active) for k in ('live','fallen','get_up','dead','simulated_bodies','awake_bodies','zombie_voices','ambient_voices','contact_voices')},
                'accepted_explicit_falls':observations['accepted_falls'],'attempted_explicit_falls':observations['fall_attempts'],
                'matched_global_frames':len(matched),'unmatched_engine_boundary_indices_retained':unmatched_engine,
                'unmatched_census_boundary_frames_retained':unmatched_census}
        for name in ('timeline.csv','fall_events.csv','checks.txt','observations.json'): bindings['census_'+name]=record(folder/name)
    timings=[name for name in header[1:] if name in ('FrameTime','GameThread','RenderThread','RHIThread','GameThreadTime','RenderThreadTime','GPU Frame') or
             (len(name.split('/'))>=3 and not name.startswith('COUNTS/') and name.split('/')[0] in {'ONEPhysicality','ONEProgression','ONECandidate05Presentation','ONECandidate07Profile','Chaos','ChaosPhysicsSolver','PhysicsVerbose','Exclusive'})]
    stats={name:engine_csv.summary([r['data'][name] for r in rows]) for name in timings}
    require(all(min(r['data'][name] for r in rows)>=0 for name in timings),'Negative timing scope')
    report={'schema':'one07.profile_analysis.v1','status':'PASS','candidate':result['candidate'],'execution_mode':result['mode'],
            'source_commit':result['source_commit'],'observed_head':result['observed_head'],
            'source_snapshot_sha256':result['source_before']['snapshot_sha256'],'source_was_dirty':bool(result['source_before']['git_status']),
            'runtime_files':result['runtime_files'],'build_proof':result.get('build_proof'),'requested':requested,'capture_metadata':meta,'bindings':bindings,
            'all_engine_rows':len(rows),'all_engine_numeric_columns':len(header)-1,'zero_duration_rows':sum(r['data']['FrameTime']==0 for r in rows),
            'all_frame_time_seconds':elapsed,'all_frame_timings':stats,
            'frame_spike_indices':{str(limit):[r['frame'] for r in rows if r['data']['FrameTime']>limit] for limit in (16.7,33.3,50.,100.)},
            'legacy_driver_census':{'valid_rows':len(valid),'uninitialized_boundary_rows_retained':len(rows)-len(valid),
                'exact_requested_rows':exact,'exact_requested_fraction':exact/len(valid),
                'live_histogram':{str(n):sum(r['data'][C05+'Live']==n for r in valid) for n in range(count+1)},
                'both_active_rows':len(both_frames),'both_active_profile_seconds':both_seconds,
                'both_active_frame_time_seconds':sum(r['data']['FrameTime']/1000. for r in both_frames)},
            'actual_requested_weapon_combat':{'event_first_frame':start,'event_last_frame':finish,'phase':phase,'committed_shots':shots},
            'c07_census':census,
            'limitations':['Every engine timing row, including zero frame 0, is retained. Percentiles interpolate at (n-1)*p; timings are milliseconds.',
                'Missing early numeric cells are zero per UE stream semantics. Raw event text and private metadata remain in hash-bound originals.',
                'Nested and worker scope durations are not summed into wall time. Voice counts mean component playback, not mixer audibility.',
                'C07 post-driver census includes replenishment; legacy driver live counts are sampled before its replenishment and are retained separately.',
                'Actor contact/fall/recovery sums are current-actor snapshots and can decrease after retirement.',
                'Simulated/awake body census covers infected leader meshes; detached debris and other world physics are represented only by the separate engine counters.',
                'C06 supports unchanged 6/12/18 profiles. C07 1/2 and explicit mixed falls are additional workloads, not manufactured matched baselines.',
                'One scripted, warm, capped run with disclosed health restoration, machine/pickup setup and unspecified random streams; no universal FPS, native-input or audio-review claim.']}
    safe(report); safe(header[1:]); safe(census_header)
    return report,header,rows,census_header,census_rows

def main():
    ap=argparse.ArgumentParser(description=__doc__); ap.add_argument('result',type=Path); ap.add_argument('--output',type=Path,required=True)
    args=ap.parse_args(); require(not args.output.exists(),'Preserve previous analysis: output must be new')
    report,header,rows,census_header,census_rows=analyze(args.result.resolve())
    args.output.mkdir(parents=True)
    with (args.output/'engine_timeline.csv').open('w',newline='',encoding='utf-8') as stream:
        writer=csv.writer(stream); writer.writerow(['frame_index','raw_source_line','elapsed_start_seconds','elapsed_end_seconds',*header[1:]])
        for row in rows: writer.writerow([row['frame'],row['line'],row['start'],row['end'],*row['values']])
    if census_rows is not None:
        with (args.output/'census.csv').open('w',newline='',encoding='utf-8') as stream:
            writer=csv.DictWriter(stream,fieldnames=census_header); writer.writeheader(); writer.writerows(census_rows)
    report['retained_numeric_outputs']=[record(p) for p in sorted(args.output.glob('*.csv'))]
    (args.output/'analysis.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    timing=report['all_frame_timings']['FrameTime']; requested=report['requested']
    text=f"# Candidate{report['candidate']} profile\n\n{requested['weapon']}; requested live {requested['enemies']}; explicit falls {requested['falls']}; {report['execution_mode']}.\n\n"
    text+=f"Source commit: {report['source_commit'] or 'none (editor WIP)'}. Observed HEAD: {report['observed_head']}. Input snapshot: {report['source_snapshot_sha256']}.\n\n"
    text+=f"All {report['all_engine_rows']} engine rows retained, including {report['zero_duration_rows']} zero-duration rows. Frame time mean {timing['mean_ms']:.4f} ms; p95 {timing['p95_ms']:.4f}; p99 {timing['p99_ms']:.4f}; max {timing['max_ms']:.4f}.\n\n"
    text+=f"Legacy driver exact-population fraction: {report['legacy_driver_census']['exact_requested_fraction']:.6f}; both machines active {report['legacy_driver_census']['both_active_profile_seconds']:.6f} profile seconds. Requested weapon committed {report['actual_requested_weapon_combat']['committed_shots']} shots.\n\n"
    if report['c07_census']: text+='C07 state occupancy and all per-frame body/voice counters are recorded in [analysis.json](analysis.json) and [census.csv](census.csv).\n\n'
    text+='[Complete numeric engine timeline](engine_timeline.csv). Raw evidence hashes, host metadata and scope timings are in [analysis.json](analysis.json).\n\n'
    text+='\n'.join('- '+item for item in report['limitations'])+'\n'
    (args.output/'README.md').write_text(text,encoding='utf-8')
    print(json.dumps({'status':'PASS','rows':len(rows),'output':str(args.output)}))

if __name__=='__main__': main()
