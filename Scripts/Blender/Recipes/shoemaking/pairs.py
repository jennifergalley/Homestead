"""The three pairs of footwear, built on each foot's last.

Construction notes (what we imitate):

* **Fur boots** (northern mukluk / Sami-style winter boot): one piece of sheepskin, hair side in,
  about 1.5 cm of wool loft on a 2 mm skin, so the outside is the suede flesh side. The top is
  turned out into a fur cuff. The sole is a separate thicker piece of smoked hide gathered up
  around the foot and whip-stitched to the upper about 2 cm above the ground, which puckers it at
  the toe and heel. Leather thongs wrap the ankle in a figure-eight over the instep and cinch the
  shaft below the cuff; the fur compresses under them.
* **Woven sandals** (Fort Rock sagebrush-bark sandals, ~9-10 ka; Japanese waraji): a flat sole
  about 1 cm thick of weft cords twined across a few warps, the outermost warp forming a corded
  rim. A toe cord from between the first and second toes splits over the forefoot to side loops;
  the ties cross the instep, pass rear side loops and a heel loop, wrap the ankle and are knotted
  in front.
* **Turnshoes** (the everyday medieval shoe, 10th-15th c.): one layer of ~2 mm vegetable-tanned
  calf or goat, sewn to a ~3.5 mm sole inside-out with a flesh/grain seam and then turned, so the
  sole seam hides inside and shows as a tight bead. Low-cut below the ankle bones, open over the
  instep with a tongue, and laced through awl-punched holes with a leather thong tied in front.

All parts are original geometry. Units meters, bind pose, the character faces -Y.
"""
import math

import numpy as np

from . import geo as G
from . import last as L

TAU = L.TAU

# part ids (texture kinds)
FUR_LINING, SUEDE, CUFF, THONG = 1, 2, 3, 4
TURN_OUTER, TURN_INNER, TURN_RIM, LACE = 10, 11, 12, 13
SANDAL_SOLE, CORD = 20, 21

FAR = 1.0      # "far from any opening" for the coverage-mask edge distance


def fbm(P, scale, seed=0, octaves=3):
    from outfit import textures as T
    return T.fbm3(np.asarray(P, float) / scale + seed * 13.1, octaves, seed) - 0.5


class Built:
    def __init__(self, name):
        self.name = name
        self.mb = G.MeshBuilder()
        self.info = {}
        self.feet = {}


def _alphas(n, extra=()):
    a = np.arange(n) * TAU / n
    if extra:
        a = np.unique(np.concatenate([a, np.mod(np.asarray(extra), TAU)]))
    return a


def _knot(mb, center, nrm, tang, part, width, thick, size=0.0085, turns=2, attrs=None):
    """An overhand knot: a trefoil of the thong/cord, pulled flat against the surface."""
    nrm = nrm / np.linalg.norm(nrm)
    tang = tang - nrm * (tang @ nrm)
    tang /= np.linalg.norm(tang)
    bi = np.cross(nrm, tang)
    t = np.linspace(0, TAU, 61)
    k = size / 3.2
    x = (np.sin(t) + 2 * np.sin(2 * t)) * k
    y = (np.cos(t) - 2 * np.cos(2 * t)) * k
    zz = -np.sin(3 * t) * k * 0.55 * (thick / max(width, thick)) * 2.2
    path = center[None] + x[:, None] * tang[None] + y[:, None] * bi[None] + (zz + k * 1.2)[:, None] * nrm[None]
    up = np.repeat(nrm[None], len(t), 0)
    G.add_tube(mb, path, up, G.ellipse_profile(10, width * 0.9, thick * 1.1), part, caps=False,
               attrs={kk: np.full(len(t), np.asarray(v).ravel()[0]) for kk, v in (attrs or {}).items()})


def _tail(mb, surf, start, direction_pts, part, profile, length_taper=0.0, offset=0.0012, attrs=None):
    pts, nrm = G.hug(surf, [start] + list(direction_pts), offset, step=0.002)
    n = len(pts)
    t = np.linspace(0, 1, n)
    sc = np.stack([1 - length_taper * t, np.ones(n)], 1)
    G.add_tube(mb, pts, nrm, profile, part, scale=sc, attrs=attrs)
    return pts


# =================================================================== fur boots

def fur_boots(V, T, surf, log=print, seed=0):
    b = Built("SKM_FurBoots")
    mb = b.mb
    FLOOR_IN, FLOOR_OUT = -0.0005, -0.0120
    Z_TOP = 0.335
    WALL = 0.0170                     # 15 mm wool loft on a 2 mm skin
    CUFF_T, CUFF_LEN = 0.0150, 0.064
    for side in "l":          # the right foot is its mirror image (body symmetric to 0.2 mm)
        last = L.Last(V, T, side, z_top=Z_TOP, log=log)
        Ph = last.hull_points()
        z = Ph[..., 2]
        g = 0.0055 + 0.0140 * L.smoothstep(z, 0.045, 0.092)
        rin, se_in = last.layer(g, FLOOR_IN, 0.008, round_k=0.006, extra_smooth=(last.s_f0 + 0.035, 70))
        target = lambda P: np.where(P[:, 2] > 0.085, 0.0190, 0.004 + 0.015 * L.smoothstep(P[:, 2], 0.05, 0.085))
        rin, worst = last.enforce(rin, surf, target, FLOOR_IN, iters=8)
        off_out = rin - last.rho_h + WALL
        rout, se_out = last.layer(off_out, FLOOR_OUT, 0.008 + WALL, round_k=0.009, smooth=4,
                                  extra_smooth=(last.s_f0 + 0.035, 30))
        s_top = last.z0 - Z_TOP
        # --- thong paths and the fur compressing under them. Ankle figure-eight: two turns that
        # cross on the instep, one passing high behind the ankle, one low round the heel.
        Z_F, Z_BA, Z_BB = 0.080, 0.118, 0.056
        zA = lambda a: Z_F + (Z_BA - Z_F) * (1 + np.cos(a)) / 2
        zB = lambda a: Z_F + (Z_BB - Z_F) * (1 + np.cos(a)) / 2
        s_band = last.z0 - 0.252
        S, A = np.meshgrid(last.s, last.alpha, indexing="ij")
        sA = last.s_at_height(rout, last.alpha, zA(last.alpha))
        sB = last.s_at_height(rout, last.alpha, zB(last.alpha))
        dip = 0.0026 * (np.exp(-((S - sA[None]) / 0.0045) ** 2) + np.exp(-((S - sB[None]) / 0.0045) ** 2))
        dip += 0.0030 * np.exp(-((S - s_band) / 0.0050) ** 2)
        rout = rout - np.minimum(dip, 0.0032)
        # --- sole gathers at toe and heel, and the stitched sole seam bead (3D, on the outer layer)
        Po = last.point(S, A, rout)
        zseam = 0.021
        env = np.exp(-((Po[..., 2] - zseam) / 0.009) ** 2) * (Po[..., 2] > FLOOR_OUT + 0.002)
        toe_c = np.array([last.sgn * 0.155, last.y_toe + 0.05])
        ang = np.arctan2(Po[..., 0] - toe_c[0], Po[..., 1] - toe_c[1])
        front = L.smoothstep(-(Po[..., 1] - (last.y_toe + 0.075)), 0.0, 0.03)
        heel = L.smoothstep(Po[..., 1] - (last.y_heel - 0.035), 0.0, 0.02)
        hang = np.arctan2(Po[..., 0] - last.sgn * 0.135, Po[..., 1] - (last.y_heel - 0.04))
        pucker = front * np.sin(ang * 23.0 + 0.7 * fbm(Po, 0.02, seed)) * np.exp(-((Po[..., 2] - 0.012) / 0.010) ** 2)
        pucker += 0.7 * heel * np.sin(hang * 21.0) * np.exp(-((Po[..., 2] - 0.013) / 0.009) ** 2)
        bead = np.exp(-((Po[..., 2] - zseam) / 0.0016) ** 2)
        wrinkle = 0.0008 * fbm(Po * np.array([1, 1, 0.35]), 0.018, seed + 3)
        rout = rout + 0.0011 * np.maximum(pucker, -0.4) + 0.0007 * bead * (Po[..., 2] < 0.05) + wrinkle
        rout = np.minimum(rout, last.floor_rho(FLOOR_OUT))
        mirror = side == "r"
        cols = _alphas(72)
        # --- inner lining: from the rim to the toe
        Pin, Sin, Ain = last.surface(rin, s_top, se_in, cols, 56)
        ptop = last.point(s_top, cols, last.interp(rin, s_top, cols))
        uv = G.grid_uv(Pin, center_col=36)
        vparam = uv[..., 1][:, :-1]
        # fur lining lumps where it shows near the rim
        lump = fbm(Pin, 0.011, seed + 5) * (1 - L.smoothstep(vparam, 0.01, 0.05))
        dirv = last.direction(Sin, Ain)
        Pin = Pin - dirv * (0.0025 * lump)[..., None]
        roll_len = 0.5 * math.pi * 0.5 * (CUFF_T + WALL)
        mb.add_grid(Pin, uv * 0.6, FUR_LINING, collapse_last=True, flip=not mirror,
                    attrs=dict(open=vparam + roll_len, s=Sin, ca=np.cos(Ain), sa=np.sin(Ain), dback=_dback(uv)))
        # --- suede outer with the sole
        Pout, Sout, Aout = last.surface(rout, s_top + 0.004, se_out, cols, 88)
        uv = G.grid_uv(Pout, center_col=36)
        mb.add_grid(Pout, uv, SUEDE, collapse_last=True, flip=mirror,
                    attrs=dict(open=np.full(Sout.shape, FAR), s=Sout, ca=np.cos(Aout), sa=np.sin(Aout),
                               dback=_dback(uv)))
        # --- turned-out fur cuff: roll over the top, fur outside, cut edge, leather side back up
        r_in_top = last.interp(rin, s_top, cols)
        r_out_c = last.interp(rout, np.full_like(cols, s_top), cols) + 0.0006
        s_c_edge = s_top + CUFF_LEN + 0.006 * np.array([fbm(np.array([math.cos(a) * 0.05, math.sin(a) * 0.05, 0.0]),
                                                                0.012, seed + 9) for a in cols]) * 2.2
        rows_S, rows_R, rows_open, rows_id = [], [], [], []
        # roll (theta from inner side over the top to the cuff outside)
        r_co_top = r_out_c + CUFF_T
        mid = 0.5 * (r_in_top + r_co_top)
        hw = 0.5 * (r_co_top - r_in_top)
        for th in np.linspace(math.pi, 0, 9)[:-1]:
            rows_S.append(np.full_like(cols, s_top) - 0.85 * hw * math.sin(th))
            rows_R.append(mid + hw * math.cos(th))
            rows_open.append(np.full_like(cols, (math.pi - th) * np.mean(hw) * 0.9 - roll_len * 0 + 0.0))
        nfur = 16
        for t in np.linspace(0, 1, nfur):
            s = s_top + (s_c_edge - s_top) * t
            rows_S.append(s)
            rows_R.append(last.interp(rout, s, cols) + 0.0006 + CUFF_T)
            rows_open.append(np.full_like(cols, FAR))
        # cut edge roll
        for th in np.linspace(0, math.pi, 7)[1:-1]:
            rows_S.append(s_c_edge + 0.35 * CUFF_T * math.sin(th))
            rr = last.interp(rout, s_c_edge, cols) + 0.0006
            rows_R.append(rr + 0.5 * CUFF_T * (1 + math.cos(th)))
            rows_open.append(np.full_like(cols, FAR))
        for t in np.linspace(0, 1, 9):
            s = s_c_edge + (s_top + 0.006 - s_c_edge) * t
            rows_S.append(s)
            rows_R.append(last.interp(rout, s, cols) + 0.0006)
            rows_open.append(np.full_like(cols, FAR))
        CS = np.array(rows_S); CR = np.array(rows_R)
        CA = np.broadcast_to(cols[None], CS.shape)
        Pc = last.point(CS, CA, CR)
        n_roll = 8
        fur_zone = np.zeros(CS.shape)
        fur_zone[:n_roll + nfur + 3] = 1.0
        lumps = fbm(Pc, 0.010, seed + 11) + 0.5 * fbm(Pc, 0.0045, seed + 12)
        Pc = Pc + last.direction(CS, CA) * (0.0034 * lumps * fur_zone)[..., None]
        # the roll's first row is the lining's top row exactly (welded)
        Pc[0] = Pin[0]
        uv = G.grid_uv(Pc, center_col=36)
        opn = np.array(rows_open)
        mb.add_grid(Pc, uv, CUFF, flip=mirror, attrs=dict(open=np.where(opn < FAR, opn, FAR), s=CS,
                                                         ca=np.cos(CA), sa=np.sin(CA), fur=fur_zone, dback=_dback(uv)))
        # --- thongs
        surf_out = G.surface_from(_grid_arrays(Pout))
        prof = G.ellipse_profile(8, 0.0072, 0.0017)
        tau = np.linspace(0, 2, 280)
        al = math.pi + TAU * tau
        zz_path = np.where(tau < 1, zA(al), zB(al))
        ss = last.s_at_height(rout, np.mod(al, TAU), zz_path)
        rr = last.interp(rout, ss, al) + 0.0010
        path = last.point(ss, al, rr)
        nrm = last.normal(rout, ss, al)
        # the second turn rides over the first where they cross on the instep
        cross = np.exp(-(np.minimum(np.abs(tau - 1), np.abs(tau - 2)) / 0.06) ** 2) * (tau > 0.9)
        path = path + nrm * (0.0019 * cross)[:, None]
        G.add_tube(mb, path, nrm, prof, THONG, attrs=dict(open=FAR))
        kpt, kn = path[0], nrm[0]
        _knot(mb, kpt + kn * 0.0015, kn, np.array([last.sgn, 0.0, 0.0]), THONG, 0.0068, 0.0017, size=0.0115,
              attrs=dict(open=np.full(25, FAR)))
        for dx, dy, dz in ((0.022, -0.030, -0.035), (-0.004, -0.040, -0.028)):
            st = kpt + kn * 0.004
            wp = [st + np.array([last.sgn * dx, dy, dz]) * f for f in (0.4, 0.75, 1.0)]
            _tail(mb, surf_out, st, wp, THONG, prof, length_taper=0.2, offset=0.0011, attrs=dict(open=FAR))
        # shaft band: a turn and a bit, knotted on the outside of the leg
        al2 = math.pi / 2 - 0.25 + np.linspace(0, TAU + 0.5, 200)
        ss2 = s_band + 0.0035 * np.sin(al2 * 0.5)
        rr2 = last.interp(rout, ss2, al2) + 0.0010
        path2 = last.point(ss2, al2, rr2)
        n2 = last.normal(rout, ss2, al2)
        over = L.smoothstep(al2 - al2[0], TAU - 0.4, TAU - 0.1)
        path2 = path2 + n2 * (0.0017 * over)[:, None]
        G.add_tube(mb, path2, n2, prof, THONG, attrs=dict(open=FAR))
        ka = math.pi / 2
        kp = last.point(np.array([s_band]), np.array([ka]), last.interp(rout, np.array([s_band]), np.array([ka])))[0]
        kn2 = last.normal(rout, np.array([s_band]), np.array([ka]))[0]
        _knot(mb, kp + kn2 * 0.0015, kn2, np.array([0.0, 0.0, 1.0]), THONG, 0.0068, 0.0017, size=0.011,
              attrs=dict(open=np.full(25, FAR)))
        for dy, dl in ((-0.010, 0.080), (0.012, 0.064)):
            st = kp + kn2 * 0.004
            wp = [st + np.array([last.sgn * 0.004, dy, -dl]) * f for f in (0.4, 0.75, 1.0)]
            _tail(mb, surf_out, st, wp, THONG, prof, length_taper=0.2, offset=0.0011, attrs=dict(open=FAR))
        # --- stats
        Ptab_in = last.point(S, A, rin)
        live = last.s <= last.s_tip
        d_in, _, _ = surf.signed(Ptab_in[live].reshape(-1, 3))
        zz = Ptab_in[live][..., 2].ravel()
        shaft = (zz > 0.085) & (zz < Z_TOP)
        loft = (rout - rin)[live & (last.s > s_top)]
        b.feet[side] = dict(last=last, rin=rin, rout=rout, s_top=s_top)
        fv = V[last.leg_vertices]
        fv = fv[fv[:, 2] < 0.03]
        b.tex_extra = dict(foot_x=float(fv[:, 0].mean()), foot_y=float(fv[:, 1].mean()))
        b.info[side] = dict(pivot=[last.y_p, last.z_p], gap_worst_deficit_mm=1000 * worst,
                            shaft_inner_gap_min_mm=float(1000 * d_in[shaft].min()),
                            shaft_inner_gap_median_mm=float(1000 * np.median(d_in[shaft])),
                            foot_inner_gap_min_mm=float(1000 * d_in[~shaft & (zz > 0.004)].min()),
                            loft_mean_mm=float(1000 * loft.mean()))
        log(f"fur boots {side}: {b.info[side]}")
    b.mb.mirror_x()
    b.sole_thickness_mm = -1000 * FLOOR_OUT
    b.footbed_z_mm = 1000 * FLOOR_IN
    return b


def _dback(uv):
    """Distance around the row to the back seam (the grid's first/last column), per vertex."""
    U = uv[..., 0]
    return np.minimum(U[:, :-1] - U[:, :1], U[:, -1:] - U[:, :-1])


def _grid_arrays(P):
    """Triangle arrays of a cyclic grid (for surface queries): (V, N, T)."""
    R, C = P.shape[:2]
    Vv = P.reshape(-1, 3)
    idx = np.arange(R * C).reshape(R, C)
    tris = []
    for i in range(R - 1):
        a, b_ = idx[i], np.roll(idx[i], -1)
        c, d = np.roll(idx[i + 1], -1), idx[i + 1]
        tris.append(np.stack([a, b_, c], 1)); tris.append(np.stack([a, c, d], 1))
    Tt = np.vstack(tris)
    fn = np.cross(Vv[Tt[:, 1]] - Vv[Tt[:, 0]], Vv[Tt[:, 2]] - Vv[Tt[:, 0]])
    N = np.zeros_like(Vv)
    for k in range(3):
        np.add.at(N, Tt[:, k], fn)
    cen = Vv.mean(0)
    N /= np.maximum(np.linalg.norm(N, axis=1, keepdims=True), 1e-12)
    # orient outward from each ring's centre
    ring_c = P.mean(1, keepdims=True).repeat(C, 1).reshape(-1, 3)
    flip = ((Vv - ring_c) * N).sum(1) < 0
    N[flip] *= -1
    return Vv, N, Tt


# =================================================================== turnshoes

def turnshoes(V, T, surf, log=print, seed=0):
    b = Built("SKM_TurnShoes")
    mb = b.mb
    FLOOR_IN, FLOOR_OUT = -0.0005, -0.0050
    WALL = 0.0020
    SLIT_HALF = 0.125          # rad: half-width of the gap over the tongue
    HOLE_IN = 0.125            # rad from the gap edge to the lace holes
    for side in "l":          # the right foot is its mirror image (body symmetric to 0.2 mm)
        last = L.Last(V, T, side, z_top=0.13, log=log)
        Ph = last.hull_points()
        S, A = np.meshgrid(last.s, last.alpha, indexing="ij")
        toe_room = 0.0022 * L.smoothstep(S, last.s_f0 + 0.045, last.s_f0 + 0.085) * (0.5 - 0.5 * np.cos(A))
        g = 0.0017 + toe_room
        rin, se_in = last.layer(g, FLOOR_IN, 0.0045, round_k=0.004, extra_smooth=(last.s_f0 + 0.05, 45))
        rin, worst = last.enforce(rin, surf, np.full_like(g, 0.0014), FLOOR_IN)
        rout, se_out = last.layer(rin - last.rho_h + WALL, FLOOR_OUT, 0.0045 + WALL, round_k=0.0028, smooth=3,
                                  extra_smooth=(last.s_f0 + 0.05, 10))
        # topline: low at the ankle bones (lateral lower), up at the heel and the throat
        cols = _alphas(72, extra=[math.pi + sgn * (SLIT_HALF + d) for sgn in (-1, 1) for d in (-0.012, 0.012)])
        h = 0.062 - 0.006 * np.cos(cols) + 0.012 * np.cos(2 * cols) - 0.004 * np.sin(cols)
        s_top = last.s_at_height(rin, cols, h)
        s_top = L.gauss1d(s_top, 1.5, periodic=True)
        s_throat = float(last.s_at_height(rin, np.array([math.pi]), 0.080)[0])
        s_slit_end = s_throat + 0.058
        # tongue: the outer surface steps down between the two lace flaps
        da = np.abs(np.mod(A - math.pi + math.pi, TAU) - math.pi)
        across = 1 - L.smoothstep(da, SLIT_HALF - 0.010, SLIT_HALF + 0.010)
        along = 1 - L.smoothstep(S, s_slit_end - 0.010, s_slit_end)
        tongue = across * along
        rflap = rout.copy()
        rout = rout - 0.0017 * tongue
        # sole seam bead where the turned sole meets the upper, and faint wear creases at the ball
        Po = last.point(S, A, rout)
        bead = np.exp(-((Po[..., 2] - (FLOOR_OUT + 0.0038)) / 0.0011) ** 2)
        s_ball = last.s_f0 + (last.y_p - (last.y_toe + 0.075))
        crease = np.exp(-((S - s_ball) / 0.012) ** 2) * (0.5 - 0.5 * np.cos(A)) * np.sin(A * 9 + fbm(Po, 0.02, seed) * 3)
        rout = rout + 0.00045 * bead + 0.00035 * crease + 0.00015 * fbm(Po, 0.008, seed + 1)
        rout = np.minimum(rout, last.floor_rho(FLOOR_OUT))
        n_in, n_out = 48, 64
        Pin, Sin, Ain = last.surface(rin, s_top, se_in, cols, n_in)
        Pout, Sout, Aout = last.surface(rout, s_top, se_out, cols, n_out)
        ci = int(np.argmin(np.abs(cols - math.pi)))
        uv_in = G.grid_uv(Pin, center_col=ci)
        uv_out = G.grid_uv(Pout, center_col=ci)
        v_in = uv_in[..., 1][:, :-1]
        v_out = uv_out[..., 1][:, :-1]
        rim_len = 0.5 * math.pi * WALL * 0.5
        mirror = side == "r"
        mb.add_grid(Pin, uv_in * 0.5, TURN_INNER, collapse_last=True, flip=not mirror,
                    attrs=dict(open=v_in + rim_len, s=Sin, ca=np.cos(Ain), sa=np.sin(Ain), vtop=v_in, dback=_dback(uv_in)))
        mb.add_grid(Pout, uv_out, TURN_OUTER, collapse_last=True, flip=mirror,
                    attrs=dict(open=v_out + rim_len, s=Sout, ca=np.cos(Aout), sa=np.sin(Aout), vtop=v_out,
                               dback=_dback(uv_out),
                               dside=np.abs(uv_out[..., 0][:, :-1] - uv_out[:, [int(np.argmin(np.abs(cols - 1.5 * math.pi)))], 0]),
                               tongue=last.interp(tongue, Sout, Aout)))
        # rim: rounded cut edge from lining to grain side
        ri, ro = last.interp(rin, s_top, cols), last.interp(rout, s_top, cols)
        rows = []
        for th in np.linspace(math.pi, 0, 5):
            mid, hw = 0.5 * (ri + ro), 0.5 * (ro - ri)
            rows.append(last.point(s_top - 0.8 * np.abs(hw) * math.sin(th), cols, mid + hw * math.cos(th)))
        Pr = np.array(rows)
        Pr[0], Pr[-1] = Pin[0], Pout[0]
        uv = G.grid_uv(Pr, center_col=ci)
        mb.add_grid(Pr, uv, TURN_RIM, flip=mirror,
                    attrs=dict(open=np.zeros(Pr.shape[:2]), s=np.broadcast_to(s_top, Pr.shape[:2]),
                               ca=np.broadcast_to(np.cos(cols), Pr.shape[:2]),
                               sa=np.broadcast_to(np.sin(cols), Pr.shape[:2]), vtop=np.zeros(Pr.shape[:2]), dback=_dback(uv)))
        # lacing: 4 pairs of awl holes, criss-cross thong, tied at the throat
        hole_s = s_throat + 0.009 + np.arange(4) * 0.0125
        holes = {}
        for sg in (-1, 1):
            a = math.pi + sg * (SLIT_HALF + HOLE_IN)
            holes[sg] = [(s, a) for s in hole_s]
        prof = G.ellipse_profile(10, 0.0031, 0.0012)
        lace_r = 0.0006

        def lace_seg(p, q, extra=0.0):
            t = np.linspace(0, 1, 60)
            ss = p[0] + (q[0] - p[0]) * t
            aa = p[1] + (q[1] - p[1]) * t
            lift = np.where((t > 0.1) & (t < 0.9), lace_r + 0.00035 + extra * np.sin(math.pi * (t - 0.1) / 0.8),
                            -0.0012 + (lace_r + 0.0015) * L.smoothstep(np.minimum(t, 1 - t), 0.0, 0.1))
            base = np.maximum(last.interp(rflap, ss, aa), last.interp(rout, ss, aa))
            path = last.point(ss, aa, base + lift)
            nrm = last.normal(rflap, ss, aa)
            G.add_tube(mb, path, nrm, prof, LACE, caps=True, attrs=dict(open=np.full(len(path), FAR)))
            return path
        lace_seg(holes[-1][0], holes[1][0])
        for k in range(3):
            lace_seg(holes[-1][k], holes[1][k + 1], extra=0.0)
            lace_seg(holes[1][k], holes[-1][k + 1], extra=0.0013)
        # knot at the throat, bow loops and tails lying out over the instep
        ks = s_throat - 0.002
        kp = last.point(np.array([ks]), np.array([math.pi]), last.interp(rflap, np.array([ks]), np.array([math.pi])))[0]
        kn = last.normal(rflap, np.array([ks]), np.array([math.pi]))[0]
        tg = last.point(ks + 0.01, math.pi, 0.0) - last.point(ks, math.pi, 0.0)
        for k in range(2):
            lace_seg(holes[-1 if k == 0 else 1][3], (ks, math.pi + (0.03 if k else -0.03)), extra=0.0006 * k)
        _knot(mb, kp, kn, tg, LACE, 0.0030, 0.0012, size=0.0075, attrs=dict(open=np.full(25, FAR)))
        surf_out = G.surface_from(_grid_arrays(Pout))
        for sg, (loop_a, loop_s) in ((1, (0.55, 0.020)), (-1, (0.50, 0.018))):
            # bow loop: out over the flap and back to the knot
            p1 = last.point(ks + loop_s, math.pi + sg * loop_a, last.interp(rflap, ks + loop_s, math.pi + sg * loop_a))
            p2 = last.point(ks + loop_s * 0.35, math.pi + sg * loop_a * 1.15,
                            last.interp(rflap, ks + loop_s * 0.35, math.pi + sg * loop_a * 1.15))
            pts, nrm = G.hug(surf_out, [kp + kn * 0.002, p1, p2, kp + kn * 0.003], lace_r + 0.0005, step=0.0015)
            G.add_tube(mb, pts, nrm, prof, LACE, attrs=dict(open=np.full(len(pts), FAR)))
            # tail
            a_t = math.pi + sg * (loop_a * 0.6)
            wp = [last.point(ks + d, a_t + sg * 0.25 * d / 0.05, last.interp(rflap, ks + d, a_t + sg * 0.25 * d / 0.05))
                  for d in (0.02, 0.04, 0.055 + 0.01 * (sg > 0))]
            _tail(mb, surf_out, kp + kn * 0.0015, wp, LACE, prof, length_taper=0.15, offset=lace_r + 0.0004,
                  attrs=dict(open=np.full(1, FAR)))
        b.feet[side] = dict(last=last, rin=rin, rout=rout, s_top=s_top, cols=cols, holes=holes,
                            s_throat=s_throat, s_slit_end=s_slit_end, slit_half=SLIT_HALF)
        hole3 = np.array([last.point(np.array([s]), np.array([a]), last.interp(rflap, np.array([s]), np.array([a])))[0]
                          for sg_ in (-1, 1) for (s, a) in holes[sg_]])
        b.tex_extra = dict(ball_y=last.y_toe + 0.075, toe_y=last.y_toe, heel_y=last.y_heel, floor_out=FLOOR_OUT, top_z=0.085, holes=hole3)
        d_in, _, _ = surf.signed(Pin[:-1].reshape(-1, 3))
        zz = Pin[:-1, :, 2].ravel()
        b.info[side] = dict(pivot=[last.y_p, last.z_p], inner_gap_min_mm=float(1000 * d_in[zz > 0.003].min()),
                            inner_gap_median_mm=float(1000 * np.median(d_in[zz > 0.003])),
                            topline_z_mm={"heel": float(1000 * h[0]), "lateral": float(1000 * h[len(cols) // 4]),
                                          "throat": 80.0, "medial": float(1000 * h[3 * len(cols) // 4])},
                            gap_worst_deficit_mm=1000 * worst)
        log(f"turnshoes {side}: {b.info[side]}")
    b.mb.mirror_x()
    b.sole_thickness_mm = -1000 * FLOOR_OUT
    b.footbed_z_mm = 1000 * FLOOR_IN
    return b


# ============================================================== woven sandals

def woven_sandals(V, T, surf, bones, log=print, seed=0):
    b = Built("SKM_WovenSandals")
    mb = b.mb
    TOP, BOT = -0.0006, -0.0105
    R_RIM = 0.5 * (TOP - BOT)
    NA = 144
    for side in "l":          # the right foot is its mirror image (body symmetric to 0.2 mm)
        sg = 1.0 if side == "l" else -1.0
        m = (V[:, 0] * sg > 0.03) & (V[:, 2] < 0.045)
        P = V[m]
        H = L.hull2d(P[:, :2])
        _, c = L.poly_area_centroid(H)
        # margins: toes 7 mm, heel 5 mm, sides 5 mm
        ang = np.arange(NA) * TAU / NA
        rr = L.ray_poly(c, H, ang)
        dirs = np.stack([np.cos(ang), np.sin(ang)], 1)
        outline = c + dirs * rr[:, None]
        fwd = L.smoothstep(-(outline[:, 1] - c[1]), 0.03, 0.10)
        outline = outline + dirs * (0.005 + 0.002 * fwd)[:, None]
        outline = np.stack([L.gauss1d(outline[:, 0], 2.0, periodic=True), L.gauss1d(outline[:, 1], 2.0, periodic=True)], 1)
        # outward normals of the outline
        tng = np.roll(outline, -1, 0) - np.roll(outline, 1, 0)
        nrm2 = np.stack([tng[:, 1], -tng[:, 0]], 1)
        nrm2 /= np.linalg.norm(nrm2, axis=1, keepdims=True)
        if ((outline - c) * nrm2).sum() < 0:
            nrm2 = -nrm2
        inner = outline - nrm2 * R_RIM
        # rim bumps: weft rows wrap the outer warp every ~7 mm
        arc = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(np.vstack([outline, outline[:1]]), axis=0), axis=1))])[:-1]
        rows = []
        rad_frac = []
        zlist = []
        kind = []

        def flat(frac, z):
            pts = c + (inner - c) * frac
            return np.column_stack([pts, np.full(NA, z)])
        fr = np.linspace(0, 1, 12)
        for f in fr:                          # bottom, centre outward
            rows.append(flat(f, BOT)); rad_frac.append(np.full(NA, f)); kind.append(np.full(NA, 0.0))
        for th in np.linspace(-math.pi / 2, math.pi / 2, 11)[1:-1]:
            bump = 0.0010 * np.abs(np.sin(math.pi * (arc / 0.0068) + 1.4 * math.sin(th)))
            r = R_RIM + bump
            pts = inner + nrm2 * (r * math.cos(th))[:, None]
            rows.append(np.column_stack([pts, np.full(NA, 0.5 * (TOP + BOT)) + r * math.sin(th)]))
            rad_frac.append(np.full(NA, 1.0)); kind.append(np.full(NA, 1.0))
        for f in fr[::-1]:                    # top, rim inward
            rows.append(flat(f, TOP)); rad_frac.append(np.full(NA, f)); kind.append(np.full(NA, 2.0))
        Ps = np.array(rows)
        # weave relief: weft rows across the foot, grooves only on top (never into the foot)
        yy = Ps[..., 1]
        row_ph = (yy - yy.min()) / 0.0068
        ridge = np.abs(np.sin(math.pi * row_ph))
        kd = np.array(kind)
        frac = np.array(rad_frac)
        interior = frac < 0.97
        Ps[..., 2] -= np.where((kd == 2) & interior, 0.0011 * (1 - ridge), 0.0)
        Ps[..., 2] += np.where((kd == 0) & interior, 0.0010 * (1 - ridge), 0.0)
        # open distance: to the sole's outer edge along the top surface
        edge = (1 - frac) * np.linalg.norm(inner - c, axis=1)[None] + R_RIM
        nb = len(fr)
        pieces = ((slice(0, nb), "bottom", True, False), (slice(nb - 1, len(Ps) - nb + 1), "rim", False, False),
                  (slice(len(Ps) - nb, len(Ps)), "top", False, True))
        for sl, what, c_first, c_last in pieces:
            P_ = Ps[sl]
            if what == "rim":
                PP = np.concatenate([P_, P_[:, :1]], 1)
                u = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(PP[len(P_) // 2], axis=0), axis=1))])
                v = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(PP[:, 0], axis=0), axis=1))])
                uv = np.stack(np.meshgrid(u, v), -1)
            else:
                plan = np.stack([P_[..., 0] * sg * (-1 if what == "bottom" else 1), P_[..., 1]], -1)
                uv = np.concatenate([plan, plan[:, :1]], 1)
            mb.add_grid(P_, uv, SANDAL_SOLE, collapse_first=c_first, collapse_last=c_last,
                        attrs=dict(open=np.where(kd[sl] == 2, edge[sl], 0.0), kind=kd[sl], frac=frac[sl],
                                   row=row_ph[sl]))
        # --- cords
        CR = 0.0026
        prof = G.ellipse_profile(10, 2 * CR, 2 * CR * 0.92)

        def cord(points, surf_=surf, off=CR + 0.0005, caps=True):
            pts, nrm = G.hug(surf_, points, off, step=0.002)
            G.add_tube(mb, pts, nrm, prof, CORD, caps=caps, attrs=dict(open=np.full(len(pts), FAR)))
            return pts, nrm

        def edge_pt(y, which):
            """Sole rim point at height ~mid-rim at foot y on the medial/lateral edge."""
            k = np.argmin(np.abs(outline[:, 1] - y) + 10 * ((outline[:, 0] - c[0]) * sg * (1 if which == "lat" else -1) < 0))
            p = outline[k]
            return np.array([p[0], p[1], TOP + 0.002])

        bt = np.array(bones[f"bigtoe_01_{side}"][0]); it = np.array(bones[f"indextoe_01_{side}"][0])
        web = 0.5 * (bt + it)
        web_y = web[1] - 0.012
        # toe cord: from between the first two toes, up to the forefoot, then to both front side loops
        j = np.array([web[0] + sg * 0.012, web[1] + 0.028, 0.05])
        anchor = np.array([web[0], web_y, TOP - 0.004])
        y_front = web[1] + 0.045
        ml, ll = edge_pt(y_front, "med"), edge_pt(y_front, "lat")
        cord([anchor, anchor + np.array([0, 0.004, 0.012]), np.array([web[0], web_y + 0.01, 0.03]), j], caps=True)
        for e in (ml, ll):
            cord([j, 0.5 * (j + e) + np.array([0, 0, 0.012]), e + np.array([0, 0, 0.006]), e], caps=True)
        # side loops (chichi) standing up from the rim, front and rear
        y_rear = c[1] + 0.035
        mr, lr = edge_pt(y_rear, "med"), edge_pt(y_rear, "lat")
        for e in (ml, ll, mr, lr):
            n_out = np.array([e[0] - c[0], 0.0, 0.0]); n_out /= np.linalg.norm(n_out)
            loop = [e + np.array([0, -0.006, -0.004]), e + n_out * 0.001 + np.array([0, -0.004, 0.009]),
                    e + n_out * 0.001 + np.array([0, 0.004, 0.009]), e + np.array([0, 0.006, -0.004])]
            cord(loop, off=CR * 0.8 + 0.0004)
        # ties: front loop -> across the instep (crossing) -> rear loop on the other side
        inst = np.array([c[0] + sg * 0.005, c[1] - 0.035, 0.075])
        for e0, e1, lift in ((ml, lr, 0.0), (ll, mr, 0.0028)):
            midp = 0.5 * (e0 + e1)
            midp = np.array([midp[0], midp[1], 0.07])
            cord([e0 + np.array([0, 0, 0.008]), 0.5 * (e0 + midp) + np.array([0, 0, 0.012]), midp,
                  0.5 * (midp + e1) + np.array([0, 0, 0.012]), e1 + np.array([0, 0, 0.008])], off=CR + 0.0005 + lift)
        # heel loop up the back of the heel
        heel_b = np.array([c[0], outline[:, 1].max(), TOP + 0.002])
        cord([heel_b + np.array([-sg * 0.008, 0, 0]), heel_b + np.array([-sg * 0.006, 0.0, 0.032]),
              heel_b + np.array([sg * 0.006, 0.0, 0.032]), heel_b + np.array([sg * 0.008, 0, 0])], off=CR + 0.0005)
        # ankle wraps: from each rear loop up behind the ankle through the heel loop, round to the front
        ank = np.array(bones[f"foot_{side}"][0])
        kfront = np.array([ank[0] + sg * 0.022, ank[1] - 0.050, 0.060])
        for e, sgn_w in ((mr, -1), (lr, 1)):
            back = np.array([ank[0] - sgn_w * sg * 0.010, heel_b[1] - 0.004, 0.036 + 0.004 * (sgn_w > 0)])
            side_p = np.array([ank[0] - sgn_w * sg * 0.045, ank[1] + 0.005, 0.055])
            cord([e + np.array([0, 0, 0.008]), e + np.array([0, 0.012, 0.03]), back, side_p,
                  np.array([ank[0] - sgn_w * sg * 0.035, ank[1] - 0.04, 0.062]), kfront],
                 off=CR + 0.0005 + 0.0025 * (sgn_w > 0))
        # knot at the front of the ankle and two tails falling down the outside of the foot
        _, kl, kn = surf.signed(kfront[None])
        kpos = kl[0] + kn[0] * (CR + 0.001)
        _knot(mb, kpos, kn[0], np.array([sg * 1.0, 0.0, -0.3]), CORD, 2 * CR, 2 * CR, size=0.0135, turns=2,
              attrs=dict(open=np.full(25, FAR)))
        for dx, dy, dz in ((0.030, -0.020, -0.030), (0.034, 0.004, -0.024)):
            t0 = kpos + np.array([sg * 0.004, 0, 0])
            _tail(mb, surf, t0, [t0 + np.array([sg * dx * 0.5, dy * 0.5, dz * 0.5]),
                                 t0 + np.array([sg * dx, dy, dz])],
                  CORD, prof, length_taper=0.1, offset=CR + 0.0006, attrs=dict(open=np.full(1, FAR)))
        b.feet[side] = dict(outline=outline, center=c)
        b.tex_extra = dict(sgn=1.0, heel_y=float(outline[:, 1].max() - 0.035),
                           ball_y=float(outline[:, 1].min() + 0.08))
        b.info[side] = dict(sole_length_mm=float(1000 * (outline[:, 1].max() - outline[:, 1].min())),
                            sole_width_mm=float(1000 * (outline[:, 0].max() - outline[:, 0].min())))
        log(f"sandals {side}: {b.info[side]}")
    b.mb.mirror_x()
    b.sole_thickness_mm = -1000 * BOT
    b.footbed_z_mm = 1000 * TOP
    return b
