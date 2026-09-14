"""Native ONE05 presentation recording guards for source-bound WAV comparison.

No capture.json, recorder migration, video encoding or invented stop clock.
The original Candidate06 and Candidate07 packages execute the same named actor.
"""
from pathlib import Path
import re
import candidate07_legacy_motion as native

require=native.require
TRANSPORT_FIELDS=('frames','dimensions','audio_duration_seconds','audio_sample_rate','audio_channels','audio_sample_frames',
                  'source_wav_layout','ordered_frame_identity_sha256','actual_real_stop_tail_seconds',
                  'recorded_audio_after_last_callback_seconds','first_frame_audio_seconds','last_frame_audio_seconds')

def prepare(folder,args):
    return native.prepare(folder,args,presentation=True)

def transport(summary,ledger,artifact_files):
    recorded={row['path']:row for row in artifact_files}
    require(len(recorded)==len(artifact_files),'Duplicate native presentation artifact path')
    for row in summary['input_files']+ledger:
        expected={'path':row['file'],'bytes':row['bytes'],'sha256':row['sha256']}
        require(recorded.get(expected['path'])==expected,'Native recording changed after raw binding')
    result={key:summary[key] for key in TRANSPORT_FIELDS}
    result.update(schema='one07.native_presentation_transport.v1',status='PASS',path_base='fixture_folder',
                  input_format='legacy-presentation',metadata_created=False,
                  limit='Native callback/PCM/file validation only. Actual mixer stop clock and writer-pending flags were not serialized; no common-recorder transport claim.')
    for key,name in (('frames_csv','frames.csv'),('audio','gameplay_master.wav'),('checks','checks.txt'),('observations','observations.csv'),('chapters','chapters.csv'),('input_events','input_events.csv')):
        require(name in recorded,'Missing original presentation evidence: '+name);result[key]=recorded[name]
    return result

def validate_actor(folder,done,log,summary):
    import run_candidate07_checks as runner
    actor=summary['gameplay_checks']
    require(done=={'complete':1,'failures':0,'checks':actor['checks'],'frames':summary['frames'],'profile':0} and actor['status']=='PASS',
            'Presentation must finish its actual passing non-profile recording')
    labels=[label.removesuffix('\r') for label in runner.assertions(folder/'checks.txt',done['checks'])]
    require(re.findall(r'\bONE05_PRESENTATION\s+(PASS|FAIL)\s*\|\s*([^\r\n]*)',log)==[('PASS',label) for label in labels],
            'Original presentation assertions differ from engine log')
    require('All six actual catalog configurations were acquired and shown' in labels and
            'At least four actual pistol/rifle old-magazine releases occurred in the recorded sequence' in labels,
            'Native presentation lacks its six-configuration/mechanical completion checks')
    rows=native.legacy.read_csv(folder/'observations.csv',{'world_seconds','family','upgraded','shots','operation','magazine_drops'})
    configurations=set()
    for row in rows:
        family=native.legacy.integer(row['family'],'family');upgraded=native.legacy.integer(row['upgraded'],'upgraded')
        if family in (0,1,2):
            require(upgraded in (0,1),'Invalid observed upgrade state');configurations.add((family,upgraded))
    require(configurations=={(f,u) for f in range(3) for u in (0,1)},'Native observations do not retain all six actual configurations')
    require(summary['dimensions']==[1600,900] and summary['audio_sample_rate']==48000 and summary['audio_channels']==2,
            'Presentation original viewport/PCM format differs')
    return labels

def native_c06_proof(proof,root,source,runtime):
    require(proof.get('candidate')=='06' and proof.get('source_commit')==source and proof.get('result')=='BUILD_PASS_PENDING_RELEASE_AUDIT' and
            Path(proof['fresh_clone_root']).resolve()==root,'Original C06 build proof/source/root differs')
    for key in ('required_runtime_bytes_bound_to_source_commit','all_28_source_tests_pass','tracked_source_unchanged_after_verification',
                'candidate06_eight_source_assets_match','candidate06_source_assets_unchanged_after_verification'):
        require(proof.get('checks',{}).get(key) is True,'Native C06 build proof gate missing: '+key)
    require(sorted(proof.get('built_runtime_files',[]),key=lambda r:r['path'])==sorted(runtime,key=lambda r:r['path']),
            'Original C06 proof runtime identity differs')

def run_binding(args,folder,summary,ledger,public):
    import run_candidate07_checks as runner
    import assemble_candidate07_capture as media
    require(args.run_result is not None,'Native presentation source binding needs its actual runner result')
    report=native.common.strict_json(args.run_result)
    require(report.get('schema')=='one07.private_runtime.v1' and report.get('check')=='presentation' and
            report.get('candidate') in ('06','07') and report.get('mode') in ('packaged','editor') and
            report.get('capture_requested') is True and report.get('recording') is True,'Expected native presentation runner identity')
    require(report['candidate']=='07' or report['mode']=='packaged','Historical C06 presentation must use its original packaged runtime')
    require(report.get('requested')=={'weapon_variants':6,'native_recorder':'ONE05PresentationCapture'} and
            report.get('settings')=={'width':1600,'height':900,'max_fps':120,'vsync':0,'screen_percentage':100},
            'Native presentation requested configuration or settings differ')
    require(args.capture_kind==('packaged' if report['mode']=='packaged' else 'editor-game'),'Operator capture kind differs from actual run')
    require(not args.source_revision or args.source_revision==report.get('source_commit' if report['mode']=='packaged' else 'observed_head'),
            'Supplied source/base differs from actual run')
    if report['mode']=='editor':
        require(not public and report.get('source_commit') is None and args.source_state!='exact-commit','Editor WIP cannot claim a built commit')
        require(len(report.get('runtime_files',[]))==2 and {r['path'] for r in report['runtime_files']}==
                {Path(report['executable']).name,'Binaries/Win64/UnrealEditor-ProjectONE.dll'},'WIP native presentation must retain editor/DLL identities')
    saved=Path(report['saved_root']).resolve(strict=True);relative=Path(report['fixture_folder'])
    require(not relative.is_absolute() and '..' not in relative.parts and folder==(saved/relative).resolve(strict=True) and folder.is_relative_to(saved),
            'Native presentation folder differs from actual run')
    artifacts=report.get('artifacts',{})
    require(set(artifacts)=={'driver'} and len(artifacts['driver'])==1 and Path(artifacts['driver'][0]['folder']).resolve()==folder,'Wrong native actor inventory')
    actual=[runner.record(p,folder) for p in sorted(folder.rglob('*')) if p.is_file()]
    require(actual==artifacts['driver'][0]['files'],'Original presentation artifact inventory or bytes changed')
    require(transport(summary,ledger,actual)==report.get('capture'),'Native presentation transport differs from original run')
    log_path=args.run_result.resolve().parent/'engine.log'
    require(runner.record(log_path)==report.get('engine_log'),'Original presentation engine log changed')
    log=runner.readtext(log_path);done=runner.completion(log,'ONE05_PRESENTATION_COMPLETE')
    for name,expected in (('t.MaxFPS',120),('r.VSync',0),('r.ScreenPercentage',100)):
        values=re.findall(re.escape(name)+r'\s*=\s*"?(-?[\d.]+)',log)
        require(values and all(float(value)==expected for value in values),'Actual presentation setting readback differs: '+name)
    require(done==report.get('completion'),'Original presentation completion differs')
    validate_actor(folder,done,log,summary)
    require(not re.search(r'\bFAIL\s*\||Fatal error:|Assertion failed:|Ensure condition failed:|Result=\{Fail\}',log),'Original presentation run reports a failure')
    before,after=report.get('source_before',{}),report.get('source_after',{})
    require(before.get('files') and runner.same_runtime_inputs(before,after,report['mode']) and
            report.get('inputs_unchanged') is True and report.get('state')=='PASS' and report.get('exit_code')==0,
            'Native presentation runner was not passing with unchanged inputs')
    binding={'status':'PRESENTATION_RUN_ARTIFACTS_BOUND','candidate':report['candidate'],'source_commit':report.get('source_commit'),
             'observed_head':report.get('observed_head'),'report':media.identity(args.run_result),'completion':done,
             'runner':report.get('runner'),'validation_tool':media.identity(Path(__file__)),
             'runtime_files':report.get('runtime_files',[]),'source_snapshot_sha256':before.get('snapshot_sha256'),
             'limit':'Actual native actor, callback frames and original PCM bytes rechecked. No mixer stop clock, public movie, playback, audition or fresh build is claimed.'}
    if public:
        require(args.build_root and args.source_state=='exact-commit' and args.source_revision and report['mode']=='packaged',
                'Public native audio requires exact committed packaged source and build root')
        root=args.build_root.resolve(strict=True);package=Path(report['package_root']).resolve(strict=True)
        require(root==Path(report['root']).resolve() and report['source_commit']==args.source_revision and
                report.get('observed_head')==args.source_revision and before==after and before.get('git_status')=='' and
                runner.source_snapshot(root)==before,'Public native presentation source is not exact, unchanged and clean')
        require(all(report.get(key) is True for key in ('prelaunch_source_runtime_verified','postrun_source_runtime_verified')),'Native presentation lacks runtime verification gates')
        runtime=[runner.record(package/name,package) for name in runner.RUNTIME]
        require(runtime==report['runtime_files'] and saved==package/'ProjectONE/Saved' and
                Path(report['executable']).resolve()==package/'ProjectONE/Binaries/Win64/ProjectONE.exe','Native presentation six runtime identities differ')
        proof_path=Path(report['build_proof_path']).resolve(strict=True)
        require(runner.record(proof_path)==report['build_proof'],'Original native build proof changed')
        proof=native.common.strict_json(proof_path)
        if report['candidate']=='06':native_c06_proof(proof,root,args.source_revision,runtime)
        else:
            require(proof.get('schema')=='one07.runtime_build.v1' and proof.get('status')=='PASS' and proof.get('candidate')=='07' and
                    proof.get('source_commit')==args.source_revision and sorted(proof.get('runtime_files',[]),key=lambda r:r['path'])==sorted(runtime,key=lambda r:r['path']),
                    'C07 presentation fresh-build proof differs')
            require(proof.get('build_logs'),'C07 build proof lacks retained logs')
            for row in proof['build_logs']:
                path=(root/row['path']).resolve(strict=True)
                require(path.is_relative_to(root) and runner.record(path,root)==row,'C07 presentation build log changed')
        binding.update(status='PASSING_NATIVE_PRESENTATION_SOURCE_RUNTIME_AND_BUILD_BOUND',build_proof=media.identity(proof_path))
    media.portable(binding)
    return binding
