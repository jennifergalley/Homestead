"""Anatomy audit of every heroine clip (realistic-animation skill): runs joint_limits on each baked clip
and writes one markdown report with the issues by clip and severity.

    from homestead_agent import anim_audit
    anim_audit.run()                                    # every clip below
    anim_audit.run(['AN_HeroineMH_ScytheMow'], every=2)  # a few, every other frame

Editor Python only (it loads AnimSequences). The report goes under E:\\CopilotScratch (OUT_ROOT), never
the repo. Clips with an authoring recipe pass its FRAMES as named key frames, and the strike or impact
frames as contacts, where a fast joint is only a warning.
"""
import datetime
import importlib
import os

import unreal

from homestead_agent import joint_limits as jl

ANIMATIONS = '/Game/Characters/Heroine_MH/Animations'
OUT_ROOT = os.environ.get('HOMESTEAD_ANIM_AUDIT_DIR', r'E:\CopilotScratch\anim-audit')

# (asset, recipe module or None, contact FRAMES keys). Recipe-less clips are retargeted or imported.
CLIPS = [
    ('AN_HeroineMH_ActiveIdle', 'active_idle', ()),
    ('AN_HeroineMH_LivingIdle02', None, ()),
    ('AN_HeroineMH_GASP_Walk', None, ()),
    ('AN_HeroineMH_GASP_WalkRelaxed', None, ()),
    ('AN_HeroineMH_GASP_Run', None, ()),
    ('AN_HeroineMH_GASP_Sprint', None, ()),
    ('AN_HeroineMH_GASP_SprintRelaxed', None, ()),
    ('AN_HeroineMH_KneelGatherSticks', 'kneel_gather', ()),
    ('AN_HeroineMH_KneelGatherPouch', 'kneel_pouch', ()),
    ('AN_HeroineMH_KneelPlant', 'kneel_plant', ('poke', 'press', 'pat')),
    ('AN_HeroineMH_KneelHarvest', 'kneel_harvest', ()),
    ('AN_HeroineMH_KneelCutReeds', 'kneel_reeds', ('cut',)),
    ('AN_HeroineMH_KneelPullWeeds', 'kneel_pull_weeds', ()),
    ('AN_HeroineMH_PailFill', 'pail_fill', ()),
    ('AN_HeroineMH_PailPour', 'pail_pour', ()),
    ('AN_HeroineMH_Eat', 'eat_berry', ()),
    ('AN_HeroineMH_CraftHands', 'craft_hands', ()),
    ('AN_HeroineMH_LampSetDown', None, ()),
    ('AN_HeroineMH_AxeFell', 'axe_fell', ('strike1', 'bite1', 'strike2', 'bite2')),
    ('AN_HeroineMH_GroundStrike', 'ground_strike', ('strike1', 'bite1', 'strike2', 'bite2')),
    ('AN_HeroineMH_ScytheMow', 'scythe_mow', ('strike1', 'bite1', 'strike2', 'bite2')),
    ('AN_HeroineMH_MacheteHack', 'machete_hack', ('strike1', 'strike2')),
    ('AN_HeroineMH_HoeTill', 'hoe_till', ('chop1', 'bite1', 'chop2', 'bite2')),
    ('AN_HeroineMH_KnifeCut', None, ()),
    ('AN_HeroineMH_Chop', None, ()),
    ('AN_HeroineMH_Till', None, ()),
    ('AN_HeroineMH_WaterRefined', None, ()),
]


def _recipe(module):
    if not module:
        return {}
    try:
        mod = importlib.import_module(f'homestead_agent.{module}')
    except Exception as error:  # a recipe that can't import still gets audited without key frames
        unreal.log_warning(f'anim_audit: {module} not importable ({error})')
        return {}
    return dict(getattr(mod, 'FRAMES', {}) or {})


def audit(asset, module=None, contact_keys=(), every=1, neutral=None):
    """(summary lines, issue counts {(kind, severity): n}) for one clip, or None if it doesn't load."""
    anim = unreal.load_asset(f'{ANIMATIONS}/{asset}')
    if anim is None:
        return None
    events = _recipe(module)
    contacts = [k for k in contact_keys if k in events]
    frames_n = int(round(anim.get_play_length() * jl.FPS))
    indices = list(range(0, frames_n + 1, every))
    frames = [jl.pose_at(anim, i / jl.FPS) for i in indices]
    scaled = {k: v // every for k, v in events.items()}
    result = jl.check_frames(frames, neutral, fps=jl.FPS / every, events=scaled,
                             contacts=[scaled[k] for k in contacts])
    keys = {}
    for name, i in scaled.items():
        keys.setdefault(i, name)
    counts = {}
    for it in result['issues']:
        counts[(it['kind'], it['severity'])] = counts.get((it['kind'], it['severity']), 0) + 1
    return jl.summarize(result, keys, limit=30), counts


def run(names=None, every=1):
    """Audit the listed clips (default: all of CLIPS) and write audit.md; returns its path."""
    stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S')
    out_dir = os.path.join(OUT_ROOT, stamp)
    os.makedirs(out_dir, exist_ok=True)
    neutral = jl.neutral_pose()
    lines = [f'# Heroine animation anatomy audit {stamp}', '',
             'joint_limits against the realistic-animation skill bands. Errors are beyond the extreme band, '
             'warnings between the comfortable and extreme bands.', '',
             '| Clip | Errors | Warnings | Worst |', '|---|---:|---:|---|']
    details = []
    for asset, module, contact_keys in CLIPS:
        if names and asset not in names:
            continue
        found = audit(asset, module, contact_keys, every, neutral)
        if found is None:
            lines.append(f'| {asset} | - | - | not found |')
            continue
        summary, counts = found
        errors = sum(n for (k, s), n in counts.items() if s == 'error')
        warns = sum(n for (k, s), n in counts.items() if s == 'warn')
        worst = summary[1].strip() if len(summary) > 1 else 'clean'
        lines.append(f'| {asset} | {errors} | {warns} | {worst.replace("|", "/")} |')
        details += ['', f'## {asset}', '', '```', *summary, '```']
        unreal.log(f'anim_audit {asset}: {errors} errors, {warns} warnings')
    path = os.path.join(out_dir, 'audit.md')
    with open(path, 'w', encoding='utf-8') as handle:
        handle.write('\n'.join(lines + details) + '\n')
    unreal.log(f'anim_audit wrote {path}')
    return path
