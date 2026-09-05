"""Repair Candidate04 evidence chapter metadata; preserve originals and encoded AV packets.

Run after full capture assembly and before comparison assembly. Explicitly rerun
for the comparison if its chapter readback has blank titles or dangling QT refs.
This is evidence-only postprocessing: no game, capture, or original WAV changes.
"""
from pathlib import Path
import argparse, datetime, hashlib, json, os, re, shutil, subprocess, uuid

ROOT=Path(__file__).resolve().parents[3]
FFMPEG=ROOT/'Saved/MediaTools/imageio_ffmpeg/binaries/ffmpeg-win-x86_64-v7.1.exe'

def require(ok,message):
    if not ok:raise ValueError(message)

def sha(path):
    h=hashlib.sha256()
    with path.open('rb') as f:
        for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
    return h.hexdigest()

def run(command,log):
    result=subprocess.run(command,capture_output=True)
    log.write_bytes(result.stderr)
    require(result.returncode==0,'FFmpeg failed; inspect private '+log.name)
    return result

def parse_chapters(raw):
    chapters=[]
    for line in raw.decode('utf8').splitlines():
        if line=='[CHAPTER]':chapters.append({})
        elif chapters and '=' in line:
            key,value=line.split('=',1);chapters[-1][key]=value
    result=[]
    for c in chapters:
        n,d=map(int,c['TIMEBASE'].split('/'))
        result.append({'title':c.get('title',''),'start_seconds':int(c['START'])*n/d,'end_seconds':int(c['END'])*n/d})
    return result

def stream_hashes(ff,path,log):
    r=run([str(ff),'-hide_banner','-nostdin','-i',str(path),'-map','0:v:0','-map','0:a:0','-c','copy','-f','streamhash','-'],log)
    rows=r.stdout.decode().splitlines()
    require(len(rows)==2 and all(re.fullmatch(r'[01],[va],SHA256=[a-f0-9]{64}',s) for s in rows),'Unexpected AV streamhash output')
    return rows

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--movie',type=Path,required=True)
    p.add_argument('--commit',required=True)
    p.add_argument('--ffmpeg',type=Path,default=FFMPEG)
    p.add_argument('--expected-chapters',type=int)
    a=p.parse_args()
    movie=a.movie.resolve();sidecar=movie.with_suffix('.json')
    require(movie.parent==(ROOT/'Evidence/Candidate04').resolve() and movie.name in ('progression_loop.mp4','weapon_comparison.mp4'),'Only the two explicit Candidate04 evidence movie paths are supported')
    for path in (movie,sidecar,*movie.parents):
        if os.path.lexists(path):require(not path.is_symlink() and not getattr(path.lstat(),'st_file_attributes',0) & 0x400,'Linked media path refused')
    raw=sidecar.read_bytes();report=json.loads(raw)
    require(re.fullmatch('[a-f0-9]{40}',a.commit) and report.get('source_revision')==a.commit and report.get('encoded') is True,'Exact source/encoded sidecar required')
    original_sha=sha(movie)
    require(original_sha==report['output_sha256'] and movie.stat().st_size==report['output_bytes'],'Movie differs from current sidecar')
    if movie.name=='progression_loop.mp4':
        chapters=report.get('chapters',[])
        expected_duration=report.get('audio_duration_seconds')
    else:
        require(report.get('source_binding_verified') is True,'Verified comparison source binding required')
        chapters=[{'title':c['title'],'start_seconds':c['output_start_seconds'],
                   'end_seconds':c['output_end_seconds']} for c in report.get('clips',[])]
        expected_duration=report.get('output_duration_seconds')
    require(isinstance(expected_duration,(int,float)) and expected_duration>0,'Positive output duration required')
    require(chapters and all(isinstance(c.get('start_seconds'),(int,float)) and isinstance(c.get('end_seconds'),(int,float))
            and 0<=c['start_seconds']<c['end_seconds']<=expected_duration+.0011 for c in chapters),'Invalid chapter intervals')
    require(abs(chapters[0]['start_seconds'])<=.0011 and abs(chapters[-1]['end_seconds']-expected_duration)<=.0011,'Chapters must cover output endpoints')
    require(all(abs(a['end_seconds']-b['start_seconds'])<=.0011 for a,b in zip(chapters,chapters[1:])),'Chapters must be ordered and contiguous')
    require(chapters and (a.expected_chapters is None or len(chapters)==a.expected_chapters),'Unexpected chapter count')
    require(all(isinstance(c.get('title'),str) and c['title'] and not any(ord(v)<32 for v in c['title']) for c in chapters),'Missing/control-character chapter titles')
    folder=ROOT/'Saved/Candidate04/Assembly'/('ChapterRemux_'+datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')+'_'+uuid.uuid4().hex[:8])
    folder.mkdir(parents=True,exist_ok=False)
    original=folder/'original_encoded.mp4'
    # Immutable private byte copy, not a hard link to the public movie.
    shutil.copy2(movie,original)
    require(sha(original)==original_sha,'Original backup copy mismatch')
    (folder/'original_sidecar.json').write_bytes(raw)
    meta=folder/'chapters.ffmetadata';lines=[';FFMETADATA1']
    for c in chapters:
        title=re.sub(r'([\\=;#])',r'\\\1',c['title'])
        lines.extend(['[CHAPTER]','TIMEBASE=1/1000000','START='+str(round(c['start_seconds']*1e6)),
                      'END='+str(round(c['end_seconds']*1e6)),'title='+title])
    meta.write_text('\n'.join(lines)+'\n',encoding='utf8')
    pending=folder/'verified_remux.mp4'
    command=[str(a.ffmpeg),'-hide_banner','-nostdin','-i',str(movie),'-i',str(meta),'-map','0:v:0','-map','0:a:0',
             '-c','copy','-map_metadata','-1','-map_metadata:s:v','-1','-map_metadata:s:a','-1','-map_chapters','1']
    for i,c in enumerate(chapters):command.extend(['-metadata:c:'+str(i),'title='+c['title']])
    command.extend(['-movflags','+faststart',str(pending)])
    run(command,folder/'remux.log')
    before=stream_hashes(a.ffmpeg,original,folder/'original_streamhash.log')
    after=stream_hashes(a.ffmpeg,pending,folder/'remux_streamhash.log')
    require(before==after,'Encoded AV packet payloads changed during stream-copy remux')
    readback=run([str(a.ffmpeg),'-hide_banner','-nostdin','-i',str(pending),'-map_metadata','0','-map_chapters','0','-f','ffmetadata','-'],folder/'readback.log')
    (folder/'chapters_readback.ffmetadata').write_bytes(readback.stdout)
    actual=parse_chapters(readback.stdout)
    require(len(actual)==len(chapters),'Container chapter count mismatch')
    error=0.
    for expected,observed in zip(chapters,actual):
        require(observed['title']==expected['title'],'Container chapter title mismatch')
        error=max(error,abs(observed['start_seconds']-expected['start_seconds']),abs(observed['end_seconds']-expected['end_seconds']))
    require(error<=.0011,'Chapter timing differs by more than MP4 millisecond rounding')
    require(b'Referenced QT chapter track not found' not in readback.stderr,'Missing chapter track remains')
    decoded=run([str(a.ffmpeg),'-hide_banner','-nostdin','-xerror','-i',str(pending),'-map','0:v:0','-map','0:a:0','-af','volumedetect','-f','null','NUL'],folder/'decode.log')
    text=decoded.stderr.decode('utf8','replace')
    require('Referenced QT chapter track not found' not in text,'Missing chapter track in full decode')
    peak=re.findall(r'max_volume: (-?\d+(?:\.\d+)?) dB',text)
    mean=re.findall(r'mean_volume: (-?\d+(?:\.\d+)?) dB',text)
    require(peak and mean and float(peak[-1])>-60,'Decoded output contains no measurable audio above -60dBFS')
    duration=re.search(r'Duration: (\d+):(\d+):(\d+(?:\.\d+)?)',readback.stderr.decode('utf8','replace'))
    require(duration is not None,'Container duration unavailable')
    seconds=int(duration[1])*3600+int(duration[2])*60+float(duration[3])
    require(abs(seconds-expected_duration)<=1/30+.011,'Container/audio duration mismatch')
    repair={'method':'FFmpeg stream-copy remux with explicit per-chapter title metadata; no AV re-encoding',
            'helper_repository_path':'Evidence/Candidate04/Tools/repair_movie_chapters.py',
            'helper_sha256':sha(Path(__file__)),
            'expected_output_duration_seconds':expected_duration,
            'original_encoded_sha256':original_sha,'encoded_video_audio_packet_hashes_unchanged':True,
            'av_stream_sha256':after,'chapter_count':len(actual),'all_chapter_titles_verified':True,
            'maximum_chapter_time_rounding_seconds':error,'missing_qt_chapter_track_warning':False,
            'full_av_decode_exit_code':0,'container_duration_seconds_rounded_to_10ms':seconds,
            'decoded_audio_peak_dbfs_rounded':float(peak[-1]),'decoded_audio_mean_dbfs_rounded':float(mean[-1]),
            'audio_audition_performed':False,'original_encoded_file_preserved_privately':True}
    final_sha=sha(pending);final_bytes=pending.stat().st_size
    require(sha(movie)==original_sha and sidecar.read_bytes()==raw,'Public output changed while verifying')
    private={'movie':str(movie),'source_revision':a.commit,'repair':repair,'verified_chapters':actual,
             'output_sha256':final_sha,'output_bytes':final_bytes,'original_backup':str(original)}
    (folder/'verification.json').write_text(json.dumps(private,indent=2)+'\n',encoding='utf8')
    # Only the explicit newly encoded evidence movie is replaced. Its exact
    # prior bytes and sidecar remain in the private run directory above.
    pending.replace(movie)
    report.update(output_sha256=final_sha,output_bytes=final_bytes,chapter_metadata_remux=repair)
    sidecar.write_text(json.dumps(report,indent=2)+'\n',encoding='utf8')
    print(json.dumps({'result':'CHAPTER_REMUX_AND_DECODE_PASS','chapters':len(actual),'movie_sha256':final_sha,
                      'movie_bytes':final_bytes,'private_verification':str(folder/'verification.json'),'repair':repair},indent=2))

if __name__=='__main__':main()
