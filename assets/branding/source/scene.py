import math, sys, subprocess, os
FONT = "'Inter Display', 'Inter', sans-serif"

def rider(cx, cy, s, rot=-18, col="#0b1630"):
    """Original generic snowboarder silhouette mid-air, tucked with a board grab. Centred at cx,cy, scale s."""
    return f'''<g transform="translate({cx},{cy}) rotate({rot}) scale({s})" fill="{col}" stroke="{col}" stroke-linecap="round" stroke-linejoin="round">
      <rect x="-64" y="40" width="128" height="12" rx="6" stroke="none"/>
      <rect x="-30" y="34" width="14" height="9" rx="3" stroke="none"/>
      <rect x="16" y="34" width="14" height="9" rx="3" stroke="none"/>
      <path d="M-23 38 L-31 14 L-10 -2" fill="none" stroke-width="14"/>
      <path d="M23 38 L24 12 L8 -2" fill="none" stroke-width="14"/>
      <path d="M-15 0 L-21 -36 Q-5 -48 9 -38 L12 0 Z" stroke-width="6"/>
      <circle cx="-5" cy="-55" r="12.5" stroke="none"/>
      <path d="M-14 -55 Q-5 -71 10 -58" fill="none" stroke-width="5"/>
      <path d="M-15 -32 L-36 -40 L-56 -58" fill="none" stroke-width="8.5"/>
      <path d="M6 -30 L22 -8 L27 36" fill="none" stroke-width="8.5"/>
    </g>'''

def snowflake(cx, cy, r, col="#ffffff", w=None):
    w = w or r*0.11
    out=[f'<g stroke="{col}" stroke-linecap="round" fill="none" stroke-width="{w:.2f}">']
    for k in range(6):
        a=math.pi/2+k*math.pi/3
        x=cx+r*math.cos(a); y=cy+r*math.sin(a)
        out.append(f'<line x1="{cx}" y1="{cy}" x2="{x:.2f}" y2="{y:.2f}"/>')
        for rr,l in ((0.45,0.28),(0.75,0.2)):
            px=cx+r*rr*math.cos(a); py=cy+r*rr*math.sin(a)
            for sgn in (-1,1):
                b=a+sgn*math.pi/3
                out.append(f'<line x1="{px:.2f}" y1="{py:.2f}" x2="{px+r*l*math.cos(b):.2f}" y2="{py+r*l*math.sin(b):.2f}"/>')
    out.append(f'</g><circle cx="{cx}" cy="{cy}" r="{r*0.2:.2f}" fill="{col}"/>')
    return ''.join(out)

def emblem(cx, cy, r):
    return (f'<circle cx="{cx}" cy="{cy}" r="{r}" fill="url(#emb)"/>'
            f'<circle cx="{cx}" cy="{cy}" r="{r*0.9:.2f}" fill="none" stroke="#a9dcff" stroke-width="{r*0.045:.2f}"/>'
            + snowflake(cx, cy, r*0.66, "#f2f9ff"))

DEFS = '''<defs>
  <linearGradient id="sky" x1="0" y1="0" x2="0" y2="1">
    <stop offset="0" stop-color="#0a1a3f"/><stop offset="0.55" stop-color="#1f5d9e"/><stop offset="1" stop-color="#8fd3ff"/>
  </linearGradient>
  <radialGradient id="sun" cx="0.5" cy="0.5" r="0.5">
    <stop offset="0" stop-color="#fff6d8" stop-opacity="1"/><stop offset="0.35" stop-color="#ffd27a" stop-opacity=".55"/><stop offset="1" stop-color="#ff9b5a" stop-opacity="0"/>
  </radialGradient>
  <linearGradient id="mtnFar" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#6d97c9"/><stop offset="1" stop-color="#3f6ea8"/></linearGradient>
  <linearGradient id="mtnMid" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#e9f4ff"/><stop offset="0.45" stop-color="#9cc3ea"/><stop offset="1" stop-color="#2f5d97"/></linearGradient>
  <linearGradient id="slope" x1="0" y1="0" x2="1" y2="1"><stop offset="0" stop-color="#ffffff"/><stop offset="1" stop-color="#bfe0fb"/></linearGradient>
  <linearGradient id="emb" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#2a7bd6"/><stop offset="1" stop-color="#123f82"/></linearGradient>
  <linearGradient id="title" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#ffffff"/><stop offset="1" stop-color="#cfeaff"/></linearGradient>
  <filter id="shadow" x="-20%" y="-20%" width="140%" height="140%"><feDropShadow dx="0" dy="6" stdDeviation="8" flood-color="#04102a" flood-opacity=".55"/></filter>
</defs>'''

def landscape(W, H, sun=(0.72,0.30), rider_at=(0.5,0.42), rider_s=1.0, slope_top=0.70, show_rider=True):
    """Sky, sun, far + mid mountain ranges, foreground kicker slope and a rider in the air.
    Coordinates are fractions of W,H."""
    def ridge(base, amp, n, seed, w=W, h=H):
        import random
        rnd=random.Random(seed); pts=[(0,h)]
        x=0; step=w/n
        for i in range(n+1):
            y=h*base - h*amp*(0.35+0.65*rnd.random())*(1 if i%2 else 0.45)
            pts.append((i*step,y))
        pts.append((w,h))
        return ' '.join(f'{x:.1f},{y:.1f}' for x,y in pts)
    sx,sy=sun; rx,ry=rider_at
    parts=[f'<rect width="{W}" height="{H}" fill="url(#sky)"/>',
           f'<circle cx="{W*sx}" cy="{H*sy}" r="{min(W,H)*0.38}" fill="url(#sun)"/>',
           f'<polygon points="{ridge(0.66,0.30,9,3)}" fill="url(#mtnFar)" opacity=".85"/>',
           f'<polygon points="{ridge(0.76,0.34,6,11)}" fill="url(#mtnMid)"/>']
    # light snow particles
    import random
    rnd=random.Random(7)
    for i in range(int(W*H/9000)):
        x=rnd.random()*W; y=rnd.random()*H*0.8; r=rnd.random()*min(W,H)*0.004+0.6
        parts.append(f'<circle cx="{x:.1f}" cy="{y:.1f}" r="{r:.2f}" fill="#fff" opacity="{0.25+rnd.random()*0.5:.2f}"/>')
    # foreground slope with a kicker lip on the left
    t=slope_top
    parts.append(f'<path d="M0 {H*(t+0.08)} C {W*0.18} {H*(t+0.06)}, {W*0.28} {H*(t-0.02)}, {W*0.34} {H*(t-0.07)} L {W*0.36} {H*(t+0.03)} C {W*0.55} {H*(t+0.12)}, {W*0.8} {H*(t+0.10)}, {W} {H*(t+0.16)} L {W} {H} L 0 {H} Z" fill="url(#slope)"/>')
    parts.append(f'<path d="M{W*0.36} {H*(t+0.03)} C {W*0.55} {H*(t+0.12)}, {W*0.8} {H*(t+0.10)}, {W} {H*(t+0.16)}" fill="none" stroke="#9fcdf3" stroke-width="{max(2,H*0.004):.1f}" opacity=".7"/>')
    if show_rider:
        R=min(W,H)*0.0042*rider_s
        cx,cy=W*rx,H*ry
        # spin trail (nod to the charge meter): dashed arcs behind the rider
        for i,(rr,op) in enumerate(((95,.55),(125,.35),(155,.2))):
            parts.append(f'<path d="M{cx-rr*R*1.1:.1f} {cy+rr*R*0.15:.1f} A {rr*R:.1f} {rr*R*0.55:.1f} 0 0 1 {cx+rr*R*0.2:.1f} {cy-rr*R*0.55:.1f}" fill="none" stroke="#ffffff" stroke-width="{R*6:.1f}" stroke-linecap="round" opacity="{op}"/>')
        parts.append(f'<g filter="url(#shadow)">{rider(cx,cy,R,-18)}</g>')
    return ''.join(parts)

def title_block(cx, y, size, anchor="middle", sub=True, fill="url(#title)", stroke="#0b2a57"):
    sw=size*0.07
    t=(f'<g filter="url(#shadow)" font-family="{FONT}" font-weight="900" font-style="italic" text-anchor="{anchor}">'
       f'<text x="{cx}" y="{y}" font-size="{size}" letter-spacing="{-size*0.01:.1f}" fill="{fill}" stroke="{stroke}" stroke-width="{sw:.1f}" paint-order="stroke">COOL BOARDERS 2</text>')
    if sub:
        t+=(f'<text x="{cx}" y="{y+size*0.62:.1f}" font-size="{size*0.42:.1f}" letter-spacing="{size*0.12:.1f}" fill="#7fd0ff" stroke="{stroke}" stroke-width="{sw*0.6:.1f}" paint-order="stroke">RECOMPILED</text>')
    return t+'</g>'

def svg(W,H,body,bg=True):
    return f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">{DEFS}{body}</svg>'

def render(name, W, H, body, transparent=False):
    html=f'''<!doctype html><html><head><style>html,body{{margin:0;padding:0;background:{'transparent' if transparent else '#000'};overflow:hidden}}</style></head><body>{svg(W,H,body)}</body></html>'''
    open(name+'.html','w').write(html)
    args=['google-chrome','--headless=new','--no-sandbox','--disable-gpu','--hide-scrollbars',f'--window-size={W},{H}',
          f'--screenshot={os.path.abspath(name)}.png','--force-device-scale-factor=1']
    if transparent: args.append('--default-background-color=00000000')
    args.append('file://'+os.path.abspath(name+'.html'))
    subprocess.run(args,check=True,capture_output=True)
