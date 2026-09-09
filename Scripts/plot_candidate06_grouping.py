"""Plot actual ONE06GroupingCheck wall contacts. Requires matplotlib.

Input is a completed grouping directory; raw local paths are not written into
the report. A failed or incomplete gameplay check cannot produce accepted art.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--input', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    folder, output = args.input, args.output
    assert not output.exists(), 'Preserve an existing plot directory.'
    checks = list(csv.DictReader((folder/'checks.csv').open(encoding='utf-8-sig', newline='')))
    meta = json.loads((folder/'observations.json').read_text(encoding='utf-8-sig'))
    assert checks and all(row['pass'] == '1' for row in checks), 'Grouping checks must all pass.'
    assert meta['complete'] is True, 'Grouping must complete.'
    groups = list(csv.DictReader((folder/'groups.csv').open(encoding='utf-8-sig', newline='')))
    rays = [row for row in csv.DictReader((folder/'rays.csv').open(encoding='utf-8-sig', newline=''))
            if row['row_kind'] == 'projectile']
    patterns = ('M4A1_short_3', 'M4A1_sustained_12', '870_three_shells')
    names = ('M4A1 · 3-shot burst', 'M4A1 · 12-shot burst', '870 · 3 shells / 24 pellets')
    distances = (300, 650, 1000)
    assert len(groups) == 18 and len(rays) == 234, 'Expected all 18 complete paired trials.'
    import matplotlib
    matplotlib.use('Agg')
    import matplotlib.pyplot as plt
    plt.rcParams.update({'font.size': 10, 'axes.spines.top': False, 'axes.spines.right': False})
    fig, axes = plt.subplots(3, 3, figsize=(12, 10), constrained_layout=True)
    measured = []
    for i, (pattern, name) in enumerate(zip(patterns, names)):
        for j, distance in enumerate(distances):
            ax = axes[i, j]
            paired = []
            for scene in (0, 1):
                rows = [r for r in rays if r['pattern'] == pattern and int(float(r['distance_cm'])) == distance and int(r['scene']) == scene]
                assert rows and all(r['wall_hit'] == '1' for r in rows)
                paired.append(rows)
            centre_y = sum(float(r['end_y']) for r in paired[0])/len(paired[0])
            centre_z = sum(float(r['end_z']) for r in paired[0])/len(paired[0])
            for scene, rows in enumerate(paired):
                ax.scatter([float(r['end_y'])-centre_y for r in rows], [float(r['end_z'])-centre_z for r in rows],
                           s=48 if scene == 0 else 29, marker='o' if scene == 0 else 'x',
                           facecolors='none' if scene == 0 else '#c45428', edgecolors='#237fa2' if scene == 0 else None,
                           linewidths=1.3, label='Wall only' if scene == 0 else 'With infected')
            pair_groups = [r for r in groups if r['pattern'] == pattern and int(float(r['distance_cm'])) == distance]
            g = next(r for r in pair_groups if r['scene'] == '0')
            ax.set_title(f'{name}\n{distance/100:g} m · span {float(g["diagonal_span"]):.2f} cm')
            ax.set_aspect('equal', adjustable='datalim')
            ax.grid(alpha=.2)
            ax.set_xlabel('Horizontal offset (cm)')
            if j == 0: ax.set_ylabel('Vertical offset (cm)')
            measured.append({key: g[key] for key in ('pattern','distance_cm','spread_seed','diagonal_span','first_spread','last_spread','recovered_bloom')})
    axes[0, 0].legend(loc='best', fontsize=8)
    fig.suptitle('Measured wall groups · identical seed and pose in each pair', fontsize=16)
    fig.supxlabel('Offsets use the wall-only group centre for both scenes. Panel scales vary; spans are Y/Z bounding-box diagonals.\nStanding targets at 1.8 m; actual game collision, finite spread and body penetration. Fixed fixture poses; not native input.', fontsize=10)
    output.mkdir(parents=True)
    fig.savefig(output/'grouping.png', dpi=160)
    fig.savefig(output/'grouping.svg')
    plt.close(fig)
    summary = {'schema':'one06.grouping_plot.v1','status':'PASS','checks':len(checks),'trials':18,
               'actual_projectiles':len(rays),'groups':measured,
               'inputs':[{'file':name,'sha256':hashlib.sha256((folder/name).read_bytes()).hexdigest()} for name in ('checks.csv','observations.json','groups.csv','rays.csv')],
               'method':'Actual projectile rows only. Contact rows excluded to avoid duplicated points. Shared empty-scene centre for each pair; variable panel scales. Not a fitted accuracy statistic.'}
    (output/'grouping.json').write_text(json.dumps(summary, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
