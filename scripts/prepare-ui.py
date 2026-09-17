"""Bake local Vanilla fonts/menu artwork into a fixed, private Xbox UI atlas."""
import argparse,hashlib,json,struct,subprocess
from pathlib import Path
from workspace_paths import game_data
import PIL
from PIL import Image,ImageDraw,ImageFont,features

ROOT=Path(__file__).resolve().parents[1]
def prepare(data,output,tool):
    source=output.parent/'sources';source.mkdir(parents=True,exist_ok=True)
    inputs={}
    def extract(name,texture=False):
        target=source/(name.replace('\\','_')+('.tga' if texture else ''))
        if name in inputs:return target
        run=subprocess.run([str(tool),'--texture' if texture else '--extract',str(data),name,str(target)],capture_output=True,text=True)
        if run.returncode:raise RuntimeError(run.stdout+run.stderr)
        inputs[name]={'decoded_sha256':hashlib.sha256(target.read_bytes()).hexdigest(),'bytes':target.stat().st_size}
        return target
    fonts=[ImageFont.truetype(str(extract('Fonts\\FRIZQT__.TTF')),14),
           ImageFont.truetype(str(extract('Fonts\\FRIZQT__.TTF')),18),
           ImageFont.truetype(str(extract('Fonts\\MORPHEUS.TTF')),28)]
    images=[];glyphs=[];sprites=[]
    for face,font in enumerate(fonts):
        for code in range(32,127):
            char=chr(code);left,top,right,bottom=font.getbbox(char);w,h=right-left,bottom-top
            bitmap=Image.new('RGBA',(max(w,1),max(h,1)))
            ImageDraw.Draw(bitmap).text((-left,-top),char,font=font,fill=(255,255,255,255))
            item={'image':bitmap,'glyph':len(glyphs)};images.append(item)
            glyphs.append([code,face,0,0,w,h,left,top,round(font.getlength(char)*64),0])
    definitions=[('Glues-WoW-Logo',None,(256,128)),('Glue-Panel-Button-Up',(0,0,.578125,.75),None),
        ('Glue-Panel-Button-Down',(0,0,.578125,.75),None),('Glue-Panel-Button-Disabled',(0,0,.578125,.75),None),
        ('Glue-Tooltip-Background',None,None),('Glue-Tooltip-Border',None,None)]
    for index,(name,crop,size) in enumerate(definitions):
        with Image.open(extract('Interface\\Glues\\Common\\'+name+'.blp',True)) as decoded:bitmap=decoded.convert('RGBA')
        if crop:bitmap=bitmap.crop(tuple(round(v*(bitmap.width if i%2==0 else bitmap.height)) for i,v in enumerate(crop)))
        if size:bitmap=bitmap.resize(size,Image.Resampling.LANCZOS)
        if max(bitmap.size)>512:raise ValueError('UI sprite exceeds fixed atlas: '+name)
        sprites.append([index,0,0,*bitmap.size,0]);images.append({'image':bitmap,'sprite':index})
    sprites.append([6,0,0,3,3,0]);images.append({'image':Image.new('RGBA',(3,3),(255,255,255,255)),'sprite':6})
    # Vanilla PlayerFrame/TargetFrame share a texture. The Xbox renderer mirrors
    # the player copy; keep the original 232x100 FrameXML crop and bar gradient.
    for name,crop in (('UI-TargetingFrame',(24,0,256,100)),('UI-StatusBar',None),('UI-TargetingFrame-Skull',None)):
        with Image.open(extract('Interface\\TargetingFrame\\'+name+'.blp',True)) as decoded:bitmap=decoded.convert('RGBA')
        if crop:bitmap=bitmap.crop(crop)
        index=len(sprites);sprites.append([index,0,0,*bitmap.size,0]);images.append({'image':bitmap,'sprite':index})
    atlas=Image.new('RGBA',(512,512));x=y=row_height=0
    for item in sorted(images,key=lambda i:i['image'].height,reverse=True):
        bitmap=item['image'];w,h=bitmap.size
        if x+w+2>512:x=0;y+=row_height;row_height=0
        if y+h+2>512:raise ValueError('UI atlas full; do not silently enlarge the Xbox budget')
        atlas.paste(bitmap,(x+1,y+1))
        record=glyphs[item['glyph']] if 'glyph' in item else sprites[item['sprite']]
        offset=2 if 'glyph' in item else 1;record[offset:offset+2]=[x+1,y+1]
        x+=w+2;row_height=max(row_height,h+2)
    glyph_data=b''.join(struct.pack('<6H4h',*g) for g in glyphs);sprite_data=b''.join(struct.pack('<6H',*s) for s in sprites)
    pixels=bytearray(512*512*4)
    def morton(x,y):return sum(((x>>b)&1)<<(2*b)|((y>>b)&1)<<(2*b+1) for b in range(9))
    for i,(r,g,b,a) in enumerate(atlas.getdata()):struct.pack_into('<I',pixels,morton(i%512,i//512)*4,(a<<24)|(r<<16)|(g<<8)|b)
    pixel_offset=40+len(glyph_data)+len(sprite_data)
    header=struct.pack('<4s9I',b'WXU1',2,512,512,len(glyphs),len(sprites),40,40+len(glyph_data),pixel_offset,pixel_offset+len(pixels))
    payload=header+glyph_data+sprite_data+pixels;temporary=output.with_suffix('.tmp');temporary.write_bytes(payload);temporary.replace(output)
    atlas.save(output.with_suffix('.png'))
    manifest={'format':'WXU1','version':2,'sha256':hashlib.sha256(payload).hexdigest(),'bytes':len(payload),'atlas':[512,512],
        'glyphs':len(glyphs),'sprites':len(sprites),'pillow':PIL.__version__,'freetype':features.version_module('freetype2'),
        'assetc_sha256':hashlib.sha256(tool.read_bytes()).hexdigest(),'inputs':inputs,
        'scope':'Private assets decoded from the supplied Vanilla installation. English ASCII interface atlas; extended glyphs and additional screens remain.'}
    output.with_suffix('.json').write_text(json.dumps(manifest,indent=2)+'\n')
    print(json.dumps({k:v for k,v in manifest.items() if k!='inputs'},indent=2))
if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--data',type=Path,default=game_data())
    p.add_argument('--output',type=Path,default=ROOT/'build/ui-prepared/INTERFACE.WUI');p.add_argument('--assetc',type=Path,default=ROOT/'build/host/wowx_assetc.exe')
    a=p.parse_args();prepare(a.data,a.output,a.assetc)
