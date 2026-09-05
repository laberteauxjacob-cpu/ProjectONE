"""Author original rounded HUD glyph artwork and panel masks. Python3 + Pillow.

Every path below is authored here. No operating-system or third-party font is
read, traced or redistributed. Editable SVG sheets accompany the PNG atlases.
Weapon images are separately rendered from original assembled Project ONE art.
"""
from pathlib import Path
import hashlib
import json
import math
import re
from PIL import Image, ImageDraw

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'ArtSource/UI/Candidate05'
NUMERALS={
 '0':'M32 9 C8 9 8 85 32 85 C56 85 56 9 32 9 Z',
 '1':'M19 24 Q30 21 34 10 L34 84 M19 84 L50 84',
 '2':'M12 27 C15 4 52 4 53 27 C56 45 14 56 12 83 L54 83',
 '3':'M13 18 C31 3 54 11 52 29 Q49 43 31 44 C56 43 61 71 44 81 Q29 89 12 78',
 '4':'M44 84 L44 10 L10 60 L57 60',
 '5':'M52 11 L16 11 L14 44 C33 33 54 40 54 63 C55 86 27 93 11 76',
 '6':'M49 13 C22 0 7 40 12 68 C17 94 54 91 54 66 C54 43 17 40 12 65',
 '7':'M11 12 L55 12 C40 35 31 58 27 84',
 '8':'M32 45 C8 44 5 10 31 10 C58 10 55 44 32 45 C0 47 5 86 32 85 C62 84 59 47 32 45 Z',
 '9':'M16 80 C41 94 58 58 53 29 C48 3 12 7 12 32 C12 55 48 59 53 33',
}
LETTERS={
 'A':'M8 82 L32 10 L56 82 M16 58 L48 58',
 'B':'M12 82 L12 10 L34 10 C62 10 60 44 33 44 L12 44 M33 44 C65 44 64 82 34 82 L12 82',
 'C':'M54 22 C43 0 10 8 10 46 C10 84 43 93 54 72',
 'D':'M12 10 L30 10 C65 10 65 82 30 82 L12 82 Z',
 'E':'M54 10 L12 10 L12 82 L54 82 M12 44 L46 44',
 'F':'M13 82 L13 10 L55 10 M13 44 L46 44',
 'G':'M53 22 C41 0 9 9 9 46 C9 86 55 94 55 58 L55 48 L34 48',
 'H':'M11 10 L11 82 M53 10 L53 82 M11 45 L53 45',
 'I':'M16 10 L48 10 M32 10 L32 82 M16 82 L48 82',
 'J':'M17 10 L52 10 L52 62 C52 88 12 92 10 67',
 'K':'M12 10 L12 82 M53 10 L13 49 L54 82',
 'L':'M13 10 L13 82 L55 82',
 'M':'M8 82 L8 10 L32 51 L56 10 L56 82',
 'N':'M11 82 L11 10 L53 82 L53 10',
 'O':'M32 9 C1 9 1 83 32 83 C63 83 63 9 32 9 Z',
 'P':'M13 82 L13 10 L35 10 C65 10 65 46 35 46 L13 46',
 'Q':'M32 9 C1 9 1 83 32 83 C63 83 63 9 32 9 Z M36 65 L58 88',
 'R':'M13 82 L13 10 L35 10 C65 10 63 46 35 46 L13 46 M32 46 L57 82',
 'S':'M53 20 C39 0 10 8 10 28 C10 51 53 43 54 64 C56 86 25 94 10 73',
 'T':'M6 10 L58 10 M32 10 L32 82',
 'U':'M11 10 L11 61 C11 92 53 92 53 61 L53 10',
 'V':'M8 10 L32 83 L56 10',
 'W':'M6 10 L17 83 L32 45 L47 83 L58 10',
 'X':'M9 10 L55 82 M55 10 L9 82',
 'Y':'M8 10 L32 44 L56 10 M32 44 L32 82',
 'Z':'M10 10 L54 10 L10 82 L54 82',
}
PUNCT={
 ' ':'', '.':'M32 81 L32 82', ',':'M34 78 L29 90', ':':'M32 29 L32 30 M32 69 L32 70',
 ';':'M32 29 L32 30 M34 70 L29 84', '!':'M32 12 L32 57 M32 80 L32 82',
 '?':'M12 25 C12 4 53 3 53 27 C53 43 32 43 32 57 M32 80 L32 82',
 '-':'M15 47 L49 47', '+':'M11 46 L53 46 M32 24 L32 68', '/':'M13 86 L51 8',
 '\\':'M13 8 L51 86', '=':'M13 34 L51 34 M13 59 L51 59',
 '(':'M43 8 C14 24 14 71 43 88', ')':'M21 8 C50 24 50 71 21 88',
 '[':'M43 8 L22 8 L22 88 L43 88', ']':'M21 8 L42 8 L42 88 L21 88',
 '<':'M49 16 L15 47 L49 78', '>':'M15 16 L49 47 L15 78',
 '%':'M10 82 L54 10 M19 11 C5 11 5 34 19 34 C33 34 33 11 19 11 M45 60 C31 60 31 83 45 83 C59 83 59 60 45 60',
 "'":'M32 10 L29 26', '"':'M23 10 L21 26 M43 10 L41 26',
 '_':'M10 87 L54 87', '#':'M23 13 L17 80 M47 13 L41 80 M9 34 L56 34 M7 60 L54 60',
 '*':'M32 21 L32 68 M10 32 L54 57 M10 57 L54 32', '|':'M32 9 L32 87',
 '$':'M52 23 C39 8 13 13 13 30 C13 49 51 43 51 64 C51 82 23 90 11 73 M32 7 L32 91',
 '&':'M53 80 L19 40 C1 17 31 0 40 17 C51 37 9 48 10 66 C12 89 46 91 55 53',
 '@':'M54 75 C36 98 6 84 6 49 C6 12 60 3 59 43 L57 63 C45 80 41 64 46 34 M44 36 C21 23 14 68 31 66 Q43 65 46 37',
 '^':'M13 34 L32 11 L51 34', '`':'M25 10 L35 24',
 '{':'M47 8 C21 5 33 34 26 40 L16 46 L26 52 C33 58 21 89 47 87',
 '}':'M17 8 C43 5 31 34 38 40 L48 46 L38 52 C31 58 43 89 17 87',
 '~':'M10 48 C24 24 40 71 54 43',
}


def flatten(path):
    tokens=re.findall(r'[A-Z]|-?\d+(?:\.\d+)?',path); paths=[]; current=[]; point=(0.,0.); begin=(0.,0.); i=0
    while i<len(tokens):
        command=tokens[i]; i+=1
        if command=='Z':
            current.append(begin); point=begin; continue
        count={'M':2,'L':2,'Q':4,'C':6}[command]; values=list(map(float,tokens[i:i+count])); i+=count
        if command=='M':
            if current: paths.append(current)
            point=tuple(values); begin=point; current=[point]
        elif command=='L': point=tuple(values); current.append(point)
        else:
            start=point; end=tuple(values[-2:])
            for step in range(1,25):
                t=step/24; u=1-t
                if command=='Q': current.append((u*u*start[0]+2*u*t*values[0]+t*t*end[0],u*u*start[1]+2*u*t*values[1]+t*t*end[1]))
                else: current.append((u**3*start[0]+3*u*u*t*values[0]+3*u*t*t*values[2]+t**3*end[0],u**3*start[1]+3*u*u*t*values[1]+3*u*t*t*values[3]+t**3*end[1]))
            point=end
    if current: paths.append(current)
    return paths


def draw_path(image,path,box,stroke,lower=False):
    draw=ImageDraw.Draw(image); x,y,w,h=box; supersample=4
    for points in flatten(path):
        points=[(x+(px+8)/80*w,y+((py*.72+25) if lower else py)/100*h) for px,py in points]
        width=stroke/100*h
        draw.line(points,fill=(255,255,255,255),width=round(width),joint='curve')
        for px,py in points:
            draw.ellipse((px-width/2,py-width/2,px+width/2,py+width/2),fill=(255,255,255,255))


def main():
    OUT.mkdir(parents=True,exist_ok=True); scale=4
    digits=Image.new('RGBA',(1280*scale,160*scale))
    number_svg=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 800 100">']
    for index,(char,path) in enumerate(NUMERALS.items()):
        draw_path(digits,path,(index*128*scale,0,128*scale,160*scale),13)
        number_svg.append(f'<g transform="translate({index*80+8} 0)"><path d="{path}" fill="none" stroke="white" stroke-width="13" stroke-linecap="round" stroke-linejoin="round"/></g>')
    number_svg.append('</svg>'); (OUT/'UI05_Numerals.svg').write_text('\n'.join(number_svg)+'\n',encoding='utf-8')
    digits.resize((1280,160),Image.Resampling.LANCZOS).save(OUT/'T_UI05Numerals.png')
    glyphs=Image.new('RGBA',(1024*scale,576*scale)); svg=['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1280 600">']
    for index,code in enumerate(range(32,128)):
        char=chr(code); lower=char.islower(); path=LETTERS.get(char.upper(),NUMERALS.get(char,PUNCT.get(char,PUNCT['?'])))
        if char==' ': path=''
        draw_path(glyphs,path,((index%16)*64*scale,(index//16)*96*scale,64*scale,96*scale),8,lower)
        transform=f'translate({(index%16)*80+8} {(index//16)*100})'+(' translate(0 25) scale(1 .72)' if lower else '')
        svg.append(f'<g transform="{transform}"><path d="{path}" fill="none" stroke="white" stroke-width="8" stroke-linecap="round" stroke-linejoin="round"/></g>')
    svg.append('</svg>'); (OUT/'UI05_Glyphs.svg').write_text('\n'.join(svg)+'\n',encoding='utf-8')
    glyphs.resize((1024,576),Image.Resampling.LANCZOS).save(OUT/'T_UI05Glyphs.png')
    panel=Image.new('RGBA',(128*scale,128*scale)); ImageDraw.Draw(panel).rounded_rectangle((0,0,128*scale-1,128*scale-1),radius=22*scale,fill='white')
    panel.resize((128,128),Image.Resampling.LANCZOS).save(OUT/'T_UI05Panel.png')
    manifest={'candidate':'05','generator':'Scripts/create_candidate05_ui.py','original_artwork':True,
        'font_provenance':'All numeric/alphabet/punctuation paths authored in this script. No installed font or third-party font input.',
        'numerals':{'file':'T_UI05Numerals.png','width':1280,'height':160,'columns':10,'cell':[128,160]},
        'glyphs':{'file':'T_UI05Glyphs.png','width':1024,'height':576,'columns':16,'rows':6,'first_ascii':32,'last_ascii':127,'cell':[64,96]},
        'palette':{'points':'#FFD54A','white':'#F8F2D9','teal':'#55D6C6','background':'#111820'},'files':[]}
    for name in ('T_UI05Numerals.png','T_UI05Glyphs.png','T_UI05Panel.png','UI05_Numerals.svg','UI05_Glyphs.svg'):
        path=OUT/name
        manifest['files'].append({'source':path.relative_to(ROOT).as_posix(),'bytes':path.stat().st_size,'sha256':hashlib.sha256(path.read_bytes()).hexdigest()})
    (OUT/'ui_inventory.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    print('CANDIDATE05_ORIGINAL_UI_ART_GENERATED 3 atlases/masks + 2 editable SVG sheets')


if __name__=='__main__': main()
