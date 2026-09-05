"""Verify original C05 UI payloads; optionally reconcile proven metadata-only sanitation.

Use --metadata-report Saved/PublicationAudit/Metadata_.../report.json after the
portable sanitizer. It accepts only verified PNG metadata or Blender metadata
edits, requires the old inventory hash to match the report's original hash,
and never renders, edits pixels or changes a Blender scene.
"""
from pathlib import Path
import argparse, hashlib, json

ROOT=Path(__file__).resolve().parents[1]
SOURCE=ROOT/'ArtSource/UI/Candidate05'

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--metadata-report',type=Path)
    args=parser.parse_args()
    icon_path=SOURCE/'weapon_icons.json'
    icons=json.loads(icon_path.read_text())
    art=json.loads((SOURCE/'ui_inventory.json').read_text())
    if args.metadata_report:
        report_path=(ROOT/args.metadata_report).resolve()
        assert report_path.is_relative_to((ROOT/'Saved/PublicationAudit').resolve())
        report=json.loads(report_path.read_text())
        assert report['applied']
        rows={row['path']:row for row in report['files']}
        expected={row['source'] for row in icons['images'].values()}|{'ArtSource/UI/Candidate05/WeaponIconStudio.blend'}
        assert set(rows)==expected, 'Use a report limited to these seven UI source files'
        for row in icons['images'].values():
            edit=rows[row['source']]
            assert row['sha256'] in (edit['original_sha256'],edit['sanitized_sha256'])
            assert sha(ROOT/row['source'])==edit['sanitized_sha256']
            assert edit['format']=='PNG' and edit['all_nontext_chunks_identical']
            row['sha256']=edit['sanitized_sha256']
        blend=rows['ArtSource/UI/Candidate05/WeaponIconStudio.blend']
        assert blend['decoded_nonmetadata_bytes_identical'] and sha(SOURCE/'WeaponIconStudio.blend')==blend['sanitized_sha256']
        icons['editable_studio']={'source':'ArtSource/UI/Candidate05/WeaponIconStudio.blend','sha256':blend['sanitized_sha256']}
        icon_path.write_text(json.dumps(icons,indent=2)+'\n',encoding='utf-8')
    records=art['files']+list(icons['images'].values())
    for row in records:
        path=(ROOT/row['source']).resolve()
        assert path.is_relative_to(SOURCE.resolve()) and sha(path)==row['sha256'], row['source']
    assert sha(ROOT/icons['source_blend'])==icons['source_blend_sha256']
    if 'editable_studio' in icons:
        row=icons['editable_studio']; assert sha(ROOT/row['source'])==row['sha256']
    assert len([row for row in records if row['source'].endswith('.png')])==9
    print('CANDIDATE05_UI_SOURCE_AUDIT_PASS: nine textures, two original SVG atlases, actual assembled weapon source')

if __name__=='__main__': main()
