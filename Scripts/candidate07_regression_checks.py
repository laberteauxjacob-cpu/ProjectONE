"""Native legacy fixture schemas used unchanged inside a C07 runtime.

These checks bind original assertions to the engine log. They do not rename
the actor's candidate identity or establish visual/audio/performance approval.
"""
import math
from pathlib import Path
import re
import struct

import assemble_candidate06_capture as capture

MODES = {
    'combat': ('ONE06CombatCheck','ONE06_COMBAT_COMPLETE','Candidate06/CombatCheck',False,240),
    'grouping': ('ONE06GroupingCheck','ONE06_GROUPING_COMPLETE','Candidate06/GroupingCheck',False,240),
    'survival': ('ONE06SurvivalCheck','ONE06_SURVIVAL_COMPLETE','Candidate06/SurvivalCheck',False,240),
    'progression': ('ONE04ProgressionCheck','ONE04_PROGRESSION_COMPLETE','Candidate04/Progression',True,240),
    'aim05': ('ONE05AimCheck','ONE05_AIM_COMPLETE','Candidate05/AimCheck',False,480),
    'weapon05': ('ONE05WeaponCheck','ONE05_WEAPON_COMPLETE','Candidate05/Weapon{rate}',True,360),
    'ui': ('ONE05UICheck','ONE05_UI_COMPLETE','Candidate05/UI{size}',True,180),
}

def require(value,message):
    if not value: raise ValueError(message)

def rows(path,fields):
    result=capture.read_csv(path,set(fields))
    require(result,'Empty native fixture timeline: '+path.name)
    return result

def counter(report,key,value):
    require(type(report.get(key)) in (int,float) and report[key]==value,'Native report counter differs: '+key)

def ui_rows(report,folder,width,height):
    result=[]; actions=[]
    for row in report['assertions']:
        require(isinstance(row,dict),'Malformed UI record')
        if 'kind' not in row:
            require(set(row)=={'label','pass','stage','world_seconds'} and type(row['pass']) is bool,'Unknown UI assertion schema')
            result.append(('PASS' if row['pass'] else 'FAIL',row['label']))
            continue
        require(set(row)=={'kind','action','x','y','min_x','min_y','max_x','max_y'} and row['kind']=='drawn_button_target',
                'Unknown UI metadata, or metadata pretending to be an assertion')
        require(all(type(row[key]) in (int,float) and math.isfinite(row[key]) for key in row if key!='kind'),'Invalid UI target coordinate')
        require(row['action']==int(row['action']) and result and result[-1]==('PASS',f"Drawn action{int(row['action'])} has a nonempty in-viewport hit rectangle"),
                'UI target lacks its matching preceding assertion')
        require(0<=row['min_x']<row['max_x']<=width and 0<=row['min_y']<row['max_y']<=height and
                min(row['max_x']-row['min_x'],row['max_y']-row['min_y'])>5 and
                abs(row['x']-(row['min_x']+row['max_x'])/2)<=.001 and abs(row['y']-(row['min_y']+row['max_y'])/2)<=.001,
                'UI target is outside its actual viewport or not its drawn midpoint')
        actions.append(int(row['action']))
    require(actions==[8,16,1,2,3],'UI must exercise exactly GrantPoints, CloseTools, Resume, Restart and Quit')
    require(report.get('quit_requested_via_button') is True,'UI did not request its actual Quit action')
    frames=report.get('frames',[])
    require(len(frames)==6 and len({row.get('file') for row in frames})==6,'UI must retain six distinct original PNGs')
    names=set()
    for row in frames:
        require(row.get('width')==width and row.get('height')==height,'UI screenshot metadata dimensions differ')
        path=capture.local_input(folder,row['file']);names.add(path)
        with path.open('rb') as stream: header=stream.read(24)
        require(path.suffix=='.png' and header[:8]==b'\x89PNG\r\n\x1a\n' and header[12:16]==b'IHDR' and
                struct.unpack('>II',header[16:24])==(width,height),'Original PNG header dimensions differ; no resizing allowed')
    media={path.resolve() for path in folder.rglob('*') if path.suffix.lower() in {'.png','.jpg','.jpeg','.wav','.mp4'}}
    require(media==names,'UI media inventory differs from its six original screenshots')
    return result

def validate(check,folder,done,log,rate,size):
    flag,marker,_,_,_=MODES[check];prefix=marker.removesuffix('_COMPLETE')
    observed=re.findall(r'\b'+prefix+r'\s+(PASS|FAIL)\s*\|\s*([^\r\n]*)',log)
    require(len(observed)==done['checks'] and done['failures']==0 and all(state=='PASS' for state,_ in observed),
            'Native engine assertion stream disagrees with completion')
    if check in ('combat','aim05'):
        text=capture.read_record(folder/'checks.txt')
        actual=re.findall(r'^'+prefix+r'\s+(PASS|FAIL)\s*\|\s*([^\r\n]*)',text,re.M)
        timeline=rows(folder/'rays.csv',('variant','trial'))
        require({int(row['variant']) for row in timeline}==set(range(6)),'Actual projectile evidence lacks six weapon variants')
        if check=='combat':
            tested={tuple(map(int,match.groups())) for _,label in actual
                    if (match:=re.match(r'variant=(\d+) trial=(\d+) Actual trace awards exactly one impact plus eligible kill per distinct victim$',label))}
            require(tested=={(v,t) for v in range(6) for t in range(13)},'Combat did not validate all six weapons and thirteen collision/award scenes')
        else:
            require('Aim mode: unshaken logical viewport projection' in text,'Aim regression must use the ordinary projected height plane')
            require(len(timeline)==480 and {(int(r['variant']),int(r['heading']),int(r['case'])) for r in timeline}==
                    {(v,h*45,c) for v in range(6) for h in range(8) for c in range(10)},'Aim evidence must retain all 480 original discharges')
    elif check in ('grouping','survival'):
        require(done.get('complete')==1 and done.get('frames')==0,'Numeric regression did not finish or generated capture')
        text=capture.read_record(folder/'checks.txt')
        for key in ('complete','checks','failures','frames'):
            values=re.findall(r'^'+key.capitalize()+r': (\d+)\s*$',text,re.M)
            require(values==[str(done[key])],'Native text completion differs: '+key)
        timeline=rows(folder/'checks.csv',('index','world_seconds','pass','label'))
        require([int(row['index']) for row in timeline]==list(range(1,done['checks']+1)),'Native assertion order/count differs')
        actual=[]
        for row in timeline:
            require(row['pass'] in ('0','1'),'Invalid native assertion Boolean')
            label=row['label']
            if check=='grouping': label=f"{row['pattern']} distance={float(row['distance_cm']):.0f} scene={int(row['scene'])} {label}"
            actual.append(('PASS' if row['pass']=='1' else 'FAIL',label))
        if check=='grouping':
            require(done.get('trials')==18,'Grouping must finish all eighteen paired trials')
            rows(folder/'groups.csv',('pattern','distance_cm','scene'))
            rows(folder/'rays.csv',('pattern','distance_cm','scene'))
        capture.strict_json(folder/'observations.json')
    else:
        report=capture.strict_json(folder/'checks.json')
        for key in ('checks','failures'):counter(report,key,done[key])
        require(isinstance(report.get('assertions'),list),'Missing original JSON assertions')
        if check=='ui':
            width,height=map(int,size.split('x'))
            require((done.get('width'),done.get('height'))==(width,height),'Actual UI viewport differs')
            counter(report,'width',width);counter(report,'height',height)
            actual=ui_rows(report,folder,width,height)
            rows(folder/'input.csv',('world_seconds','frame','stage'))
        else:
            actual=[]
            for row in report['assertions']:
                require(isinstance(row,dict) and type(row.get('pass')) is bool and isinstance(row.get('label'),str),'Malformed native JSON assertion')
                actual.append(('PASS' if row['pass'] else 'FAIL',row['label']))
            timeline=rows(folder/'timeline.csv',('stage',))
            if check=='weapon05':
                require(done.get('rate')==rate,'Weapon fixture cadence cap differs');counter(report,'requested_fps',rate)
                require({int(row['variant']) for row in timeline}>=set(range(6)),'Cadence timeline lacks one of the six actual weapons')
    require(actual==observed,'Original native assertion stream differs from engine log; no assertions may be dropped')
    return {'native_fixture':flag,'checks':len(actual),'failures':0,'ordered_assertion_labels':[label for _,label in actual],
            'scope':'All original native actor assertions bound to engine completion/log. Raw timelines retained. UI PNG headers are validated without pixel review; no audio, native-input or performance claim.'}
