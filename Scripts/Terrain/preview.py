import os, sys
import numpy as np
from PIL import Image, ImageDraw
WORK = r"E:\TerrainSource\work"
src = sys.argv[1] if len(sys.argv) > 1 else "game_raw_4033.npy"
out = sys.argv[2] if len(sys.argv) > 2 else "game_raw_preview.png"
z = np.load(os.path.join(WORK, src))
s = z[::2, ::2]
gy, gx = np.gradient(s, 2.0)
slope = np.arctan(np.hypot(gx, gy)); aspect = np.arctan2(-gx, gy)
a, b = np.radians(360 - 315 + 90), np.radians(45)
hs = np.clip(np.sin(b)*np.cos(slope) + np.cos(b)*np.sin(slope)*np.cos(a-aspect), 0, 1)
sea = s < 0.5
e = np.clip(s/190.0, 0, 1)
r = np.where(sea, 0.12, 0.30 + 0.5*hs*(0.7+0.3*e)); g = np.where(sea, 0.28, 0.40 + 0.5*hs); bb = np.where(sea, 0.55, 0.25 + 0.45*hs)
img = Image.fromarray((np.stack([r, g, bb], -1)*255).astype(np.uint8))
d = ImageDraw.Draw(img)
W = img.width
for m in range(-2000, 2001, 250):
    p = int((m + 2016) / 2)
    col = (255, 255, 0) if m % 1000 == 0 else (120, 120, 60)
    d.line([(p, 0), (p, W)], fill=col); d.line([(0, p), (W, p)], fill=col)
for m in range(-2000, 2001, 500):
    p = int((m + 2016) / 2)
    d.text((p + 2, 2), f"y{m}", fill=(255, 255, 255)); d.text((2, W - p - 12), f"x{m}", fill=(255, 255, 255))
img.save(os.path.join(WORK, out))
