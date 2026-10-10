"""Modern footwear on the shoemaker's last: low canvas sneakers and flat leather lace-up ankle boots.

Construction notes (what we imitate):

* **Sneakers** (a slim women's vulcanised canvas low-top, the Superga 2750 / Vans Authentic
  family): a 10-12 oz cotton-duck upper (~1.2 mm, two plies at the eyestays) over a cotton drill
  lining, lasted onto an insole and dipped into a vulcanised rubber unit: a gum outsole with a
  diamond tread, a 1.2 mm white foxing tape wrapped ~1 cm up over the canvas all round, a rubber
  toe cap rising ~2.5 cm over the toe, a navy pinstripe over a thin red line. Five pairs of 4.6 mm
  nickel eyelets, a 7 mm flat cotton lace in a criss-cross from a bar over the bottom pair, tied
  in a bow above the top pair; a lightly padded tongue standing ~2 cm above the throat. Collar
  40 mm at the lateral side, 46 mm medial, 58 mm at the heel, 75 mm at the throat.
* **Ankle boots** (a sleek flat Goodyear-welted derby boot): 1.2-1.4 mm cognac aniline calf,
  leather lined, the shaft ending ~3.5 cm above the lateral ankle bone (104-114 mm). Quarters
  lapped over the vamp, a backstay and a heel counter, all double-stitched; seven pairs of small
  antique-brass eyelets, a 2.6 mm round waxed lace, a gusseted tongue. A 3.3 mm welt all round,
  stitched through its top; a 6 mm leather sole with a 3 mm raised waist, a stacked leather heel
  with a rubber top-piece, slight toe spring. 10.5 mm under the foot.

Both reuse the turnshoe's layer construction (``last.layer``/``enforce``) for the lining and the
upper, add a separate sole unit swept round the upper (``sole_unit``) whose top hugs the upper
like foxing tape (sneakers) or ends in a welt (boots), and drop upper faces hidden inside it.
All parts are original geometry. Units meters, bind pose, the character faces -Y.
"""
import math

import numpy as np
from mathutils import Vector

from . import geo as G
from . import last as L
from .pairs import FAR, Built, _alphas, _dback, _grid_arrays, fbm

TAU = L.TAU

# part ids (texture kinds)
SNK_CANVAS, SNK_LINING, SNK_RIM, SNK_RUBBER, SNK_LACE, SNK_EYELET = 30, 31, 32, 33, 34, 35
BOOT_CALF, BOOT_LINING, BOOT_RIM, BOOT_SOLE, BOOT_LACE, BOOT_EYELET = 40, 41, 42, 43, 44, 45


# ================================================================== the upper

def arc_positions(last, rho, alpha, s0, offsets, s_hi):
    """s values along the column ``alpha`` at the given 3D arc lengths below ``s0``."""
    ss = np.linspace(s0, s_hi, 400)
    P = last.point(ss, np.full_like(ss, alpha), last.interp(rho, ss, np.full_like(ss, alpha)))
    arc = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(P, axis=0), axis=1))])
    return np.interp(offsets, arc, ss)


def square_bottom(last, rho, z_v, floor_z, stadium, taper=0.12, k=0.0015):
    """Last the bottom of a layer square, all the way round: below ``z_v`` the foot (heel, arch,
    sides) curves in under itself, but leather pulled over a last drops straight to the sole.
    Every table point below ``z_v`` is pushed out along its own section direction until, in plan,
    it reaches the layer's outline at ``z_v`` (seen from the nearest point on the stadium axis),
    leaning in by ``taper`` per metre of drop; the floor then trims it to a crisp, slightly
    rounded edge. The sole unit can then meet the upper at the welt without a ledge."""
    S, A = np.meshgrid(last.s, last.alpha, indexing="ij")
    live = last.s <= last.s_tip
    s_hi = float(last.s[live].max())
    grid = last.surface(rho, np.zeros(last.na), s_hi, last.alpha, int(round(s_hi / 0.002)))[0]
    bvh = G.surface_from(_grid_arrays(grid)).bvh
    P = last.point(S, A, rho)
    D = last.direction(S, A)
    out = rho.copy()
    a0, a1 = stadium.a0, stadium.a1
    ax = (a1 - a0) / np.linalg.norm(a1 - a0)
    idx = np.argwhere(live[:, None] & (P[..., 2] < z_v) & (rho > 0))
    for i, j in idx:
        p = P[i, j]
        t = np.clip((p[:2] - a0) @ ax, 0.0, stadium.Ls)
        o = a0 + ax * t
        d = p[:2] - o
        nd = np.linalg.norm(d)
        if nd < 1e-4:
            continue
        d /= nd
        hit = bvh.ray_cast(Vector((o[0] + d[0] * 0.25, o[1] + d[1] * 0.25, z_v)), Vector((-d[0], -d[1], 0.0)), 0.25)
        if hit[0] is None:
            continue
        target = 0.25 - hit[3] - taper * (z_v - p[2])
        gain = D[i, j, 0] * d[0] + D[i, j, 1] * d[1]
        if gain < 0.25 or target <= nd:
            continue
        out[i, j] = rho[i, j] + (target - nd) / gain
    rf = last.floor_rho(floor_z)
    return L.smin(np.minimum(out, rf), rf, k)

def spring(y_start, y_tip, rise):
    """Toe spring of the upper: how far its floor (and the welt / foxing) lifts at plan y, 0
    behind ``y_start`` (the toe tips, where her bare toes rest flat), ``rise`` at ``y_tip``."""
    return lambda y: rise * L.smoothstep(-(np.asarray(y, float) - y_start), 0.0, y_start - y_tip) ** 1.3


def rocker(y_ball, y_toe, rise):
    """Extra lift of the outsole's tread under the toes: the sole thins from the ball to the toe
    tips so the toe spring starts at the ball, as on a real sole, without lifting her toes."""
    return lambda y: rise * L.smoothstep(-(np.asarray(y, float) - y_ball), 0.0, y_ball - y_toe) ** 1.4


def _smax(a, b, k):
    return -L.smin(-a, -b, k)


def toe_box(last, rho, req, spec, floor_z, grow=0.0):
    """Close the forefoot of a layer with a designed round toe, as a last maker shapes the toe
    instead of letting the leather follow the toes to a point. Behind ``spec['y_ref']`` (about
    2 cm behind her toe tips) the layer is kept; from there each vertical section is a
    superellipse whose bounds (lateral, medial, top, bottom) start at the layer's bounds at
    ``y_ref`` and close on an apex at ``spec['y_end'] - grow`` with their own profiles
    ``(1 - t**p) ** (1/q)``: the lateral side rounds off early, the medial stays straight longer,
    the top keeps the toe-box height and rounds over, the bottom stays on the floor (plus toe
    spring) and curls up only at the tip. Never inside ``req`` (smooth max). Returns (rho, s_end)."""
    sgn = last.sgn
    y_of = lambda s: last.y_p - (s - last.s_f0)
    s_of = lambda y: last.s_f0 + last.y_p - y
    s_r, s1 = s_of(spec["y_ref"]), s_of(spec["y_end"] - grow)
    blend = spec.get("blend", 0.02)

    def bounds(i):
        P = last.point(np.full(last.na, last.s[i]), last.alpha, rho[i])
        u = P[:, 0] * sgn
        return u.max(), u.min(), P[:, 2].max()
    i_r = int(np.searchsorted(last.s, s_r))
    ul_r, um_r, zt_r = bounds(i_r)
    zt_r = max(zt_r, spec.get("min_top", 0.0) + grow)
    zf = lambda y: floor_z + spec["spring"](y)
    za = zf(spec["y_end"]) + spec.get("apex_frac", 0.4) * (zt_r - floor_z)
    ua = um_r + spec.get("apex_u", 0.42) * (ul_r - um_r)
    out = rho.copy()
    k = spec.get("k", 0.0012)
    nu, nt, nb = spec.get("n_side", 2.6), spec.get("n_top", 2.3), spec.get("n_bot", 5.0)
    prof = lambda t, pq: (1 - np.clip(t, 0, 1) ** pq[0]) ** (1 / pq[1])
    for i in range(int(np.searchsorted(last.s, s_r - blend)), len(last.s)):
        s = last.s[i]
        if s >= s1:
            out[i] = 0.0
            continue
        y = y_of(s)
        if s < s_r:
            ul, um, zt = bounds(i)
            zb = zf(y)
        else:
            t = (s - s_r) / (s1 - s_r)
            ul = ua + (ul_r - ua) * prof(t, spec["lat"])
            um = ua - (ua - um_r) * prof(t, spec["med"])
            zt = za + (zt_r - za) * prof(t, spec["top"])
            zb = za - (za - zf(y)) * prof(t, spec["bot"])
        uc, hw = 0.5 * (ul + um), 0.5 * (ul - um)
        zc, hh = 0.5 * (zt + zb), 0.5 * (zt - zb)
        c = last.center(np.array([s]))[0]
        cz, cu = last.z_p - c[0], c[1]

        def f(z, u):
            n = np.where(z > zc, nt, nb)
            return (np.abs(u - uc) / hw) ** nu + (np.abs(z - zc) / hh) ** n
        if hw <= 1e-4 or hh <= 1e-4 or f(np.array(cz), np.array(cu)) >= 1:
            out[i] = 0.0 if s > s_r else out[i]
            continue
        du, dz = np.sin(last.alpha) * sgn * sgn, -np.cos(last.alpha)
        lo, hi = np.zeros(last.na), np.full(last.na, 0.25)
        for _ in range(36):
            mid = 0.5 * (lo + hi)
            inside = f(cz + mid * dz, cu + mid * du) < 1
            lo, hi = np.where(inside, mid, lo), np.where(inside, hi, mid)
        rd = 0.5 * (lo + hi)
        if s <= last.s_tip:
            rd = _smax(rd, req[i], k)
        w = L.smoothstep(s, s_r - blend, s_r)
        out[i] = rho[i] * (1 - w) + rd * w
    live = (last.s >= s_r - blend - 0.01) & (last.s < s1)
    lo_b = np.where((last.s <= last.s_tip)[:, None], req, 0.0)
    for _ in range(spec.get("relax", 4)):
        avg = 0.25 * (np.roll(out, 1, 1) + np.roll(out, -1, 1))
        up = np.vstack([out[:1], out[:-1]]); dn = np.vstack([out[1:], out[-1:]])
        avg = avg + 0.25 * (up + dn)
        out = np.where(live[:, None], np.maximum(out + 0.5 * (avg - out), lo_b), out)
    out[last.s >= s1] = 0.0
    return out, float(s1)

def laced_upper(b, last, surf, cfg, part_outer, part_inner, part_rim, log=print):
    """Lining, outer upper (with a lacing slit over a stepped-down tongue and a tongue tab above
    the throat) and the rolled top edge. Faces of the outer that ``cfg['zhide'](x, y)`` puts
    inside the sole unit are dropped. Returns a dict of tables and paths for later parts."""
    mb = b.mb
    S, A = np.meshgrid(last.s, last.alpha, indexing="ij")
    DA = np.abs(A - math.pi)
    Ph = last.hull_points()
    g = cfg["gap"](S, A, Ph[..., 2])
    rin, se_in = last.layer(g, cfg["floor_in"], cfg["tip_gap"], round_k=0.004, min_off=cfg.get("min_off"),
                            smooth=cfg.get("smooth_in", 8), extra_smooth=(last.s_f0 + 0.05, 45))
    toe = cfg.get("toe")
    if toe:
        req_in = np.where((last.s <= last.s_tip)[:, None],
                          np.minimum(last.rho_h + g, last.floor_rho(cfg["floor_in"])), 0.0)
        rin, se_in = toe_box(last, rin, req_in, toe, cfg["floor_in"])
    rin, worst = last.enforce(rin, surf, cfg["min_gap"](S, A, Ph[..., 2]), cfg["floor_in"])
    h0 = cfg["topline"]
    SH, HI, TH = cfg["slit_half"], cfg["hole_in"], cfg["tongue_half"]
    s_throat = float(last.s_at_height(rin, np.array([math.pi]), h0(np.array([math.pi])))[0])
    s_slit_end = s_throat + cfg["slit_len"]
    instep = (1 - L.smoothstep(DA, TH, TH + 0.25)) * (1 - L.smoothstep(S, s_slit_end + 0.004, s_slit_end + 0.03))
    W = cfg["wall"] + cfg["pad"] * instep
    rout, se_out = last.layer(rin - last.rho_h + W, cfg["floor_out"], cfg["tip_gap"] + cfg["wall"],
                              round_k=cfg["round_out"], smooth=3, extra_smooth=(last.s_f0 + 0.05, 10))
    if cfg.get("square_below"):
        rout = square_bottom(last, rout, cfg["square_below"], cfg["floor_out"], cfg["stadium"])
    if toe:
        rout, se_out = toe_box(last, rout, np.where(rin > 0, rin + W, 0.0), toe, cfg["floor_out"],
                               grow=cfg["wall"])
    # tongue: steps down between the flaps along the slit, and above the flaps' topline
    s_flap = L.gauss1d(last.s_at_height(rin, last.alpha, h0(last.alpha)), 1.5, periodic=True)
    across = 1 - L.smoothstep(DA, SH - 0.010, SH + 0.010)
    along = 1 - L.smoothstep(S, s_slit_end - 0.010, s_slit_end)
    tab = (1 - L.smoothstep(DA, TH - 0.035, TH)) * (1 - L.smoothstep(S, s_flap[None] - 0.0025, s_flap[None] + 0.0005))
    tongue = np.maximum(across * along, tab)
    rflap = rout.copy()
    rout = rout - cfg["step"] * tongue
    if cfg.get("relief") is not None:
        rout = rout + cfg["relief"](last, S, A, rout)
    rout = np.minimum(rout, last.floor_rho(cfg["floor_out"]))
    rflap = np.maximum(rflap, rout)
    # topline, raised over the tongue tab
    extra = [math.pi + sg * (x + d) for sg in (-1, 1) for x in (SH, TH) for d in (-0.012, 0.012)]
    cols = _alphas(cfg["cols"], extra=extra)
    dac = np.abs(cols - math.pi)
    bump = cfg["tab_h"] * np.clip(1 - (dac / (TH - 0.01)) ** 3, 0, 1) ** (1 / 3)
    h = h0(cols) + bump
    s_top = L.gauss1d(last.s_at_height(rin, cols, h), 0.8, periodic=True)
    zhide = cfg["zhide"]
    mirror = False
    ci = int(np.argmin(np.abs(cols - math.pi)))
    # --- lining
    # lining rows: dense down the first 9 cm (the concave back of the ankle must not chord out
    # through the upper), sparse along the hidden foot
    n_top, n_rest = cfg["n_in"]
    s_mid = np.minimum(s_top + cfg.get("in_dense", 0.09), se_in - 0.03)
    tt = np.concatenate([np.linspace(0, 1, n_top + 1)[:, None] * (s_mid - s_top)[None] + s_top[None],
                         s_mid[None] + np.linspace(0, 1, n_rest + 1)[1:, None] * (se_in - s_mid)[None]])
    Ain = np.broadcast_to(cols[None], tt.shape)
    Rin = last.interp(rin, tt, Ain)
    Rin[-1] = 0.0
    Pin, Sin = last.point(tt, Ain, Rin), tt
    uv_in = G.grid_uv(Pin, center_col=ci)
    v_in = uv_in[..., 1][:, :-1]
    W_top = last.interp(rout, s_top, cols) - last.interp(rin, s_top, cols)
    rim_len = 0.5 * math.pi * float(np.mean(W_top)) * 0.5
    zero = np.zeros(Pin.shape[:2])
    mb.add_grid(Pin, uv_in * 0.5, part_inner, collapse_last=True, flip=not mirror,
                attrs=dict(open=v_in + rim_len, dback=_dback(uv_in), vtop=v_in, tongue=zero,
                           da=np.abs(Ain - math.pi) * 0.04, sfr=Sin - s_throat, dslit=zero))
    # --- outer
    # rows by one global density along s (so neighbouring columns stay in step and quads don't
    # shear), doubled round the heel fan where the outline turns from the leg to the sole
    sg = np.linspace(0, se_out, 2001)
    dens = 1.0 + 1.2 * L.smoothstep(sg, last.L_sh - 0.03, last.L_sh) * (1 - L.smoothstep(sg, last.s_f0, last.s_f0 + 0.03))
    cdf = np.concatenate([[0], np.cumsum(0.5 * (dens[1:] + dens[:-1]) * np.diff(sg))])
    c0 = np.interp(s_top, sg, cdf)
    So = np.interp(c0[None] + (cdf[-1] - c0[None]) * np.linspace(0, 1, cfg["n_out"] + 1)[:, None], cdf, sg)
    Aout = np.broadcast_to(cols[None], So.shape)
    Ro = last.interp(rout, So, Aout)
    Ro[-1] = 0.0
    Pout, Sout = last.point(So, Aout, Ro), So
    vh = Pout[..., 2] < zhide(Pout[..., 0], Pout[..., 1]) - 0.0006
    keep = ~(vh[:-1, :] & vh[1:, :] & np.roll(vh[:-1, :], -1, 1) & np.roll(vh[1:, :], -1, 1))
    uv_out = G.grid_uv(Pout, center_col=ci)
    v_out = uv_out[..., 1][:, :-1]
    rho_o = last.interp(rout, Sout, Aout)
    mb.add_grid(Pout, uv_out, part_outer, collapse_last=True, flip=mirror, keep=keep,
                attrs=dict(open=v_out + rim_len, dback=_dback(uv_out), vtop=v_out,
                           tongue=last.interp(tongue, Sout, Aout), da=np.abs(Aout - math.pi) * rho_o,
                           sfr=Sout - s_throat, dslit=(np.abs(Aout - math.pi) - SH) * rho_o))
    # --- rolled top edge from the lining to the outer
    ri, ro = last.interp(rin, s_top, cols), last.interp(rout, s_top, cols)
    rows = []
    for th in np.linspace(math.pi, 0, 6):
        mid, hw = 0.5 * (ri + ro), 0.5 * (ro - ri)
        rows.append(last.point(s_top - 0.85 * np.abs(hw) * math.sin(th), cols, mid + hw * math.cos(th)))
    Pr = np.array(rows)
    Pr[0], Pr[-1] = Pin[0], Pout[0]
    uv = G.grid_uv(Pr, center_col=ci)
    shp = Pr.shape[:2]
    mb.add_grid(Pr, uv, part_rim, flip=mirror,
                attrs=dict(open=np.zeros(shp), dback=_dback(uv), vtop=np.zeros(shp),
                           tongue=np.broadcast_to(last.interp(tongue, s_top, cols), shp),
                           da=np.broadcast_to(dac * ro, shp), sfr=np.broadcast_to(s_top - s_throat, shp),
                           dslit=np.broadcast_to((dac - SH) * ro, shp)))
    # full-resolution outer surface for the sole unit's ray casts and the lace hugging
    Pfull = last.surface(rout, s_top, se_out, cols, 90)[0]
    surf_out = G.surface_from(_grid_arrays(Pfull))
    d_in, _, _ = surf.signed(Pin[:-1].reshape(-1, 3))
    zz = Pin[:-1, :, 2].ravel()
    info = dict(pivot=[last.y_p, last.z_p], inner_gap_min_mm=float(1000 * d_in[zz > 0.003].min()),
                inner_gap_median_mm=float(1000 * np.median(d_in[zz > 0.003])),
                topline_z_mm={"heel": float(1000 * h0(np.array([0.0]))[0]),
                              "lateral": float(1000 * h0(np.array([math.pi / 2]))[0]),
                              "throat": float(1000 * h0(np.array([math.pi]))[0]),
                              "medial": float(1000 * h0(np.array([1.5 * math.pi]))[0]),
                              "tongue_tab": float(1000 * (h0(np.array([math.pi]))[0] + cfg["tab_h"]))},
                gap_worst_deficit_mm=1000 * worst, toe_pole_z_mm=float(1000 * Pout[-1, 0, 2]))
    return dict(rin=rin, rout=rout, rflap=rflap, tongue=tongue, s_top=s_top, cols=cols, s_throat=s_throat,
                s_slit_end=s_slit_end, se_out=se_out, surf_out=surf_out, info=info, Pfull=Pfull)


# ================================================================ sole unit

class Stadium:
    """Plan columns round the foot: a stadium about the foot's long axis. Each column has an
    origin on the axis segment and a horizontal outward direction."""

    def __init__(self, V, sgn, hw=0.030):
        m = (V[:, 0] * sgn > 0.03) & (V[:, 2] < 0.045)
        P = V[m][:, :2]
        c = P.mean(0)
        w, U = np.linalg.eigh(np.cov((P - c).T))
        ax = U[:, -1]
        if ax[1] > 0:
            ax = -ax                                   # toward the toes (-Y)
        lat = np.array([-ax[1], ax[0]])
        if lat[0] * sgn < 0:
            lat = -lat
        t = (P - c) @ ax
        self.a0 = c + ax * (t.min() + hw)              # heel end of the axis
        self.a1 = c + ax * (t.max() - hw)              # toe end
        self.ax, self.lat, self.hw = ax, lat, hw
        self.Ls = float(np.linalg.norm(self.a1 - self.a0))
        q = 0.5 * math.pi * hw
        self.seg = np.cumsum([0, q, self.Ls, math.pi * hw, self.Ls, q])
        self.perim = float(self.seg[-1])

    def at(self, u):
        """Origin (n,2) and direction (n,2) at stadium arc positions u (wrapped)."""
        u = np.mod(np.asarray(u, float), self.perim)
        o = np.zeros((len(u), 2)); d = np.zeros((len(u), 2))
        back, ax, lat, hw, sg = -self.ax, self.ax, self.lat, self.hw, self.seg
        for i, x in enumerate(u):
            if x < sg[1]:                        # heel cap, back -> lateral
                ph = x / hw
                o[i], d[i] = self.a0, math.cos(ph) * back + math.sin(ph) * lat
            elif x < sg[2]:                      # lateral side
                o[i], d[i] = self.a0 + ax * (x - sg[1]), lat
            elif x < sg[3]:                      # toe cap
                ph = (x - sg[2]) / hw
                o[i], d[i] = self.a1, math.cos(ph) * lat + math.sin(ph) * ax
            elif x < sg[4]:                      # medial side
                o[i], d[i] = self.a1 - ax * (x - sg[3]), -lat
            else:                                # heel cap, medial -> back
                ph = (x - sg[4]) / hw
                o[i], d[i] = self.a0, math.cos(ph) * (-lat) + math.sin(ph) * back
        return o, d

    def u_at_y(self, y, side):
        """Arc position on the lateral (+1) / medial (-1) straight where the origin has plan y."""
        uu = np.linspace(self.seg[1] if side > 0 else self.seg[3], self.seg[2] if side > 0 else self.seg[4], 400)
        o, _ = self.at(uu)
        k = np.argsort(o[:, 1])
        return float(np.interp(y, o[k, 1], uu[k]))


def _cast(surf_up, o, d, z, reach=0.25, normals=False):
    """Distance from the column origin to the outer upper along +d at height z (nan: miss), and
    optionally the hit face normal turned to face the caster (outward)."""
    out = np.full(len(z), np.nan)
    nrm = np.zeros((len(z), 3))
    bvh = surf_up.bvh
    for i in range(len(z)):
        org = Vector((o[i, 0] + d[i, 0] * reach, o[i, 1] + d[i, 1] * reach, float(z[i])))
        hit = bvh.ray_cast(org, Vector((-d[i, 0], -d[i, 1], 0.0)), reach)
        if hit[0] is not None:
            out[i] = reach - hit[3]
            nv = np.array(hit[1])
            nrm[i] = nv if nv[0] * d[i, 0] + nv[1] * d[i, 1] >= 0 else -nv
    return (out, nrm) if normals else out


def sole_unit(b, V, sgn, surf_up, cfg, part, log=print):
    """A sole unit swept round the upper: a bottom (rings to a centre pole), a rounded bottom
    edge, a sidewall at the plan outline, and a top that either hugs the upper up to
    ``cfg['zfox'](x, y)`` and tucks in (foxing tape, ``style='foxing'``) or turns in as a welt
    at ``cfg['z_welt']`` (``style='welt'``). Bottom UVs are plan metres; the sidewall is split
    into ~12 cm UV strips with U = perimeter arc length."""
    mb = b.mb
    st = cfg.get("stadium") or Stadium(V, sgn)
    nc = cfg["cols"]
    uu = np.linspace(0, st.perim, 2001)
    w = 1.0 + 1.2 * ((uu < st.seg[1]) | (uu > st.seg[4])) + 4.0 * ((uu > st.seg[2]) & (uu < st.seg[3]))
    cdf = np.concatenate([[0], np.cumsum(0.5 * (w[1:] + w[:-1]) * np.diff(uu))])
    u = np.interp((np.arange(nc) + 0.5) / nc * cdf[-1], cdf, uu)
    if cfg.get("extra_y"):
        add = []
        for y in cfg["extra_y"]:
            for sd in (1, -1):
                uu = st.u_at_y(y, sd)
                add += [uu - 0.0014, uu + 0.0014]
        u = np.sort(np.concatenate([u, add]))
    o, d = st.at(u)
    n = len(u)
    lift = cfg.get("lift", lambda x, y: np.zeros(np.shape(x)))
    Og = o + d * 0.06
    for _ in range(2):                    # heights follow the toe spring of the column they cast at
        lz = lift(Og[:, 0], Og[:, 1])
        ref = np.array([_cast(surf_up, o, d, np.full(n, z) + lz) for z in cfg["ref_z"]])
        r_ref = np.nanmax(ref, 0)
        r_ref = np.where(np.isfinite(r_ref), r_ref, np.nanmedian(r_ref))
        Og = O0 = o + d * r_ref[:, None]
    ext = cfg["ext"](O0[:, 0], O0[:, 1])
    r1 = r_ref + ext
    r_out = np.maximum(L.gauss1d(r1, 1.0, periodic=True), r_ref + 0.6 * ext)  # smooth, never inside
    re = cfg["r_edge"]
    zb = cfg["zb"]
    Oo = o + d * r_out[:, None]
    zb_o = zb(Oo[:, 0], Oo[:, 1])
    lz = lift(Oo[:, 0], Oo[:, 1])

    def up_r(z, fallback):
        r = _cast(surf_up, o, d, z)
        return np.where(np.isfinite(r), r, fallback)

    def cp(r, z):
        return np.column_stack([o + d * np.asarray(r)[:, None], np.broadcast_to(z, (n,))])

    # --- strip rows (points, kind) from the bottom edge up
    rows = [(cp(r_out - re, zb_o), 1.0)]
    for ang in np.linspace(-math.pi / 2, 0, cfg["n_edge"] + 1)[1:]:
        rows.append((cp(r_out - re + re * math.cos(ang), zb_o + re + re * math.sin(ang)), 1.0))
    if cfg["style"] == "foxing":
        t_rub = cfg["t_rub"]
        z_w1 = cfg["z_w1"] + lz
        for z in np.linspace(zb_o + re, z_w1, cfg["n_wall"] + 1)[1:]:
            rows.append((cp(r_out, z), 1.0))
        zf = cfg["zfox"](Oo[:, 0], Oo[:, 1]) + lz
        # foxing tape: offset along the upper's normal (a horizontal offset would graze the
        # sloping toe and let the canvas show through), never narrower than the tape above it
        Hs, Ns, Zs = [], [], []
        for t in np.linspace(0, 1, cfg["n_hug"] + 1)[1:]:
            z = z_w1 + (zf - z_w1) * t
            r, Nn = _cast(surf_up, o, d, z, normals=True)
            H = cp(np.nan_to_num(r, nan=0.0), z)
            if Hs:
                ok = np.isfinite(r)[:, None]
                H = np.where(ok, H, Hs[-1] + np.array([0.0, 0.0, 1.0]) * (z - Zs[-1])[:, None])
                Nn = np.where(ok, Nn, Ns[-1])
            Nn = np.stack([L.gauss1d(Nn[:, i], 1.0, periodic=True) for i in range(3)], 1)
            Nn /= np.maximum(np.linalg.norm(Nn, axis=1, keepdims=True), 1e-9)
            Hs.append(H); Ns.append(Nn); Zs.append(z)
        Ph = [H + Nn * t_rub for H, Nn in zip(Hs, Ns)]
        rp = [((P[:, :2] - o) * d).sum(1) for P in Ph]
        env = rp[-1].copy()
        for k in range(len(Ph) - 1, -1, -1):
            env = np.maximum(env, rp[k])
            Ph[k] = Ph[k] + np.column_stack([d * (env - rp[k])[:, None], np.zeros(n)])
        # taper from the sole outline at z_w1 to the tape on the canvas at the top, never inside it
        r_top = ((Ph[-1][:, :2] - o) * d).sum(1)
        for P, z in zip(Ph, Zs):
            tz = L.smoothstep(z, z_w1, zf)
            target = r_out + (r_top - r_out) * tz
            rr = ((P[:, :2] - o) * d).sum(1)
            P = P + np.column_stack([d * np.clip(target - rr, 0, None)[:, None], np.zeros(n)])
            rows.append((P, 1.0))
        H, Nn = Hs[-1], Ns[-1]
        U = np.array([0.0, 0.0, 1.0]) - Nn * Nn[:, 2:3]
        U /= np.maximum(np.linalg.norm(U, axis=1, keepdims=True), 1e-9)
        C = H + Nn * (0.5 * t_rub)
        rad = 0.5 * t_rub + 0.0002
        for ang in np.linspace(0, math.pi, 5)[1:]:
            rows.append((C + rad * (math.cos(ang) * Nn + 0.9 * math.sin(ang) * U), 2.0))
        rows.append((H - U * 0.002 - Nn * 0.0009, 2.0))
    else:
        zw, rt = cfg["z_welt"] + lz, cfg["r_top"]
        for z in np.linspace(zb_o + re, zw - rt, cfg["n_wall"] + 1)[1:]:
            rows.append((cp(r_out, z), 1.0))
        for ang in np.linspace(0, math.pi / 2, cfg["n_top"] + 1)[1:]:
            rows.append((cp(r_out - rt + rt * math.cos(ang), zw - rt + rt * math.sin(ang)), 2.0))
        ru = up_r(zw, r_out - 0.004)
        r_in = ru + 0.0003
        r0 = r_out - rt
        for t in np.linspace(0, 1, cfg["n_welt"] + 1)[1:]:
            rows.append((cp(r0 + (r_in - r0) * t, zw + 0.00025 * math.sin(math.pi * t)), 2.0 + 0.9 * t))
        rows.append((cp(up_r(zw - 0.0016, ru) - 0.0008, zw - 0.0016), 2.9))
    Ps = np.array([p for p, _ in rows])                                             # (rows, n, 3)
    K = np.array([np.full(n, k) for _, k in rows])    # --- bottom: rings to a centre pole
    Oi = o + d * (r_out - re)[:, None]
    cpt = Oi.mean(0)
    rings = []
    for f in np.linspace(0, 1, cfg["n_bottom"]):
        p2 = cpt + (Oi - cpt) * f
        rings.append(np.column_stack([p2, zb(p2[:, 0], p2[:, 1])]))
    Pb = np.array(rings)
    Pb[-1] = Ps[0]
    # winding: bottom faces must point down, strip faces outward
    nb = np.cross(Pb[2, 1] - Pb[2, 0], Pb[3, 0] - Pb[2, 0])
    flip_b = nb[2] > 0
    k = len(rows) // 2
    nw = np.cross(Ps[k, 1] - Ps[k, 0], Ps[k + 1, 0] - Ps[k, 0])
    flip_w = (nw[:2] @ d[0]) < 0
    uvb = np.concatenate([Pb[..., :2], Pb[:, :1, :2]], 1)
    zero_b = np.zeros(Pb.shape[:2])
    mb.add_grid(Pb, uvb, part, collapse_first=True, flip=flip_b,
                attrs=dict(open=np.full(Pb.shape[:2], FAR), kind=zero_b, perim=zero_b, zrel=zero_b,
                           frac=np.broadcast_to(np.linspace(0, 1, len(Pb))[:, None], Pb.shape[:2])))
    # sidewall strips
    Pc = np.concatenate([Oo, Oo[:1]], 0)
    U = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(Pc, axis=0), axis=1))])
    n_chunk = max(3, int(round(U[-1] / 0.12)))
    cuts = np.linspace(0, n, n_chunk + 1).round().astype(int)
    for c0, c1 in zip(cuts[:-1], cuts[1:]):
        idx = np.arange(c0, c1 + 1)
        jj = idx % n
        Pk = Ps[:, jj]
        dv = np.linalg.norm(np.diff(Pk, axis=0), axis=2)
        Vv = np.concatenate([np.zeros((1, len(idx))), np.cumsum(dv, 0)], 0)
        Uu = np.broadcast_to(U[idx][None] - U[c0], Vv.shape)
        uv = np.stack([Uu, Vv], -1)
        mb.add_grid(Pk, uv, part, cyclic=False, flip=flip_w,
                    attrs=dict(open=np.full(Pk.shape[:2], FAR), kind=K[:, jj],
                               perim=np.broadcast_to(U[idx][None], Vv.shape),
                               zrel=Pk[..., 2] - zb_o[jj][None], frac=np.ones(Vv.shape)))
    ov = 1000 * (r_out - r_ref)
    info = dict(sole_cols=int(n), sole_rows=int(len(rows)), sole_perimeter_mm=float(1000 * U[-1]),
                sole_overhang_mm=dict(p50=round(float(np.median(ov)), 2), p90=round(float(np.percentile(ov, 90)), 2),
                                      max=round(float(ov.max()), 2)),
                sole_length_mm=float(1000 * (Oo[:, 1].max() - Oo[:, 1].min())),
                sole_width_mm=float(1000 * (Oo[:, 0].max() - Oo[:, 0].min())))
    return dict(outline=Oo, info=info, stadium=st)


# =================================================================== hardware

def band_profile(k, width, thick, p=0.35):
    """A flat band's cross-section: a superellipse (rounded rectangle), ``k`` points."""
    a = np.arange(k) * 2 * math.pi / k + math.pi / k
    c, s = np.cos(a), np.sin(a)
    return np.stack([0.5 * width * np.sign(c) * np.abs(c) ** p, 0.5 * thick * np.sign(s) * np.abs(s) ** p], 1)


def eyelet(mb, center, nrm, part, r_major, width, thick, segs=10, k=4):
    """A rolled eyelet ring (washer section) lying on the surface round a lace hole."""
    nrm = nrm / np.linalg.norm(nrm)
    t0 = np.cross(nrm, [0.0, 0.0, 1.0])
    if np.linalg.norm(t0) < 1e-6:
        t0 = np.cross(nrm, [1.0, 0.0, 0.0])
    t0 /= np.linalg.norm(t0)
    b0 = np.cross(nrm, t0)
    ang = np.arange(-1, segs + 2) * TAU / segs
    path = center[None] + r_major * (np.cos(ang)[:, None] * t0[None] + np.sin(ang)[:, None] * b0[None])
    up = np.repeat(nrm[None], len(ang), 0)
    rings = G.sweep(path, up, G.ellipse_profile(k, width, thick))[1:-1]
    G.add_tube(mb, path[1:-1], ("rings", rings), G.ellipse_profile(k, width, thick), part, caps=False,
               attrs=dict(open=FAR, aglet=0.0))


def knot(mb, center, nrm, tang, part, width, thick, size, sides=6, n=32):
    """An overhand knot pulled flat against the surface (a lighter copy of pairs._knot)."""
    nrm = nrm / np.linalg.norm(nrm)
    tang = tang - nrm * (tang @ nrm)
    tang /= np.linalg.norm(tang)
    bi = np.cross(nrm, tang)
    t = np.linspace(0, TAU, n)
    k = size / 3.2
    x = (np.sin(t) + 2 * np.sin(2 * t)) * k
    y = (np.cos(t) - 2 * np.cos(2 * t)) * k
    zz = -np.sin(3 * t) * k * 0.55 * (thick / max(width, thick)) * 2.2
    path = center[None] + x[:, None] * tang[None] + y[:, None] * bi[None] + (zz + k * 1.2)[:, None] * nrm[None]
    G.add_tube(mb, path, np.repeat(nrm[None], n, 0), G.ellipse_profile(sides, width * 0.9, thick * 1.1), part,
               caps=False, attrs=dict(open=np.full(n, FAR), aglet=np.zeros(n)))


class Lacer:
    """Laces running over the flaps between holes (diving into them at the ends), and a bow."""

    def __init__(self, mb, last, up, part, prof, lace_r, raise_=0.0004, n_pts=16):
        self.mb, self.last, self.up, self.part, self.prof = mb, last, up, part, prof
        self.lace_r, self.raise_, self.n = lace_r, raise_, n_pts

    def seg(self, p, q, extra=0.0, dive=(True, True)):
        last, up = self.last, self.up
        t = np.linspace(0, 1, self.n)
        ss = p[0] + (q[0] - p[0]) * t
        aa = p[1] + (q[1] - p[1]) * t
        top = self.lace_r + self.raise_
        e = np.minimum(np.where(dive[0], t, 1.0), np.where(dive[1], 1 - t, 1.0))
        lift = -0.0012 + (top + 0.0012) * L.smoothstep(e, 0.0, 0.13) + extra * np.sin(math.pi * t) ** 2
        base = np.maximum(last.interp(up["rflap"], ss, aa), last.interp(up["rout"], ss, aa))
        path = last.point(ss, aa, base + lift)
        nrm = last.normal(up["rflap"], ss, aa)
        G.add_tube(self.mb, path, nrm, self.prof, self.part, caps=True,
                   attrs=dict(open=np.full(len(path), FAR), aglet=np.zeros(len(path))))
        return path

    def bow(self, ks, loops, tails, knot_size, width, thick):
        """Knot at (ks, pi) with two loops and two tails lying out over the flaps."""
        last, up = self.last, self.up
        rf = up["rflap"]
        A1, S1 = np.array([math.pi]), np.array([ks])
        kp = last.point(S1, A1, last.interp(rf, S1, A1) + self.lace_r)[0]
        kn = last.normal(rf, S1, A1)[0]
        tg = last.point(ks + 0.01, math.pi, 0.0) - last.point(ks, math.pi, 0.0)
        knot(self.mb, kp, kn, tg, self.part, width, thick, size=knot_size)
        surf = up["surf_out"]
        off = self.lace_r + 0.0006
        P_at = lambda s, a: last.point(s, a, last.interp(rf, s, a))
        for sg, (la, ls, lift) in zip((1, -1), loops):
            p1 = P_at(ks + ls, math.pi + sg * la)
            p2 = P_at(ks + ls * 0.4, math.pi + sg * la * 1.2)
            pts, nrm = G.hug(surf, [kp + kn * 0.002, p1, p2, kp + kn * 0.003], off, step=0.0040,
                             lift=lambda t, h=lift: h * np.sin(math.pi * t) ** 1.5)
            G.add_tube(self.mb, pts, nrm, self.prof, self.part,
                       attrs=dict(open=np.full(len(pts), FAR), aglet=np.zeros(len(pts))))
        for sg, (ta, tl, tw) in zip((1, -1), tails):
            wp = [P_at(ks + f * tl, math.pi + sg * (ta + tw * f)) for f in (0.3, 0.65, 1.0)]
            pts, nrm = G.hug(surf, [kp + kn * 0.0015] + wp, off, step=0.0040)
            arc = np.concatenate([[0], np.cumsum(np.linalg.norm(np.diff(pts, axis=0), axis=1))])
            ag = L.smoothstep(arc - (arc[-1] - 0.016), -0.0004, 0.0004)
            sc = np.stack([1 - 0.12 * arc / arc[-1] - 0.18 * ag, 1 + 0.25 * ag], 1)
            G.add_tube(self.mb, pts, nrm, self.prof, self.part, scale=sc,
                       attrs=dict(open=np.full(len(pts), FAR), aglet=ag))
        return kp


def _holes(last, up, alpha_off, first, pitch, n):
    """Lace holes on both flaps: ``first`` m of arc below the throat, then every ``pitch``."""
    holes = {}
    for sg in (-1, 1):
        a = math.pi + sg * alpha_off
        s = arc_positions(last, up["rflap"], a, up["s_throat"] - 0.004, first + pitch * np.arange(n),
                          up["s_slit_end"] + 0.03)
        holes[sg] = [(float(x), a) for x in s]
    return holes


def _front_point(last, up, s):
    S1, A1 = np.array([s]), np.array([math.pi])
    return last.point(S1, A1, last.interp(up["rflap"], S1, A1))[0]


def _hole_points(last, up, holes):
    out = []
    for sg in (-1, 1):
        for s, a in holes[sg]:
            S1, A1 = np.array([s]), np.array([a])
            out.append((last.point(S1, A1, last.interp(up["rflap"], S1, A1))[0], last.normal(up["rflap"], S1, A1)[0]))
    return out


def criss_cross(lacer, holes, lift):
    """Bar across the lowest pair, then crossings up to the top pair (one end always over)."""
    nh = len(holes[1])
    lacer.seg(holes[-1][nh - 1], holes[1][nh - 1], extra=-0.0003)
    for k in range(nh - 1, 0, -1):
        lacer.seg(holes[-1][k], holes[1][k - 1], extra=0.0)
        lacer.seg(holes[1][k], holes[-1][k - 1], extra=lift)


# =================================================================== sneakers

def sneakers(V, T, surf, log=print, seed=0):
    b = Built("SKM_Sneakers")
    mb = b.mb
    FLOOR_IN, FLOOR_OUT, Z_BOT = -0.0005, -0.0030, -0.0120
    CAP_AX, CAP_AY, CAP_T = 0.058, 0.050, 0.0011
    TOE_PAST, RISE, ROCK = 0.0125, 0.0050, 0.0028   # toe past her toes; upper spring; sole rocker
    for side in "l":          # the right foot is its mirror image (body symmetric to 0.2 mm)
        last = L.Last(V, T, side, z_top=0.115, log=log)
        y_toe, y_heel = last.y_toe, last.y_heel
        foot = V[(V[:, 0] * last.sgn > 0.03) & (V[:, 2] < 0.03)]
        x_toe = float(foot[foot[:, 1] < y_toe + 0.03, 0].mean())
        y_tip = y_toe - TOE_PAST - 0.0019
        lift = spring(y_toe, y_tip - 0.0015, RISE)
        rock = rocker(y_toe + 0.055, y_toe, ROCK)
        toe = dict(y_ref=y_toe + 0.040, y_end=y_toe - TOE_PAST, spring=lift, apex_u=0.38, apex_frac=0.42, blend=0.04, relax=14,
                   lat=(1.3, 1.6), med=(2.2, 1.7), top=(2.3, 1.8), bot=(4.0, 2.2), n_top=2.0, n_side=2.15)

        def zfox(x, y):
            # 9.5 mm of foxing tape above the bare sole, 11 mm round the heel
            return 0.0095 + 0.0015 * L.smoothstep(y, y_heel - 0.05, y_heel)

        def cap_e(x, y):
            # rubber toe cap glued over the canvas: an ellipse in plan over the front ~5 cm
            return np.sqrt(((x - x_toe) / CAP_AX) ** 2 + ((y - y_tip) / CAP_AY) ** 2)

        def relief(last_, S, A, rout):
            Po = last_.point(S, A, rout)
            s_ball = last_.s_f0 + (last_.y_p - (y_toe + 0.075))
            top = 0.5 - 0.5 * np.cos(A)
            crease = np.exp(-((S - s_ball - 0.006) / 0.016) ** 2) * top * np.sin(A * 7 + 3 * fbm(Po, 0.02, seed))
            cap = 1 - L.smoothstep(cap_e(Po[..., 0], Po[..., 1]), 0.92, 1.06)
            return 0.00035 * crease * (1 - cap) + 0.00012 * fbm(Po, 0.007, seed + 1) * (1 - cap) + CAP_T * cap

        cfg = dict(
            floor_in=FLOOR_IN, floor_out=FLOOR_OUT, tip_gap=0.0072, round_out=0.0030,
            gap=lambda S, A, Z: 0.0016 + 0.0032 * L.smoothstep(S, last.s_f0 + 0.045, last.s_f0 + 0.085)
            * (0.5 - 0.5 * np.cos(A)),
            min_gap=lambda S, A, Z: np.full(S.shape, 0.0013),
            topline=lambda a: 0.05475 - 0.0085 * np.cos(a) + 0.01175 * np.cos(2 * a) - 0.003 * np.sin(a),
            slit_half=0.19, hole_in=0.155, tongue_half=0.50, tab_h=0.020, slit_len=0.062,
            step=0.0013, wall=0.0019, pad=0.0011, relief=relief, zhide=lambda x, y: zfox(x, y) + lift(y) - 0.0040,
            toe=toe, cols=48, n_in=(7, 8), n_out=44)
        up = laced_upper(b, last, surf, cfg, SNK_CANVAS, SNK_LINING, SNK_RIM, log=log)
        sole_cfg = dict(
            style="foxing", cols=64, ref_z=np.linspace(0.002, 0.0115, 6),
            ext=lambda x, y: np.full(len(x), 0.0014), r_edge=0.0018, t_rub=0.0012, z_w1=-0.0010,
            zb=lambda x, y: Z_BOT + lift(y) + rock(y) + 0.0015 * L.smoothstep(y, y_heel - 0.014, y_heel + 0.008),
            zfox=zfox, lift=lambda x, y: lift(y), n_edge=3, n_wall=3, n_hug=6, n_bottom=6)
        sole = sole_unit(b, V, last.sgn, up["surf_out"], sole_cfg, SNK_RUBBER, log=log)
        # eyelets, lacing and bow
        holes = _holes(last, up, cfg["slit_half"] + cfg["hole_in"], 0.008, 0.0115, 5)
        hp = _hole_points(last, up, holes)
        for p, nrm in hp:
            eyelet(mb, p + nrm * 0.00025, nrm, SNK_EYELET, 0.0029, 0.0017, 0.0007, segs=12, k=4)
        lr = 0.00055
        lacer = Lacer(mb, last, up, SNK_LACE, band_profile(8, 0.0068, 2 * lr, p=0.2), lr, raise_=0.0008, n_pts=8)
        criss_cross(lacer, holes, lift=0.0015)
        ks = up["s_throat"] - 0.003
        for sg in (-1, 1):
            lacer.seg(holes[sg][0], (ks, math.pi + 0.03 * sg), extra=0.0006 * (sg > 0), dive=(True, False))
        lacer.n = 14
        kp = lacer.bow(ks, loops=[(0.72, 0.030, 0.0022), (0.66, 0.027, 0.0018)],
                       tails=[(0.30, 0.062, 0.55), (0.26, 0.055, 0.62)], knot_size=0.0078,
                       width=0.0058, thick=0.0011)
        b.feet[side] = dict(last=last, **{k: up[k] for k in ("rin", "rout", "s_top", "cols")})
        b.tex_extra = dict(ball_y=y_toe + 0.075, toe_y=y_toe, heel_y=y_heel, tip_y=y_tip, toe_x=x_toe,
                           floor_out=FLOOR_OUT, z_bot=Z_BOT, holes=np.array([p for p, _ in hp]),
                           slit_half_m=0.19 * 0.035, knot=kp, cap_ax=CAP_AX, cap_ay=CAP_AY,
                           slit_len=cfg["slit_len"])
        b.info[side] = dict(**up["info"], **sole["info"], eyelet_pairs=5, toe_spring_mm=1000 * (RISE + ROCK))
        log(f"sneakers {side}: {b.info[side]}")
    b.mb.mirror_x()
    b.sole_thickness_mm = -1000 * Z_BOT
    b.footbed_z_mm = 1000 * FLOOR_IN
    return b


# ================================================================= ankle boots

def ankle_boots(V, T, surf, log=print, seed=0):
    b = Built("SKM_AnkleBoots")
    mb = b.mb
    FLOOR_IN, FLOOR_OUT, Z_BOT = -0.0005, -0.0045, -0.0095
    Z_WELT, Z_WELT_BOT = -0.0012, -0.0042
    TOE_PAST, RISE, ROCK, WALL = 0.0150, 0.0050, 0.0028, 0.0021
    for side in "l":          # the right foot is its mirror image (body symmetric to 0.2 mm)
        last = L.Last(V, T, side, z_top=0.138, log=log)
        y_toe, y_heel = last.y_toe, last.y_heel
        y_ball = y_toe + 0.075
        y_breast = y_heel - 0.064
        y_tip = y_toe - TOE_PAST - WALL
        lift = spring(y_toe, y_tip - 0.0014, RISE)
        rock = rocker(y_ball + 0.005, y_toe, ROCK)
        stadium = Stadium(V, last.sgn)
        toe = dict(y_ref=y_toe + 0.040, y_end=y_toe - TOE_PAST, spring=lift, apex_u=0.36, apex_frac=0.42, blend=0.04, relax=14,
                   lat=(1.35, 1.7), med=(2.4, 1.8), top=(2.2, 1.8), bot=(4.0, 2.2), n_top=1.9, n_side=2.05)

        def relief(last_, S, A, rout):
            Po = last_.point(S, A, rout)
            s_ball = last_.s_f0 + (last_.y_p - (y_toe + 0.075))
            top = 0.5 - 0.5 * np.cos(A)
            vamp = np.exp(-((S - s_ball - 0.004) / 0.014) ** 2) * top * np.sin(A * 6 + 2.5 * fbm(Po, 0.02, seed))
            fr = (1 - L.smoothstep(np.abs(A - math.pi), 0.6, 1.3))
            ankle = np.exp(-((Po[..., 2] - 0.072) / 0.012) ** 2) * fr * np.sin(Po[..., 2] / 0.0055 * math.pi
                                                                               + 2 * fbm(Po, 0.02, seed + 2))
            return 0.00012 * vamp + 0.00016 * ankle + 0.00006 * fbm(Po, 0.009, seed + 1)

        cfg = dict(
            floor_in=FLOOR_IN, floor_out=FLOOR_OUT, tip_gap=0.0085, round_out=0.0022,
            gap=lambda S, A, Z: 0.0016 + 0.0026 * L.smoothstep(S, last.s_f0 + 0.045, last.s_f0 + 0.085)
            * (0.5 - 0.5 * np.cos(A)) + 0.0002 * L.smoothstep(Z, 0.05, 0.085),
            min_gap=lambda S, A, Z: np.full(S.shape, 0.0013),
            topline=lambda a: 0.13325 + 0.005 * np.cos(a) + 0.00075 * np.cos(2 * a) - 0.0015 * np.sin(a),
            slit_half=0.15, hole_in=0.14, tongue_half=0.42, tab_h=0.009, slit_len=0.124,
            step=0.0010, wall=WALL, pad=0.0006, relief=relief, zhide=lambda x, y: Z_WELT - 0.0004 + lift(y),
            smooth_in=40, min_off=0.0006, square_below=0.012, stadium=stadium, toe=toe, in_dense=0.115,
            cols=48, n_in=(9, 9), n_out=44)
        up = laced_upper(b, last, surf, cfg, BOOT_CALF, BOOT_LINING, BOOT_RIM, log=log)

        def ext(x, y):
            fore = 1 - L.smoothstep(y, y_ball + 0.005, y_ball + 0.045)
            heel = L.smoothstep(y, y_breast - 0.02, y_breast + 0.005)
            return 0.0012 + 0.0004 * fore + 0.0002 * heel

        def zb(x, y):
            waist = L.smoothstep(y, y_ball + 0.012, y_ball + 0.07) * (1 - L.smoothstep(y, y_breast - 0.0020, y_breast))
            return Z_BOT + 0.0018 * waist + lift(y) + rock(y)

        sole_cfg = dict(
            style="welt", cols=52, ref_z=np.linspace(Z_WELT, Z_WELT + 0.0025, 3), ext=ext, r_edge=0.0010,
            r_top=0.0008, z_welt=Z_WELT, zb=zb, lift=lambda x, y: lift(y), stadium=stadium, extra_y=[y_breast], n_edge=2,
            n_wall=4, n_top=2, n_welt=2, n_bottom=6)
        sole = sole_unit(b, V, last.sgn, up["surf_out"], sole_cfg, BOOT_SOLE, log=log)
        holes = _holes(last, up, cfg["slit_half"] + cfg["hole_in"], 0.008, 0.0122, 9)
        hp = _hole_points(last, up, holes)
        for p, nrm in hp:
            eyelet(mb, p + nrm * 0.0002, nrm, BOOT_EYELET, 0.0021, 0.0012, 0.0006, segs=8, k=4)
        lr = 0.0013
        lacer = Lacer(mb, last, up, BOOT_LACE, G.ellipse_profile(6, 2 * lr, 2 * lr * 0.9), lr, raise_=0.0006,
                      n_pts=8)
        criss_cross(lacer, holes, lift=0.0024)
        ks = up["s_throat"] - 0.003
        for sg in (-1, 1):
            lacer.seg(holes[sg][0], (ks, math.pi + 0.03 * sg), extra=0.0009 * (sg > 0), dive=(True, False))
        lacer.n = 13
        kp = lacer.bow(ks, loops=[(0.58, 0.024, 0.0016), (0.52, 0.022, 0.0014)],
                       tails=[(0.28, 0.050, 0.45), (0.24, 0.044, 0.52)], knot_size=0.0070,
                       width=2 * lr, thick=2 * lr)
        b.feet[side] = dict(last=last, **{k: up[k] for k in ("rin", "rout", "s_top", "cols")})
        b.tex_extra = dict(ball_y=y_ball, toe_y=y_toe, heel_y=y_heel, breast_y=y_breast, floor_out=FLOOR_OUT,
                           z_bot=Z_BOT, z_welt=Z_WELT, z_welt_bot=Z_WELT_BOT, holes=np.array([p for p, _ in hp]),
                           slit_end=_front_point(last, up, up["s_slit_end"]), slit_len=cfg["slit_len"],
                           slit_half_m=cfg["slit_half"] * 0.036, knot=kp, lift_p=(y_toe, y_tip - 0.0014, RISE))
        b.info[side] = dict(**up["info"], **sole["info"], eyelet_pairs=9, toe_spring_mm=1000 * (RISE + ROCK),
                            heel_breast_y_mm=float(1000 * y_breast))
        log(f"ankle boots {side}: {b.info[side]}")
    b.mb.mirror_x()
    b.sole_thickness_mm = -1000 * Z_BOT
    b.footbed_z_mm = 1000 * FLOOR_IN
    return b
