"""Animation Inspector sheets: turn one recording into overlays, contact sheets, a GIF and index.md.

    python Scripts/anim_inspector_sheet.py <recording dir> [--recipe axe_fell] [--contacts strike1,bite1] [--thumb 320]

The recording comes from AHomesteadAnimInspector (Scripts\\Inspect-Animation.ps1): frames.json (component-space
bone transforms per frame, the mesh's world transform, held props, the ground under the feet), neutral.json
(the reference pose) and cameras.json, plus <view>/fNNNN.png captures. Every frame is judged with
homestead_agent.joint_limits (realistic-animation skill); each captured view gets an overlay:
- the skeleton coloured by status (green fine, amber strained, red beyond the extreme band or a pop);
- red rings on contacts below the floor and on sliding feet;
- the centre of mass (cross) over the support polygon (outline) on the floor;
- held props' bounds (cyan boxes).
Outputs go beside the recording: <view>/oNNNN.png, sheet_<view>.png, keyframes.png, motion.gif, index.md.
Images are sized so an agent's image viewer can open them (sheets at most about 2000 px wide).
"""
import argparse
import json
import math
import os
import sys

from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'Content', 'Python'))
from homestead_agent import joint_limits as jl  # noqa: E402

GREEN, AMBER, RED = (60, 200, 90), (255, 170, 30), (235, 40, 40)
CYAN, WHITE, BLACK = (40, 210, 230), (255, 255, 255), (0, 0, 0)
# The bones drawn: the body chain, the arms, legs, feet and the fingers' main joints.
DRAWN = ['pelvis', 'spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05', 'neck_01', 'neck_02', 'head']
for _s in 'lr':
    DRAWN += [f'{b}_{_s}' for b in ('clavicle', 'upperarm', 'lowerarm', 'hand', 'thigh', 'calf', 'foot', 'ball',
                                     'thumb_01', 'thumb_02', 'thumb_03')]
    DRAWN += [f'{f}_{k}_{_s}' for f in jl.FINGERS for k in ('01', '02', '03')]
SHEET_COLUMNS = 6
SHEET_MAX_TILES = 30
KEYFRAME_ROWS = 10


def issue_bones(key):
    """The drawn bones a joint_limits issue key colours."""
    joint = key.split('.')[0]
    side = joint[-2:] if joint.endswith(('_l', '_r')) else ''
    base = joint[:-2] if side else joint
    simple = {'elbow': ['lowerarm'], 'wrist': ['hand'], 'forearm': ['lowerarm', 'hand'], 'shoulder': ['upperarm'],
              'clavicle': ['clavicle'], 'hip': ['thigh'], 'knee': ['calf'], 'ankle': ['foot'], 'toes': ['ball']}
    if base in simple:
        return [f'{b}{side}' for b in simple[base]]
    for f in list(jl.FINGERS) + ['thumb']:
        for k, bone in (('mcp', '01'), ('pip', '02'), ('dip', '03'), ('ip', '03')):
            if base == f'{f}_{k}':
                return [f'{f}_{bone if f != "thumb" or k != "mcp" else "02"}{side}']
    if base == 'lumbar':
        return ['spine_01', 'spine_02']
    if base == 'spine':
        return ['spine_01', 'spine_02', 'spine_03', 'spine_04', 'spine_05']
    if base == 'neck':
        return ['neck_01', 'neck_02', 'head']
    return [joint]


def load(directory):
    with open(os.path.join(directory, 'frames.json'), encoding='utf-8') as f:
        rec = json.load(f)
    with open(os.path.join(directory, 'neutral.json'), encoding='utf-8') as f:
        neutral = json.load(f)
    with open(os.path.join(directory, 'cameras.json'), encoding='utf-8') as f:
        cams = json.load(f)
    return rec, neutral, cams


def to_pose(bones, values):
    pose = {}
    for name, v in zip(bones, values):
        if v is not None:
            pose[name] = ((v[0], v[1], v[2]), (v[3], v[4], v[5], v[6]))
    return pose


def world(mesh, point):
    loc, q = (mesh[0], mesh[1], mesh[2]), (mesh[3], mesh[4], mesh[5], mesh[6])
    return jl.add(loc, jl.q_rotate(q, point))


def project(cam, size, p):
    d = jl.sub(p, tuple(cam['location']))
    x, y, z = jl.dot(d, tuple(cam['right'])), jl.dot(d, tuple(cam['up'])), jl.dot(d, tuple(cam['forward']))
    if cam['ortho']:
        scale = size / cam['width']
        return (size * 0.5 + x * scale, size * 0.5 - y * scale)
    if z <= 1.0:
        return None
    focal = size * 0.5 / math.tan(math.radians(cam['fov']) * 0.5)
    return (size * 0.5 + x / z * focal, size * 0.5 - y / z * focal)


def drawn_parents(parents):
    """Each drawn bone's nearest drawn ancestor."""
    out = {}
    for bone in DRAWN:
        p = parents.get(bone)
        while p and p not in DRAWN:
            p = parents.get(p)
        if p:
            out[bone] = p
    return out


def floor_component_z(rec):
    """The floor under her feet in component space, from the first frame's ground traces."""
    first = rec['frames'][0]
    zs = [z for z in first['ground'].values() if z is not None]
    if not zs:
        return 0.0
    mesh = first['mesh']
    # Component z of the floor (the mesh is upright: only yaw, so z maps straight across).
    return min(zs) - mesh[2]


def overlay(image, cam, size, frame, pose, status, links, com, support, label):
    draw = ImageDraw.Draw(image)
    mesh = frame['mesh']
    pts = {b: project(cam, size, world(mesh, pose[b][0])) for b in DRAWN if b in pose}
    for child, parent in links.items():
        a, b = pts.get(parent), pts.get(child)
        if a and b:
            colour = {'error': RED, 'warn': AMBER}.get(status.get(child), GREEN)
            draw.line([a, b], fill=BLACK, width=6)
            draw.line([a, b], fill=colour, width=3)
    for bone, p in pts.items():
        if p:
            r = 3 if '_0' in bone else 4
            draw.ellipse([p[0] - r, p[1] - r, p[0] + r, p[1] + r], fill={'error': RED, 'warn': AMBER}.get(status.get(bone), WHITE))
    for prop in frame.get('props', []):
        c, e = prop['centre'], prop['extent']
        corners = [(c[0] + sx * e[0], c[1] + sy * e[1], c[2] + sz * e[2]) for sx in (-1, 1) for sy in (-1, 1) for sz in (-1, 1)]
        pp = [project(cam, size, p) for p in corners]
        for i in range(8):
            for j in range(i + 1, 8):
                if bin(i ^ j).count('1') == 1 and pp[i] and pp[j]:
                    draw.line([pp[i], pp[j]], fill=CYAN, width=1)
    if support:
        ring = [project(cam, size, world(mesh, p)) for p in support]
        ring = [p for p in ring if p]
        if len(ring) >= 2:
            draw.line(ring + [ring[0]], fill=WHITE, width=2)
    if com:
        c = project(cam, size, world(mesh, com[0]))
        g = project(cam, size, world(mesh, com[1]))
        for p, colour in ((c, WHITE), (g, com[2])):
            if p:
                draw.line([p[0] - 7, p[1], p[0] + 7, p[1]], fill=colour, width=3)
                draw.line([p[0], p[1] - 7, p[0], p[1] + 7], fill=colour, width=3)
        if c and g:
            draw.line([c, g], fill=WHITE, width=1)
    for bone in [b for b, s in status.items() if s == 'ring']:
        p = pts.get(bone)
        if p:
            draw.ellipse([p[0] - 12, p[1] - 12, p[0] + 12, p[1] + 12], outline=RED, width=3)
    draw.rectangle([0, 0, size, 22], fill=(0, 0, 0))
    draw.text((6, 5), label, fill=WHITE)
    return image


def recipe_frames(module):
    """A recipe's FRAMES dict, read from its source (the recipes import unreal, so they can't be imported
    here). Handles a literal dict and dict(<alias>.FRAMES) from another recipe."""
    import ast
    folder = os.path.join(ROOT, 'Content', 'Python', 'homestead_agent')
    with open(os.path.join(folder, f'{module}.py'), encoding='utf-8') as f:
        tree = ast.parse(f.read())
    aliases = {}
    for node in tree.body:
        if isinstance(node, ast.ImportFrom) and node.module == 'homestead_agent':
            for name in node.names:
                aliases[name.asname or name.name] = name.name
    for node in tree.body:
        if isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'FRAMES' for t in node.targets):
            value = node.value
            if isinstance(value, ast.Dict):
                return ast.literal_eval(value)
            if (isinstance(value, ast.Call) and value.args and isinstance(value.args[0], ast.Attribute)
                    and isinstance(value.args[0].value, ast.Name) and value.args[0].value.id in aliases):
                return recipe_frames(aliases[value.args[0].value.id])
    return {}


def run(directory, recipe=None, contacts=(), thumb=320):
    rec, neutral_json, cams = load(directory)
    bones = rec['bones']
    every = rec.get('every', 1)
    neutral = {b: ((v[0], v[1], v[2]), (v[3], v[4], v[5], v[6])) for b, v in neutral_json['bones'].items()}
    links = drawn_parents(neutral_json['parents'])
    poses = [to_pose(bones, f['pose']) for f in rec['frames']]
    events = recipe_frames(recipe) if recipe else {}
    contact_frames = [events[c] for c in contacts if c in events]
    floor = floor_component_z(rec)
    result = jl.check_frames(poses, neutral, fps=30.0, events=events, contacts=contact_frames, floor_z=floor)
    by_frame = {}
    for it in result['issues']:
        by_frame.setdefault(it['frame'], []).append(it)
    names = {v: k for k, v in events.items()}
    size = cams['resolution']
    outputs = {v['name']: [] for v in cams['views']}
    for i, (frame, pose) in enumerate(zip(rec['frames'], poses)):
        if i % every:
            continue
        status = {}
        for it in by_frame.get(i, []):
            for b in issue_bones(it['joint']):
                if status.get(b) != 'error':
                    status[b] = 'error' if it['severity'] == 'error' else 'warn'
            if it['kind'] in ('ground', 'slide'):
                status[it['joint']] = 'ring'
        com = jl.centre_of_mass(pose)
        support = jl.convex_hull(jl.support_points(pose, floor))
        com_marker = None
        if com:
            outside = jl.distance_outside((com[0], com[1]), support) if support else float('inf')
            com_marker = (com, (com[0], com[1], floor), GREEN if outside == 0 else RED)
        support3 = [(p[0], p[1], floor) for p in support]
        n_err = sum(1 for it in by_frame.get(i, []) if it['severity'] == 'error')
        n_warn = len(by_frame.get(i, [])) - n_err
        tag = f' [{names[i]}]' if i in names else ''
        label = f"{rec['action']} f{i} {frame['t']:.2f}s{tag}  errors {n_err} warnings {n_warn}"
        for cam in cams['views']:
            src = os.path.join(directory, cam['name'], f'f{i:04d}.png')
            if not os.path.exists(src):
                continue
            with Image.open(src) as im:
                image = im.convert('RGB')
            overlay(image, cam, size, frame, pose, status, links, com_marker, support3, label)
            out = os.path.join(directory, cam['name'], f'o{i:04d}.png')
            image.save(out)
            outputs[cam['name']].append((i, out))
    # Contact sheets per view.
    sheets = []
    for name, items in outputs.items():
        if not items:
            continue
        step = max(1, math.ceil(len(items) / SHEET_MAX_TILES))
        picked = items[::step]
        rows = math.ceil(len(picked) / SHEET_COLUMNS)
        sheet = Image.new('RGB', (SHEET_COLUMNS * thumb, rows * thumb), (30, 30, 30))
        for k, (_, path) in enumerate(picked):
            with Image.open(path) as im:
                tile = im.resize((thumb, thumb))
            sheet.paste(tile, ((k % SHEET_COLUMNS) * thumb, (k // SHEET_COLUMNS) * thumb))
        path = os.path.join(directory, f'sheet_{name}.png')
        sheet.save(path)
        sheets.append(path)
    # Key frames: the recipe's beats and the frames with the most errors, every view side by side.
    flagged = sorted(by_frame, key=lambda f: -sum(1 for it in by_frame[f] if it['severity'] == 'error'))
    keys = sorted(set([f for f in events.values() if f % every == 0] + [f - f % every for f in flagged[:6]]))[:KEYFRAME_ROWS]
    views = [n for n in outputs if outputs[n]]
    if keys and views:
        sheet = Image.new('RGB', (len(views) * thumb, len(keys) * thumb), (30, 30, 30))
        for r, f in enumerate(keys):
            for c, name in enumerate(views):
                path = os.path.join(directory, name, f'o{f:04d}.png')
                if os.path.exists(path):
                    with Image.open(path) as im:
                        sheet.paste(im.resize((thumb, thumb)), (c * thumb, r * thumb))
        sheet.save(os.path.join(directory, 'keyframes.png'))
    # Motion: the three-quarter view (or the first) at gameplay speed.
    motion = outputs.get('threequarter') or (outputs[views[0]] if views else [])
    if motion:
        gif = []
        for _, p in motion:
            with Image.open(p) as im:
                gif.append(im.resize((thumb + 64, thumb + 64)))
        gif[0].save(os.path.join(directory, 'motion.gif'), save_all=True, append_images=gif[1:],
                    duration=int(1000 * every / 30), loop=0)
    # index.md
    lines = [f"# Animation Inspector: {rec['action']}", '',
             f"{len(rec['frames'])} frames at 30 fps, captured every {every}. Views: {', '.join(views)}.", '',
             '- Sheets: ' + ', '.join(f'[{os.path.basename(p)}]({os.path.basename(p)})' for p in sheets),
             '- Key frames: [keyframes.png](keyframes.png); motion: [motion.gif](motion.gif)', '', '## Summary', '', '```',
             *jl.summarize(result, names, limit=30), '```', '', '## Flagged frames', '',
             '| Frame | Beat | Errors | Warnings | Worst |', '|---:|---|---:|---:|---|']
    for f in sorted(by_frame):
        its = by_frame[f]
        errors = [it for it in its if it['severity'] == 'error']
        worst = max(errors or its, key=lambda it: abs(it['value']))
        lines.append(f"| {f} | {names.get(f, '')} | {len(errors)} | {len(its) - len(errors)} | {worst['text'].replace('|', '/')} |")
    with open(os.path.join(directory, 'index.md'), 'w', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')
    return os.path.join(directory, 'index.md')


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument('directory')
    parser.add_argument('--recipe', help='homestead_agent module whose FRAMES name the beats (e.g. axe_fell)')
    parser.add_argument('--contacts', default='', help='comma-separated FRAMES keys of strikes or impacts')
    parser.add_argument('--thumb', type=int, default=320)
    args = parser.parse_args(argv)
    contacts = [c for c in args.contacts.split(',') if c]
    print(run(args.directory, args.recipe, contacts, args.thumb))


if __name__ == '__main__':
    main()
