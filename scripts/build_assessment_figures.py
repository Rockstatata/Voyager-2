"""Build reproducible, source-backed diagrams and runtime comparison plates.

Diagrams explain the current generators; photographs of rendered objects are
always taken from the application. No generated illustration impersonates a
runtime screenshot. See docs/report/figures/visual_manifest.json for provenance.
"""
import csv
import hashlib
import io
import json
import math
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageOps

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs/report/figures/technical"
OUT.mkdir(parents=True, exist_ok=True)
INK, BLUE, TEAL, RED, PALE = "#14283b", "#0f4d92", "#167f82", "#b64342", "#edf3f6"
MANIFEST = []

def refresh_base_figures():
    selections={
        'jupiter_encounter':'tour/03_jupiter_approach','saturn_encounter':'tour/04_saturn_approach',
        'overview':'tour/01_overview','controls':'tour/12_help','dish':'raytrace/voyager_dish_raster',
        'voyager':'raytrace/voyager_inspect_raster','voyager_traced':'raytrace/voyager_inspect_traced',
        'saturn_traced':'raytrace/saturn_traced','moon_maps':'shading/maps_moon_on',
        'moon_no_maps':'shading/maps_moon_off','spotlight':'shading/light_headlamp','fill':'shading/light_fill',
        'saturn_shadow':'shading/shadow_saturn_soft','saturn_no_shadow':'shading/shadow_saturn_off',
        **{'earth_'+mode:'shading/earth_'+mode for mode in ['flat','gouraud','phong','blinn_phong','toon']}}
    provenance=[]
    for name,relative in selections.items():
        source=ROOT/'captures/assessment'/(relative+'.bmp')
        with Image.open(source) as im:
            im.convert('RGB').save(OUT.parent/(name+'.jpg'),quality=93,subsampling=0,optimize=True)
        provenance.append({'figure':name+'.jpg','runtime_source':str(source.relative_to(ROOT)),
                           'source_sha256':hashlib.sha256(source.read_bytes()).hexdigest()})
    (OUT.parent/'provenance.json').write_text(json.dumps(provenance,indent=2)+'\n',encoding='utf-8')

def font(size=32, bold=False):
    return ImageFont.truetype("C:/Windows/Fonts/" + ("arialbd.ttf" if bold else "arial.ttf"), size)

def save(im, name, sources=(), kind="construction diagram"):
    path = OUT / (name + ".png")
    im.save(path, optimize=True)
    MANIFEST.append({"figure": str(path.relative_to(ROOT)), "kind": kind,
                     "sources": [{"path": str(Path(p).relative_to(ROOT)),
                                  "sha256": hashlib.sha256(Path(p).read_bytes()).hexdigest()} for p in sources]})
    return path

def screenshot(path, crop=True):
    im = Image.open(path).convert("RGB")
    if crop:
        # Remove telemetry only, keeping the full horizontal field of view.
        im = im.crop((0, 165, im.width, im.height - 115))
    return im

def plate(name, entries, columns=3, cell=(640, 440), crop=True, crop_box=None, label_size=25):
    cw, ch = cell
    rows = math.ceil(len(entries)/columns)
    im = Image.new("RGB", (cw*columns, ch*rows), "white")
    d = ImageDraw.Draw(im)
    sources = []
    for k, (label, path) in enumerate(entries):
        sources.append(path)
        x, y = (k % columns)*cw, (k//columns)*ch
        pic = screenshot(path, crop)
        if crop_box:pic=pic.crop(crop_box)
        pic = ImageOps.contain(pic, (cw-20, ch-75), Image.Resampling.LANCZOS)
        im.paste(pic, (x+(cw-pic.width)//2, y+8+(ch-75-pic.height)//2))
        label_font = font(label_size, True)
        while d.textbbox((0,0),label,font=label_font)[2] > cw-24:
            label_font = font(label_font.size-1, True)
        d.text((x+cw/2, y+ch-42), label, font=label_font, fill=INK, anchor="mm")
    return save(im, name, sources, "labelled runtime screenshot comparison")

def canvas(name, w=1600, h=820):
    im = Image.new("RGB", (w, h), "white")
    return im, ImageDraw.Draw(im)

def txt(d, xy, text, size=30, bold=False, color=INK, anchor=None):
    d.text(xy, text, font=font(size,bold), fill=color, anchor=anchor, spacing=10)

def arrow(d, a, b, color=TEAL, width=5):
    d.line([a,b], fill=color,width=width)
    t=math.atan2(b[1]-a[1],b[0]-a[0]); r=17
    d.polygon([b,(b[0]-r*math.cos(t-.45),b[1]-r*math.sin(t-.45)),
               (b[0]-r*math.cos(t+.45),b[1]-r*math.sin(t+.45))], fill=color)

def topology():
    im,d=canvas("topology",1600,650)
    txt(d,(30,25),"ONE INDEXED CELL  /  the same connectivity drives curved surfaces",34,True)
    p=[(110,170),(510,170),(110,490),(510,490)]
    d.polygon([p[0],p[1],p[2]],fill="#d7e7f4")
    d.polygon([p[1],p[3],p[2]],fill="#d9efeb")
    for a,b in [(0,1),(1,3),(3,2),(2,0),(1,2)]:d.line([p[a],p[b]],fill=BLUE,width=6)
    for xy,label in zip(p,["TL = i*65+j","TR = TL+1","BL = TL+65","BR = BL+1"]):
        d.ellipse((xy[0]-8,xy[1]-8,xy[0]+8,xy[1]+8),fill=RED)
        txt(d,(xy[0],xy[1]-42 if xy[1]<300 else xy[1]+38),label,25,True,anchor="mm")
    txt(d,(650,140),"Vertex record = position + normal + UV",32,True)
    txt(d,(650,205),"8 floats = 32 bytes; locations 0, 1, 2",28)
    txt(d,(650,275),"Triangle A:  (TL, TR, BL)",32,True,color=BLUE)
    txt(d,(650,335),"Triangle B:  (TR, BR, BL)",32,True,color=TEAL)
    txt(d,(650,415),"The EBO stores integer references to the VBO.\nEach consecutive index triple becomes a triangle.\nCCW winding selects the outward-facing side.",28)
    txt(d,(40,607),"Sphere poles: omit the triangle with coincident pole vertices. Seam: duplicate position, distinct u=0 / u=1.",25)
    save(im,"index_cell",[ROOT/"src/rendering/UvSphereGenerator.cpp"])

def primitives():
    im,d=canvas("primitives",1800,660)
    # Isometric hard-edged box, labelled face split.
    p=[(70,220),(260,130),(430,230),(240,320),(70,430),(240,525),(430,435)]
    for face,col in [([0,1,2,3],"#dceaf5"),([0,3,5,4],"#bcd4e7"),([3,2,6,5],"#90b4d0")]:
        d.polygon([p[i] for i in face],fill=col,outline=BLUE,width=4)
    d.line([p[0],p[2]],fill=RED,width=4);d.line([p[3],p[6]],fill=RED,width=4)
    txt(d,(260,45),"BOX / HARD EDGES",30,True,anchor="mm")
    txt(d,(260,585),"24 vertices / 12 triangles",28,True,anchor="mm")
    # cylinder and seam-inclusive rings.
    cx=800; y0=200;y1=470;rx=175;ry=55
    d.rectangle((cx-rx,y0,cx+rx,y1),fill="#deeeeb")
    d.ellipse((cx-rx,y1-ry,cx+rx,y1+ry),fill="#afd4ce",outline=TEAL,width=4)
    d.ellipse((cx-rx,y0-ry,cx+rx,y0+ry),fill="#e5f3f0",outline=TEAL,width=4)
    for i in range(10):
        th=2*math.pi*i/10
        x=cx+rx*math.cos(th); y=y0+ry*math.sin(th)
        if math.sin(th)>=0:d.line([(x,y),(x,y+(y1-y0))],fill=TEAL,width=3)
        d.line([(cx,y0),(x,y)],fill=TEAL,width=2)
    txt(d,(800,45),"CYLINDER / FRUSTUM",30,True,anchor="mm")
    txt(d,(800,585),"4n+6 vertices / 4n triangles",28,True,anchor="mm")
    # exact dish cross section six radial rings and normals.
    pts=[]
    for i in range(151):
        x=-1+2*i/150;pts.append((1370+x*195,435-175*x*x))
    d.line(pts,fill=BLUE,width=7)
    d.line([(x,y+20) for x,y in pts],fill="#8aaac2",width=5)
    for i in range(-6,7):
        x=i/6;px=1370+x*195;py=435-175*x*x
        d.ellipse((px-6,py-6,px+6,py+6),fill=RED)
        if i in [-4,0,4]:arrow(d,(px,py),(px+x*65,py-50),TEAL,4)
    txt(d,(1370,45),"PARABOLIC DISH",30,True,anchor="mm")
    txt(d,(1370,170),"48 angular segments / 6 radial rings",25,anchor="mm")
    txt(d,(1370,510),"y = -D + D(r/R)^2",30,True,anchor="mm")
    txt(d,(1370,585),"590 vertices / 1,152 triangles",28,True,anchor="mm")
    save(im,"primitive_profiles",[ROOT/"src/rendering/BoxGenerator.cpp",ROOT/"src/rendering/CylinderGenerator.cpp",
                                  ROOT/"src/rendering/ParabolicDishGenerator.cpp",ROOT/"src/scene/VoyagerModelBuilder.cpp"])

def ring_star_comet():
    im,d=canvas("ring",1800,610)
    cx,cy=290,270
    for r in [105,200]:d.ellipse((cx-r,cy-r*.5,cx+r,cy+r*.5),outline=BLUE,width=4)
    for i in range(16):
        th=2*math.pi*i/16;a=(cx+105*math.cos(th),cy+52.5*math.sin(th));b=(cx+200*math.cos(th),cy+100*math.sin(th))
        d.line([a,b],fill=TEAL,width=3)
        t=th+2*math.pi/16;c=(cx+105*math.cos(t),cy+52.5*math.sin(t))
        d.line([b,c],fill="#a6bdca",width=2)
    txt(d,(290,35),"ANNULUS / TWO SIDES",30,True,anchor="mm")
    txt(d,(290,460),"inner + outer seam-inclusive rings\nSeparate ±Y normals; reversed back winding\n64 segments: 260 vertices / 256 triangles",25,anchor="mm")
    rng=np.random.default_rng(1)
    for xy in rng.uniform([670,140],[1110,380],(55,2)):d.ellipse((*xy,xy[0]+5,xy[1]+5),fill=BLUE)
    txt(d,(900,35),"POINTS / LINES",30,True,anchor="mm")
    d.ellipse((720,175,1080,360),outline=TEAL,width=4)
    txt(d,(900,460),"7,380 star vertices -> GL_POINTS\n96 circle vertices -> GL_LINE_LOOP\nThese primitives contain zero triangles",25,anchor="mm")
    txt(d,(1480,35),"COMET / COMPOSITE",30,True,anchor="mm")
    d.ellipse((1280,230,1360,310),fill="#a6dcd8",outline=TEAL,width=4)
    d.polygon([(1340,225),(1680,270),(1340,315)],fill="#d9efeb",outline=TEAL,width=4)
    arrow(d,(1250,270),(1190,270),RED)
    txt(d,(1230,180),"Sun",25,color=RED,anchor="mm")
    txt(d,(1480,460),"Shared sphere nucleus + glow coma\n16-segment cone: 52 vertices / 32 triangles\nTail axis follows position - Sun",25,anchor="mm")
    save(im,"environment_topology",[ROOT/"src/rendering/RingGenerator.cpp",ROOT/"src/rendering/StarfieldGenerator.cpp",ROOT/"src/scene/Comet.cpp"])

def transform_diagram():
    im,d=canvas("transform",1600,620)
    boxes=[(40,105,490,235,"PLANET FRAME","T(orbit) · R(tilt) · S(radius)"),
           (860,35,1550,190,"SURFACE ONLY","parent · R(spin): texture rotates"),
           (860,245,1550,400,"MOON / RINGS","parent · local: inherit tilt, not spin"),
           (40,425,1550,565,"CAMERA-RELATIVE RENDER","subtract eye in double precision -> float -> view -> projection -> raster / trace")]
    for x0,y0,x1,y1,title,body in boxes:
        d.rounded_rectangle((x0,y0,x1,y1),18,fill=PALE,outline=BLUE,width=3)
        txt(d,((x0+x1)/2,y0+35),title,28,True,anchor="mm")
        txt(d,((x0+x1)/2,y0+93),body,26,anchor="mm")
    arrow(d,(490,150),(860,113));arrow(d,(490,185),(860,323));arrow(d,(1200,400),(1200,425))
    txt(d,(50,310),"Mworld = Mparent · T · R · S\nNormal = normalize((M3x3^-1)^T n)",32,True)
    save(im,"transform_hierarchy",[ROOT/"src/scene/CelestialBody.cpp",ROOT/"src/rendering/Renderer.cpp"])

def ray_diagram():
    im,d=canvas("rays",1600,670)
    eye=(100,410);hit=(640,355);sun=(950,90)
    d.ellipse((535,250,815,530),fill="#d7e7f4",outline=BLUE,width=4)
    d.ellipse((910,50,990,130),fill="#f6d995",outline="#bf8c24",width=4)
    arrow(d,eye,hit,BLUE);arrow(d,hit,sun,TEAL);arrow(d,hit,(360,110),RED)
    arrow(d,hit,(575,270),INK,4)
    txt(d,(40,470),"Eye / primary ray",28,True,color=BLUE)
    txt(d,(690,190),"shadow ray -> Sun\nvisibility / eclipse",27,color=TEAL)
    txt(d,(190,60),"reflection ray\none bounce (F10)",27,color=RED)
    txt(d,(470,260),"n",29,True)
    txt(d,(1050,130),"NEAREST HIT",31,True)
    txt(d,(1050,205),"Sphere: quadratic roots\nRing: plane + radius interval\nVoyager: triangles in BVH",28)
    txt(d,(1050,385),"Then evaluate texture,\nnormal and Blinn-Phong.\nTranslucent rings continue\nthrough up to four layers.",28)
    txt(d,(60,590),"Raster view: shadow rays only. F9: primary rays replace body / spacecraft geometry; environment stays rasterized.",25,True)
    save(im,"ray_paths",[ROOT/"shaders/raytrace.frag",ROOT/"shaders/raytrace.glsl"])

def bvh_diagram():
    im,d=canvas("bvh",1600,630)
    for box in [(60,100,760,420),(90,130,400,380),(425,160,720,365)]:d.rectangle(box,outline=BLUE,width=4)
    for pts in [[(120,330),(300,160),(370,330)],[(455,210),(690,310),(500,345)]]:
        d.polygon(pts,fill="#d7e7f4",outline=TEAL,width=4)
    arrow(d,(10,355),(770,230),RED)
    txt(d,(410,45),"SLAB TEST REJECTS EMPTY BOXES",28,True,anchor="mm")
    txt(d,(950,110),"ROOT AABB",30,True)
    arrow(d,(1070,160),(935,240));arrow(d,(1070,160),(1270,240))
    txt(d,(875,265),"LEFT BOX",27,True);txt(d,(1210,265),"RIGHT BOX",27,True)
    txt(d,(860,345),"Near child first; closest t prunes far boxes.\nLeaves: up to 4 triangles.\n12-bin surface-area heuristic split.",28)
    txt(d,(45,505),"5,828 triangles / 6,723 nodes / depth 24 / 32-entry traversal stack",32,True)
    txt(d,(45,565),"CPU builds once. GPU texture buffers store nodes, vertices, normals, UVs and material indices.",27)
    save(im,"bvh_layout",[ROOT/"src/rendering/TriangleBvh.cpp",ROOT/"shaders/raytrace_mesh.glsl"])

def plots():
    plt.rcParams.update({"font.family":["Arial","DejaVu Sans"],"font.size":16,"axes.spines.top":False,
                         "axes.spines.right":False,"axes.linewidth":1.5,"legend.frameon":False,"svg.fonttype":"none"})
    fig,ax=plt.subplots(1,2,figsize=(13,3.5))
    dist=np.linspace(0,150,400)
    ax[0].plot(dist,1/(.3+.7*(dist/20)**2),color=BLUE,lw=2.5)
    ax[0].set(xlabel="Rendered distance to Sun",ylabel="Attenuation",ylim=(0,3.5),title="F7: compressed inverse-square falloff")
    a=np.linspace(0,24,300);f=np.clip((np.cos(np.deg2rad(a))-math.cos(math.radians(18)))/(math.cos(math.radians(12))-math.cos(math.radians(18))),0,1)
    ax[1].plot(a,f,color=TEAL,lw=2.5);ax[1].axvline(12,color="gray",ls="--");ax[1].axvline(18,color="gray",ls="--")
    ax[1].set(xlabel="Angle from spotlight axis (degrees)",ylabel="Cone factor",ylim=(-.05,1.1),title="F5: inner 12° / outer 18°")
    fig.tight_layout(pad=1.2)
    for ext in ["png","pdf","svg"]:fig.savefig(OUT/f"light_response.{ext}",dpi=300)
    plt.close(fig)
    light_source=ROOT/'src/core/LightingController.cpp'
    MANIFEST.append({'figure':str((OUT/'light_response.png').relative_to(ROOT)),
                     'kind':'equation response plot; PNG/PDF/SVG exports',
                     'sources':[{'path':str(light_source.relative_to(ROOT)),
                                 'sha256':hashlib.sha256(light_source.read_bytes()).hexdigest()}]})
    rows=[]
    for line in (ROOT/"docs/report/validation/benchmark.txt").read_text().splitlines():
        p=line.split()
        if len(p)==6 and p[0]!="view":rows.append(p)
    fig,ax=plt.subplots(figsize=(12,3.5))
    x=np.arange(len(rows));v=[float(p[1]) for p in rows]
    ax.bar(x,v,color=[BLUE]*6+[TEAL]*2,edgecolor=INK)
    ax.set_xticks(x,[p[0].replace("_","\n") for p in rows],fontsize=11)
    ax.set(ylabel="Mean frame time (ms)",ylim=(0,max(v)*1.24))
    for i,y in enumerate(v):ax.text(i,y+.08,f"{y:.2f}",ha="center",fontsize=13)
    fig.tight_layout(pad=1.2)
    for ext in ["png","pdf","svg"]:fig.savefig(OUT/f"performance.{ext}",dpi=300)
    plt.close(fig)
    benchmark_source=ROOT/'docs/report/validation/benchmark.txt'
    MANIFEST.append({'figure':str((OUT/'performance.png').relative_to(ROOT)),
                     'kind':'measured frame-time plot; PNG/PDF/SVG exports',
                     'sources':[{'path':str(benchmark_source.relative_to(ROOT)),
                                 'sha256':hashlib.sha256(benchmark_source.read_bytes()).hexdigest()}]})

def maps():
    source=ROOT/"assets/textures/bodies/earth.jpg"
    albedo=Image.open(source).convert("RGB").resize((1024,512))
    a=np.asarray(albedo,dtype=float)/255
    height=a@np.array([.2126,.7152,.0722])
    padded=np.pad(height,((1,1),(0,0)),mode="edge")
    padded=np.pad(padded,((0,0),(1,1)),mode="wrap")
    dx=padded[:-2,2:]+2*padded[1:-1,2:]+padded[2:,2:]-padded[:-2,:-2]-2*padded[1:-1,:-2]-padded[2:,:-2]
    dy=padded[2:,:-2]+2*padded[2:,1:-1]+padded[2:,2:]-padded[:-2,:-2]-2*padded[:-2,1:-1]-padded[:-2,2:]
    n=np.stack([-dx*1.2,-dy*1.2,np.ones_like(dx)],axis=-1);n/=np.linalg.norm(n,axis=-1,keepdims=True)
    normal=Image.fromarray(np.uint8(np.clip((n*.5+.5)*255+.5,0,255)))
    water=(a[:,:,2]>a[:,:,0]*1.25)&(a[:,:,2]>a[:,:,1]*1.02)&(a.sum(axis=-1)<1.5)
    mask=Image.fromarray(np.where(water,255,30).astype('uint8')).convert('RGB')
    im=Image.new("RGB",(1800,410),"white");d=ImageDraw.Draw(im)
    for i,(pic,label) in enumerate([(albedo,"Albedo / RGB colour"),(normal,"Sobel-derived normal / approximation"),(mask,"Blue-dominant ocean specular mask")]):
        im.paste(pic.resize((580,290)),(i*600+10,10));txt(d,(i*600+300,345),label,24,True,anchor="mm")
    save(im,"texture_channels",[source,ROOT/"src/rendering/SurfaceMaps.cpp"],"derived explanatory map plate; resized albedo, not exported runtime texture")

def build():
    MANIFEST.clear()
    refresh_base_figures()
    overview_source=ROOT/"captures/assessment/tour/01_overview.bmp"
    # Remove the HUD bands, retaining every labelled planet in this overview.
    overview=Image.open(overview_source).convert("RGB").crop((0,210,1440,850))
    save(overview,"report_overview",[overview_source],"runtime overview; telemetry-only crop")
    topology();primitives();ring_star_comet();transform_diagram();ray_diagram();bvh_diagram();plots();maps()
    tech=ROOT/"captures/assessment/technical";shade=ROOT/"captures/assessment/shading";trace=ROOT/"captures/assessment/raytrace"
    plate("lighting",[("Sun point only",tech/"moon_point.bmp"),("Point + camera spotlight",tech/"moon_spot.bmp"),("Point + directional fill",tech/"moon_directional.bmp")])
    plate("report_lighting",[("Sun point only",tech/"moon_point.bmp"),("+ camera spotlight",tech/"moon_spot.bmp"),("+ directional fill",tech/"moon_directional.bmp")],label_size=42)
    plate("moving_spot",[("Frame 1",tech/"spot_move_0.bmp"),("Frame 2",tech/"spot_move_1.bmp"),("Frame 3",tech/"spot_move_2.bmp")])
    plate("spin",[("Spin / initial",tech/"earth_spin_0.bmp"),("Spin / later",tech/"earth_spin_1.bmp"),("Spin / later still",tech/"earth_spin_2.bmp")])
    plate("moon_motion",[("Jupiter / initial",tech/"jupiter_motion_0.bmp"),("Moons / later",tech/"jupiter_motion_1.bmp"),("Moons / later still",tech/"jupiter_motion_2.bmp")])
    names=[("Flat","flat"),("Gouraud","gouraud"),("Phong","phong"),("Blinn-Phong","blinn_phong"),("Toon","toon")]
    plate("shading",[(a,shade/f"earth_{b}.bmp") for a,b in names],5,(440,365))
    # Print needs larger individual examples than the deck's five-wide strip.
    plate("report_shading",[(a,shade/f"earth_{b}.bmp") for a,b in names],3,(640,470),True,(470,45,1010,585),42)
    plate("shading_voyager",[(a,tech/f"voyager_clean_{b}.bmp") for a,b in names],5,(440,365),False,(450,280,900,680))
    plate("shadows",[(a,shade/f"shadow_saturn_{b}.bmp") for a,b in [("Off","off"),("Hard","hard"),("Soft","soft")]])
    plate("tracing",[(a,trace/b) for a,b in [("Saturn / raster","saturn_raster.bmp"),("Saturn / traced","saturn_traced.bmp"),("Voyager / traced BVH","voyager_inspect_traced.bmp")]])
    plate("surface_maps",[("Moon / maps off",shade/"maps_moon_off.bmp"),("Moon / maps on",shade/"maps_moon_on.bmp")],2,(780,520))
    plate("report_surface_maps",[("Moon / maps off",shade/"maps_moon_off.bmp"),("Moon / maps on",shade/"maps_moon_on.bmp")],2,(780,520),True,(390,0,1050,615),36)
    plate("environment",[(a,tech/b) for a,b in [("Dynamic comet","comet.bmp"),("Asteroid belt / 4,000","asteroid_field.bmp"),("Kuiper belt / 3,000","kuiper_field.bmp"),("Oort shell / 1,500","oort_field.bmp")]],4,(520,410))
    bodydir=ROOT/"captures/assessment/bodies"
    lines=[line for line in (ROOT/"assets/data/celestial_bodies.csv").read_text().splitlines() if line and not line.startswith('#')]
    bodies=list(csv.DictReader(lines))
    plate("body_catalog",[(b['name'],bodydir/(b['id']+'.bmp')) for b in bodies],7,(380,290))
    # Existing component captures are still exact for the unchanged model.
    voyagerdir=ROOT/"docs/objects/images/voyager"
    shots=sorted(voyagerdir.glob('voyager_*.jpg'))
    plate("voyager_catalog",[(p.stem.split('_',2)[-1].replace('_',' '),p) for p in shots[1:]],6,(450,355))
    (OUT/"visual_manifest.json").write_text(json.dumps(MANIFEST,indent=2)+'\n',encoding='utf-8')
    # Matplotlib emits harmless line-end spaces; keep generated vectors clean
    # for repository whitespace checks without altering their geometry.
    for path in OUT.glob("*.svg"):
        path.write_text('\n'.join(line.rstrip() for line in path.read_text(encoding='utf-8').splitlines())+'\n',encoding='utf-8')
    print(f"[FIGURES] {len(MANIFEST)} source-backed diagram / screenshot plates")

if __name__=='__main__':build()
