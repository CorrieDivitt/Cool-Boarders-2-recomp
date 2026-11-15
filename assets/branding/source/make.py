from scene import *
from PIL import Image
out=os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'); os.makedirs(out,exist_ok=True)
def go(name,W,H,body,transparent=False,scale=1):
    render(f'{out}/{name}',W*scale,H*scale,f'<g transform="scale({scale})">{body}</g>' if scale!=1 else body,transparent)
    for f in (f'{out}/{name}.html',): os.remove(f)

def poster(W=600,H=900):
    b=landscape(W,H,sun=(0.70,0.36),rider_at=(0.58,0.50),rider_s=0.80,slope_top=0.74)
    b+=title_block(W/2,H*0.13,W*0.105)
    b+=emblem(W*0.5,H*0.93,W*0.055)
    return b
def hero(W=1920,H=620):   # backdrop only; launchers lay the logo over it
    return landscape(W,H,sun=(0.80,0.30),rider_at=(0.66,0.38),rider_s=0.95,slope_top=0.70)
def wide(W,H):            # header / wide tile / social card
    b=landscape(W,H,sun=(0.86,0.28),rider_at=(0.84,0.40),rider_s=0.85,slope_top=0.72)
    b+=title_block(W*0.05,H*0.38,H*0.135,anchor="start")
    return b
def logo(W=1280,H=400):   # transparent wordmark with the emblem
    r=H*0.30
    b=emblem(r*1.08,H*0.46,r)
    b+=title_block(r*2.35,H*0.50,H*0.235,anchor="start")
    return b
def icon(S=512):
    b=f'<rect x="{S*0.03}" y="{S*0.03}" width="{S*0.94}" height="{S*0.94}" rx="{S*0.22}" fill="url(#sky)"/>'
    b+=f'<clipPath id="ic"><rect x="{S*0.03}" y="{S*0.03}" width="{S*0.94}" height="{S*0.94}" rx="{S*0.22}"/></clipPath>'
    b+=f'<g clip-path="url(#ic)">'+landscape(S,S,sun=(0.72,0.30),rider_at=(0.53,0.47),rider_s=1.05,slope_top=0.72)+'</g>'
    return b

go('poster_600x900',600,900,poster())
go('poster_1200x1800',600,900,poster(),scale=2)
go('hero_1920x620',1920,620,hero())
go('hero_3840x1240',1920,620,hero(),scale=2)
go('header_920x430',920,430,wide(920,430))
go('wide_1280x640',1280,640,wide(1280,640))
go('logo_1280x400',1280,400,logo(),transparent=True)
go('logo_2560x800',1280,400,logo(),transparent=True,scale=2)
go('icon_512',512,512,icon(),transparent=True)
im=Image.open(f'{out}/icon_512.png').convert('RGBA')
im.resize((256,256),Image.LANCZOS).save(f'{out}/icon_256.png')
im.save(f'{out}/icon.ico',sizes=[(16,16),(32,32),(48,48),(64,64),(128,128),(256,256)])
for f in sorted(os.listdir(out)):
    p=f'{out}/{f}'
    if f.endswith('.png'): i=Image.open(p); print(f,i.size,i.mode)
