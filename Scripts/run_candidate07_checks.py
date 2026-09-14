"""Run one bounded C07 check, or an unchanged C06 packaged profile baseline.

No retries, cleanup, process killing, publication or performance conclusions.
All raw outputs stay in Saved. Editor runs bind dirty inputs and a DLL but do
not claim a built commit. Packaged runs require an exact clean source/build proof.
"""
from __future__ import annotations
import argparse
import csv
import datetime as dt
import hashlib
import json
import math
from pathlib import Path
import re
import subprocess
import time
import uuid
from types import SimpleNamespace

import assemble_candidate06_capture as capture_tools
import candidate07_regression_checks as regression

RUNTIME = ('ProjectONE.exe', 'ProjectONE/Binaries/Win64/ProjectONE.exe',
           'ProjectONE/Content/Paks/ProjectONE-Windows.pak', 'ProjectONE/Content/Paks/ProjectONE-Windows.ucas',
           'ProjectONE/Content/Paks/ProjectONE-Windows.utoc', 'Engine/Extras/Redist/en-us/vc_redist.x64.exe')

def require(condition, message):
    if not condition:
        raise ValueError(message)

def digest(path):
    before = path.stat()
    value = hashlib.sha256()
    with path.open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    after = path.stat()
    require((before.st_size, before.st_mtime_ns) == (after.st_size, after.st_mtime_ns), 'File changed while hashing: ' + path.name)
    return value.hexdigest()

def record(path, base=None):
    return {'path': path.relative_to(base).as_posix() if base else path.name,
            'bytes': path.stat().st_size, 'sha256': digest(path)}

def readtext(path):
    data = path.read_bytes()
    return data.decode('utf-16' if data.startswith((b'\xff\xfe', b'\xfe\xff')) else 'utf-8-sig')

def git(root, *args):
    return subprocess.check_output(['git', *args], cwd=root, timeout=45).decode('utf-8').strip()

def source_snapshot(root):
    paths = [root / 'ProjectONE.uproject']
    for name in ('Source', 'Config', 'Content', 'Plugins', 'Build'):
        if (root / name).exists():
            paths.extend(p for p in (root / name).rglob('*') if p.is_file())
    rows = [record(p, root) for p in sorted(paths)]
    return {'observed_head': git(root, 'rev-parse', 'HEAD'), 'git_status': git(root, 'status', '--porcelain=v1'),
            'files': rows, 'snapshot_sha256': hashlib.sha256(json.dumps(rows, sort_keys=True).encode()).hexdigest()}

def processes():
    # Inventory only. Never stop or send input to an existing process.
    script = "@(Get-Process | Where-Object { $_.ProcessName -match '^(UnrealEditor|ProjectONE)' } | Select-Object Id,ProcessName) | ConvertTo-Json -Compress"
    text = subprocess.check_output(['powershell', '-NoProfile', '-Command', script], timeout=20).decode().strip()
    value = json.loads(text or '[]')
    return value if isinstance(value, list) else [value]

def same_runtime_inputs(before, after, mode):
    # Preserve full Git status as an observation, but WIP documentation/art
    # authoring does not mutate loaded Source/Config/Content/Plugins/Build bytes.
    if mode == 'editor':
        return all(before.get(key) == after.get(key) for key in ('observed_head','files','snapshot_sha256'))
    return mode == 'packaged' and before == after

def completion(log, marker, allow_failures=False):
    rows = re.findall(r'\b' + re.escape(marker) + r'\b([^\r\n]*)', log)
    require(len(rows) == 1, 'Missing/duplicate completion marker: ' + marker)
    result = {}
    for token in rows[0].strip().split():
        match = re.fullmatch(r'([A-Za-z_]\w*)=([+-]?\d+)', token)
        require(match is not None, 'Malformed completion integer field: ' + token)
        key, value = match.groups()
        require(key not in result, 'Duplicate completion field: ' + key)
        result[key] = int(value)
    require(result.get('failures', -1) >= 0 and result.get('checks', 0) > 0 and
            (allow_failures or result['failures'] == 0), 'Actor completion reports failure or no checks')
    return result

def validate_fixture_completion(values, check, capture=False):
    require(not capture or check in ('physicality','encounter','portability'), 'Profiles cannot record capture')
    require(values.get('frames', -1) >= 2 if capture else values.get('frames') == 0,
            'Captured-frame count differs from the requested recording mode')
    if check == 'profile':
        require(values.get('complete') == 1 and values.get('profile') == 1 and values.get('frames') == 0,
                'Profile was incomplete or recorded media')
    elif check == 'portability':
        require(values.get('complete') == 1 and values.get('variants') == 3 and
                values.get('observed_frames', 0) > 0, 'Portability did not complete three variant episodes')
    else:
        # The header starts at -1. Current Tick sets ordinary encounter to 0
        # after successful setup; only the explicit physicality driver reaches 7.
        expected_phase = 0 if check == 'encounter' else 7
        require(check in ('physicality','encounter') and values.get('phases') == expected_phase and
                values.get('encounter') == int(check == 'encounter'),
                'Wrong or incomplete physicality/encounter discriminator')

def assertions(path, expected):
    text = readtext(path)
    require(not re.search(r'^FAIL\s*\|', text, re.M), 'Actor assertion failed: ' + path.name)
    rows = re.findall(r'^PASS\s*\|\s*(.+)$', text, re.M)
    require(len(rows) == expected, 'Actor assertion count differs from engine completion: ' + path.name)
    return rows

def check_layout(check, candidate):
    if check in regression.MODES:
        flag,marker,directory,_,_=regression.MODES[check]
        return flag,directory,marker,'/Game/ONE/Maps/Containment'
    if check == 'profile':
        return ('ONE07Profile' if candidate == '07' else 'ONE05Profile', 'Candidate05/Profile',
                'ONE05_PRESENTATION_COMPLETE', '/Game/ONE/Maps/Containment')
    require(check in ('physicality', 'encounter', 'portability'), 'Unknown check')
    flag, directory, marker = {
        'physicality': ('ONE07PhysicalityCheck', 'Physicality', 'ONE07_PHYSICALITY_COMPLETE'),
        'encounter': ('ONE07Encounter', 'Encounter', 'ONE07_PHYSICALITY_COMPLETE'),
        'portability': ('ONE07PortabilityCheck', 'Portability', 'ONE07_PORTABILITY_COMPLETE')}[check]
    return flag, 'Candidate07/' + directory, marker, '/Game/ONE/Maps/' + ('Portability06' if check == 'portability' else 'Containment')

def validate_portability(folder, done, log, labels, capture=False):
    report = json.loads(readtext(folder/'checks.json'))
    require(report.get('schema') == 'one07.portability.v1' and report.get('complete') is True and
            report.get('map') == '/Game/ONE/Maps/Portability06', 'Wrong portability report schema, completion or map')
    for key, expected in (('checks', done['checks']), ('failures', 0), ('variants_completed', 3),
                          ('observed_frames', done['observed_frames']), ('captured_frames', done['frames'])):
        require(type(report.get(key)) in (int, float) and report[key] == expected, 'Portability count differs: ' + key)
    require(report.get('capture_requested') is capture and report.get('capture_complete') is capture, 'Portability recording state differs')
    variants = report.get('variants')
    require(isinstance(variants, list) and len(variants) == 3 and all(isinstance(v, str) and v for v in variants) and
            len(set(variants)) == 3, 'Portability report lacks three distinct actual variant identities')
    rows = report.get('assertions')
    require(isinstance(rows, list) and len(rows) == done['checks'], 'Portability JSON assertion count differs')
    require(all(isinstance(row, dict) and set(row) == {'pass','phase','variant_index','world_seconds','label'} and
                row.get('pass') is True and isinstance(row.get('label'), str) for row in rows) and
            [row['label'] for row in rows] == labels, 'Portability JSON assertion stream differs')
    for row in rows:
        require(all(type(row[key]) in (int, float) and math.isfinite(row[key]) for key in ('phase','variant_index','world_seconds')) and
                row['phase'] == int(row['phase']) and 0 <= row['phase'] <= 6 and row['variant_index'] == int(row['variant_index']) and
                0 <= row['variant_index'] <= 3 and row['world_seconds'] >= 0, 'Malformed portability assertion coordinates')
    log_rows = re.findall(r'\bONE07_PORTABILITY\s+(PASS|FAIL)\s*\|\s*([^\r\n]*)', log)
    require(log_rows == [('PASS', label) for label in labels], 'Portability engine assertion stream differs')
    fields = ('frame,world_seconds,phase,variant_index,variant,actor_id,state,health,player_health,actor_x,actor_y,actor_z,'
              'pelvis_x,pelvis_y,pelvis_z,speed,live,kills,points,eligible_deaths,fall_count,recovery_count,contact_count,'
              'physics_bodies,awake_bodies,attack_contacts,attack_damage,foot_cues,attack_cues,hit_cues,fall_cues,body_cues,'
              'death_cues,active_voices,head,left_arm,right_arm,left_leg,rebase_cm').split(',')
    with (folder/'observations.csv').open(encoding='utf-8-sig', newline='') as stream:
        reader = csv.DictReader(stream)
        require(reader.fieldnames == fields, 'Portability timeline schema differs')
        timeline = list(reader)
    require(len(timeline) == done['observed_frames'], 'Portability timeline row count differs; no rows may be discarded')
    previous_frame, previous_time = -1, -math.inf
    phases, states, identities = [set() for _ in variants], [set() for _ in variants], [set() for _ in variants]
    for row in timeline:
        require(set(row) == set(fields) and all(value is not None for value in row.values()), 'Malformed portability timeline row')
        numbers = {key: float(value) for key, value in row.items() if key != 'variant'}
        require(all(math.isfinite(value) for value in numbers.values()), 'Nonfinite portability observation')
        for key in ('frame', 'phase', 'variant_index', 'actor_id', 'state'):
            require(numbers[key] == int(numbers[key]), 'Noninteger portability identity: ' + key)
        index = int(numbers['variant_index'])
        require(0 <= index < 3 and row['variant'] == variants[index] and numbers['actor_id'] > 0,
                'Portability timeline does not bind the declared appearance')
        require(numbers['frame'] > previous_frame and numbers['world_seconds'] >= previous_time,
                'Portability timeline frame or game-time order differs')
        previous_frame, previous_time = numbers['frame'], numbers['world_seconds']
        phases[index].add(int(numbers['phase'])); states[index].add(int(numbers['state'])); identities[index].add(int(numbers['actor_id']))
    require(all({1,2,3,4,5} <= seen for seen in phases) and all({3,5,6} <= seen for seen in states),
            'Each appearance must have actual approach/fall/get-up/recovery/death timeline coverage')
    require(all(len(ids) == 1 for ids in identities) and len(set.union(*identities)) == 3,
            'Each appearance must retain one distinct actual actor identity')
    return {'variants': variants, 'observed_frames': len(timeline), 'report': record(folder/'checks.json'),
            'timeline': record(folder/'observations.csv')}

def capture_flag(check, candidate):
    require(candidate == '07' and check in ('physicality','encounter','portability','combat'),
            'Capture is limited to C07 physicality, encounter, portability and native combat; never profiles')
    return '-ONE06Capture' if check in ('portability','combat') else '-ONE07Capture'

def capture_transport(folder, mode, source, expected_frames, artifact_files, native_combat=False):
    """Read all original frames and PCM; never launch an encoder or rewrite inputs."""
    media = folder/'Media'
    args = SimpleNamespace(chapters=True, capture_kind='packaged' if mode == 'packaged' else 'editor-game',
                           render_mode='offscreen', source_revision=source,
                           source_state='exact-commit' if mode == 'packaged' else 'working-tree')
    _, _, _, summary, ledger = capture_tools.prepare(media, args)
    require((expected_frames is None if native_combat else summary['frames'] == expected_frames) and summary['dimensions'] == [1600,900],
            'Actual capture count or full viewport size differs from actor/runner')
    recorded = {row['path']:row for row in artifact_files}
    require(len(recorded) == len(artifact_files), 'Duplicate artifact path')
    for row in summary['input_files'] + ledger:
        expected = {'path':'Media/'+row['file'], 'bytes':row['bytes'], 'sha256':row['sha256']}
        require(recorded.get(expected['path']) == expected, 'Capture changed after raw artifact binding')
    result = {key:summary[key] for key in ('frames','dimensions','audio_duration_seconds','audio_sample_rate','audio_channels',
              'audio_sample_frames','source_wav_layout','ordered_frame_identity_sha256','actual_real_stop_tail_seconds',
              'recorded_audio_after_last_callback_seconds','first_frame_audio_seconds','last_frame_audio_seconds')}
    result.update(schema='one07.capture_transport.v1', status='PASS', path_base='fixture_folder',
                  native_input_evidence=False, performance_evidence=False, perceptual_audio_review=False)
    if native_combat:result['actor_frame_counter']='Not serialized by native ONE06_COMBAT_COMPLETE; actual common recorder metadata/CSV/images supply the count'
    for key, name in (('metadata','capture.json'),('frames_csv','frames.csv'),('audio','gameplay_master.wav')):
        result[key] = recorded['Media/'+name]
    return result

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('check', choices=('profile', 'physicality', 'encounter', 'portability',*regression.MODES))
    ap.add_argument('--mode', choices=('editor', 'packaged'), required=True)
    ap.add_argument('--candidate', choices=('07', '06'), default='07')
    ap.add_argument('--root', type=Path, required=True)
    ap.add_argument('--executable', type=Path, required=True)
    ap.add_argument('--package-root', type=Path)
    ap.add_argument('--build-proof', type=Path)
    ap.add_argument('--source', help='Required full source commit for packaged mode; editor records observed HEAD only')
    ap.add_argument('--enemies', type=int, choices=(1, 2, 6, 12, 18), default=6)
    ap.add_argument('--weapon', choices=('M4A1', 'Overcurrent', '870'), default='M4A1')
    ap.add_argument('--falls', choices=('none', 'mixed'), default='mixed')
    ap.add_argument('--rate', type=int, choices=(30,60,120), help='weapon05 only; real frame cap and fixture requested rate')
    ap.add_argument('--size', choices=('1280x720','1600x900'), help='UI only; six intrinsic screenshots at this actual viewport')
    ap.add_argument('--timeout', type=int, help='Wall seconds; bounded default by check, timed-out process left running')
    ap.add_argument('--capture', action='store_true', help='Record original viewport/master audio for C07 non-profile checks')
    args = ap.parse_args()
    root, executable = args.root.resolve(), args.executable.resolve()
    flag, actor_directory, marker, map_path = check_layout(args.check, args.candidate)
    require(args.rate is None or args.check=='weapon05','--rate is weapon05 only')
    require(args.size is None or args.check=='ui','--size is UI only')
    rate=args.rate or 120;size=args.size or '1600x900';width,height=map(int,size.split('x'))
    actor_directory=actor_directory.format(rate=rate,size=size)
    fixed=regression.MODES[args.check][3] if args.check in regression.MODES else False
    if args.timeout is None:args.timeout=regression.MODES[args.check][4] if args.check in regression.MODES else 240
    capture_argument = capture_flag(args.check,args.candidate) if args.capture else None
    require(30 <= args.timeout <= 600, 'Timeout must be between 30 and 600 seconds')
    require(args.candidate == '07' or (args.mode == 'packaged' and args.check == 'profile' and args.enemies in (6,12,18) and args.falls == 'none'),
            'Unchanged C06 baseline supports packaged profile counts 6/12/18 and no new fall interventions')
    require(args.mode != 'editor' or args.source is None, 'Editor WIP source_commit is null; omit --source')
    stamp = dt.datetime.now(dt.timezone.utc).strftime('%Y%m%dT%H%M%S_%f')
    folder = root / 'Saved/Candidate07/Runtime' / (stamp + '_' + args.check + '_' + uuid.uuid4().hex[:8])
    folder.mkdir(parents=True, exist_ok=False)
    result = {'schema': 'one07.private_runtime.v1', 'state': 'PREPARED', 'candidate': args.candidate,
              'check': args.check, 'mode': args.mode, 'source_commit': args.source if args.mode == 'packaged' else None,
              'root': str(root), 'executable': str(executable), 'runner': record(Path(__file__).resolve()),
              'requested': {'variants':'all_three_production_appearances'} if args.check == 'portability' else
                           {'weapon_variants':6,'headings':8,'cases':10,'projected':True} if args.check=='aim05' else
                           {'weapon_variants':6,'cases':13} if args.check=='combat' else
                           {'rate':rate} if args.check=='weapon05' else {'viewport':[width,height]} if args.check=='ui' else
                           {} if args.check in regression.MODES else
                           {'enemies': args.enemies, 'weapon': args.weapon, 'falls': args.falls},
              'settings': {'width':width, 'height':height, 'max_fps':rate, 'vsync':0, 'screen_percentage':100},
              'map': map_path,
              'recording': args.capture, 'capture_requested':args.capture, 'timeout_seconds': args.timeout,
              'sparse_ui_screenshots':args.check=='ui',
              'scope': 'One scripted run, no native input/audition or matched performance conclusion. Random streams are not fixed across runs. Raw paths/logs remain private.'}
    def save():
        (folder/'result.json').write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    save()
    started = time.monotonic()
    try:
        result['processes_before'] = processes()
        require(not result['processes_before'], 'Existing Unreal/ProjectONE process must finish before this run')
        before = source_snapshot(root)
        result['source_before'] = before
        result['observed_head'] = before['observed_head']
        if args.mode == 'packaged':
            require(args.source and re.fullmatch(r'[0-9a-f]{40}', args.source), 'Packaged mode needs full --source')
            require(before['observed_head'] == args.source and not before['git_status'], 'Packaged checkout must be clean at exact source')
            require(args.package_root and args.build_proof, 'Packaged mode needs --package-root and --build-proof')
            package = args.package_root.resolve(); proof_path = args.build_proof.resolve()
            require(not any(part.casefold() in {'users','onedrive','.codex'} for part in root.parts), 'Use a neutral final source checkout')
            proof = json.loads(readtext(proof_path))
            require(proof.get('schema') == 'one07.runtime_build.v1' and proof.get('status') == 'PASS' and proof.get('source_commit') == args.source,
                    'Build proof has wrong schema, status or source')
            require(proof.get('candidate', args.candidate) == args.candidate, 'Build proof identifies a different candidate')
            runtime = [record(package/name, package) for name in RUNTIME]
            require(sorted(proof.get('runtime_files', []), key=lambda r:r['path']) == sorted(runtime, key=lambda r:r['path']), 'Package differs from source-bound runtime proof')
            require(executable == package/'ProjectONE/Binaries/Win64/ProjectONE.exe', 'Launch the verified inner game executable so the waited process owns the full run')
            build_logs = proof.get('build_logs', [])
            require(build_logs, 'Build proof must bind its retained build logs')
            for row in build_logs:
                path = (root/row['path']).resolve()
                require(path.is_relative_to(root) and record(path, root) == row, 'Build log binding differs')
            result['build_proof'] = record(proof_path)
            result['build_proof_path'] = str(proof_path); result['package_root'] = str(package)
            result['runtime_files'] = runtime; saved = package/'ProjectONE/Saved'
            command = [str(executable)] + ([map_path] if args.check == 'portability' else [])
        else:
            package = None; saved = root/'Saved'
            result['runtime_files'] = [record(executable), record(root/'Binaries/Win64/UnrealEditor-ProjectONE.dll', root)]
            command = [str(executable), str(root/'ProjectONE.uproject'), map_path, '-game']
        result['saved_root'] = str(saved)
        result['prelaunch_source_runtime_verified'] = True
        roots = {'driver':saved/actor_directory}
        if args.check=='profile' and args.candidate=='07': roots['companion'] = saved/'Candidate07/Profile'
        require(not fixed or not roots['driver'].exists(),'Existing fixed actor output must be preserved before another invocation: '+actor_directory)
        existing = {key:set(path.iterdir()) if path.exists() else set() for key,path in roots.items()}
        command += ['-'+flag, '-d3d12', '-RenderOffScreen', '-windowed', f'-ResX={width}', f'-ResY={height}', '-ForceRes', '-unattended', '-nosplash',
                    '-ini:Engine:[Audio]:UnfocusedVolumeMultiplier=1.0', f'-ExecCmds=t.MaxFPS {rate},r.VSync 0,r.ScreenPercentage 100,t.MaxFPS,r.VSync,r.ScreenPercentage', '-abslog='+str(folder/'engine.log')]
        if args.check=='profile':
            command += [f'-ONE05ProfileEnemies={args.enemies}', f'-ONE05ProfileWeapon={args.weapon}']
            if args.candidate=='07': command += ['-ONE07ProfileFalls='+args.falls]
        if args.check=='combat':command += ['-ONE06CombatVariants=6','-ONE03InputTrace']
        if args.check=='aim05':command += ['-ONE05AimVariants=6','-ONE05AimHeadings=8','-ONE05ProjectedAim']
        if args.check=='weapon05':command += [f'-ONE05Rate={rate}']
        if capture_argument: command.append(capture_argument)
        result['command'] = command; result['state'] = 'RUNNING'; save()
        with (folder/'process.log').open('wb') as output:
            process = subprocess.Popen(command, cwd=package or root, stdout=output, stderr=subprocess.STDOUT, creationflags=subprocess.CREATE_NO_WINDOW)
            result['pid'] = process.pid; save()
            print(json.dumps({'state':'RUNNING','pid':process.pid,'result':str(folder/'result.json')}), flush=True)
            try:
                result['exit_code'] = process.wait(timeout=args.timeout)
            except subprocess.TimeoutExpired:
                result.update(state='TIMEOUT_INSPECT_EXISTING_PROCESS', process_left_running=True)
                save(); return 124
        result['processes_after'] = processes()
        # Bind outputs before evaluating their success, so failure remains reviewable.
        result['artifacts'] = {}
        for key, path in roots.items():
            created = ([path] if fixed and key=='driver' else [p for p in path.iterdir() if p.is_dir() and p not in existing[key]]) if path.exists() else []
            result['artifacts'][key] = [{'folder':str(p), 'files':[record(f,p) for f in sorted(p.rglob('*')) if f.is_file()]} for p in created]
        result['source_after'] = source_snapshot(root)
        require(same_runtime_inputs(before,result['source_after'],args.mode), 'Source/content/HEAD changed during runtime')
        after_runtime = [record(package/name,package) for name in RUNTIME] if package else [record(executable),record(root/'Binaries/Win64/UnrealEditor-ProjectONE.dll',root)]
        require(after_runtime == result['runtime_files'], 'Runtime binaries changed during execution')
        if package: require(record(proof_path) == result['build_proof'], 'Build proof changed during execution')
        result['inputs_unchanged'] = True
        result['postrun_source_runtime_verified'] = True
        require(result['exit_code']==0, 'Process exited nonzero')
        log = readtext(folder/'engine.log'); result['engine_log'] = record(folder/'engine.log')
        require(all(len(rows)==1 for rows in result['artifacts'].values()), 'Expected one newly completed folder per actor')
        driver = Path(result['artifacts']['driver'][0]['folder'])
        result['fixture_folder'] = driver.relative_to(saved).as_posix()
        result['completion'] = completion(log, marker, allow_failures=True)
        if args.capture:
            result['capture'] = capture_transport(driver,args.mode,result['source_commit'],result['completion'].get('frames'),
                                                  result['artifacts']['driver'][0]['files'],native_combat=args.check=='combat')
            if args.check=='combat':
                require(re.findall(r'ONE06_CAPTURE_COMPLETE frames=(\d+)',log)==[str(result['capture']['frames'])],
                        'Native combat common-recorder completion differs from actual frame count')
        require(not re.search(r'\bFAIL\s*\||Fatal error:|Assertion failed:|Ensure condition failed:|Result=\{Fail\}',log), 'Engine log reports a failure/ensure')
        for name, expected in (('t.MaxFPS',rate),('r.VSync',0),('r.ScreenPercentage',100)):
            values = re.findall(re.escape(name)+r'\s*=\s*"?(-?[\d.]+)',log)
            require(values and all(float(v)==expected for v in values), 'Actual settings readback differs: '+name)
        result['completion'] = completion(log,marker)
        done=result['completion']; driver=Path(result['artifacts']['driver'][0]['folder'])
        if args.check in regression.MODES:
            result['regression']=regression.validate(args.check,driver,done,log,rate,size)
            result['assertions']={'driver':result['regression'].pop('ordered_assertion_labels')}
        else:
            validate_fixture_completion(done, args.check, args.capture)
            result['assertions'] = {'driver':assertions(driver/'checks.txt',done['checks'])}
        if args.check == 'portability':
            result['portability'] = validate_portability(driver, done, log, result['assertions']['driver'],args.capture)
        if args.check=='profile':
            require(len(list((driver/'CSV').glob('*.csv')))+len(list((driver/'CSV').glob('*.csv.gz')))==1, 'Missing/ambiguous finalized engine CSV')
            if args.candidate=='07':
                result['companion_completion']=completion(log,'ONE07_PROFILE_COMPLETE')
                require(result['companion_completion'].get('complete')==1,'Companion did not observe complete CSV')
                companion=Path(result['artifacts']['companion'][0]['folder'])
                result['assertions']['companion']=assertions(companion/'checks.txt',result['companion_completion']['checks'])
        if not args.capture and args.check!='ui':
            require(not any(Path(r['path']).suffix.lower() in {'.png','.jpg','.jpeg','.wav','.mp4'} for rows in result['artifacts'].values() for artifact in rows for r in artifact['files']), 'Recording-free check generated media')
        result['state']='PASS'
    except Exception as error:
        result.update(state='FAILED', error=str(error))
    finally:
        result['wall_seconds']=time.monotonic()-started
        for name in ('engine.log','process.log'):
            if (folder/name).is_file(): result[name.replace('.','_')]=record(folder/name)
        save()
    print(json.dumps({'state':result['state'],'result':str(folder/'result.json')}),flush=True)
    return 0 if result['state']=='PASS' else 1

if __name__=='__main__':
    raise SystemExit(main())
