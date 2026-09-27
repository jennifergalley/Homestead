import os, sys
import numpy as np
from PIL import Image, ImageDraw
WORK = r"E:\TerrainSource\work"
src, out = sys.argv[1], sys.argv[2]
x0, x1, y0, y1 = map(float, sys.argv[3:7])   # game metres: x south..north, y west..east
step = int(sys.argv[7]) if len(sys.argv) > 7 else 1
z = np.load(os.path.join(WORK, src)); half = (z.shape[0]-1)/2
r0, r1 = int(half - x1), int(half - x0); c0, c1 = int(half + y0), int(half + y1)
s = z[r0:r1:step, c0:c1:step]
gy, gx = np.gradient(s, float(step))
slope = np.arctan(np.hypot(gx, gy)); aspect = np.arctan2(-gx, gy)
a, b = np.radians(135), np.radians(45)
hs = np.clip(np.sin(b)*np.cos(slope) + np.cos(b)*np.sin(slope)*np.cos(a-aspect), 0, 1)
sea = s < 0.5
r = np.where(sea, 0.12, 0.30 + 0.5*hs); g = np.where(sea, 0.28, 0.40 + 0.5*hs); bb = np.where(sea, 0.55, 0.25 + 0.45*hs)
rgb = np.stack([r, g, bb], -1)
cont = (np.floor(s/10) != np.floor(np.roll(s, 1, 0)/10)) | (np.floor(s/10) != np.floor(np.roll(s, 1, 1)/10))
rgb[cont & ~sea] = [0.45, 0.25, 0.1]
img = Image.fromarray((np.clip(rgb,0,1)*255).astype(np.uint8)); d = ImageDraw.Draw(img)
H, W = s.shape
for m in range(int(np.ceil(y0/100)*100), int(y1)+1, 100):
    p = (m - y0)/step; d.line([(p,0),(p,H)], fill=(255,255,0) if m % 500 == 0 else (90,90,40)); d.text((p+2,2), f"y{m}", fill=(255,255,255))
for m in range(int(np.ceil(x0/100)*100), int(x1)+1, 100):
    p = (x1 - m)/step; d.line([(0,p),(W,p)], fill=(255,255,0) if m % 500 == 0 else (90,90,40)); d.text((2,p+2), f"x{m}", fill=(255,255,255))
img.save(os.path.join(WORK, out)); print(img.size, "z range", np.nanmin(s), np.nanmax(s))

if len(sys.argv) > 8:
    import json
    L = json.load(open(sys.argv[8]))
    def P(x, y): return ((y - y0)/step, (x1 - x)/step)
    for key, col in (("road", (230,200,120)), ("river", (80,160,255)), ("estuary", (40,90,200))):
        d.line([P(*p) for p in L[key]], fill=col, width=1)
    for name, poly in L["polygons"].items():
        d.line([P(*p) for p in poly] + [P(*poly[0])], fill=(255,60,60) if name == "EstateBoundary" else (255,140,255), width=1)
    for name, v in L["landmarks"].items():
        px, py = P(v[0], v[1]); d.ellipse([px-3, py-3, px+3, py+3], outline=(255,255,255)); d.text((px+4, py-4), name, fill=(255,255,255))
    img.save(os.path.join(WORK, out))
