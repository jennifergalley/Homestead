"""Per-frame clearance between the heroine's carried props and her body, measured in PIE.

    from homestead_agent import prop_clearance as pc
    pc.start()                       # begin sampling every frame
    ...  LabAction Sticks  ...
    print(pc.stop())                 # worst penetration per prop and body part

Each visible carried stick is a capsule along its mesh's long axis (local Y), radius from its
scaled bounds. Her body is capsules between bones with radii close to her MetaHuman proportions.
A negative clearance (cm) means the prop is inside that body part. The hand that grips a stick
is excluded for that stick while it is in the hand (contact is the point there), and the left forearm
for sticks cradled on it (they rest on it by construction).
"""
import builtins
import unreal

# (name, start bone, end bone, radius cm). Torso radii cover her chest and belly depth in front of
# the spine joints; limbs are skin-surface radii.
BODY = (
    ('pelvis', 'pelvis', 'spine_03', 13.0),
    ('belly', 'spine_03', 'spine_05', 13.5),
    ('chest', 'spine_05', 'neck_01', 12.5),
    ('head', 'neck_02', 'head', 10.0),
    ('upperarm_l', 'upperarm_l', 'lowerarm_l', 4.8),
    ('forearm_l', 'lowerarm_l', 'hand_l', 3.8),
    ('hand_l', 'hand_l', 'middle_03_l', 2.8),
    ('upperarm_r', 'upperarm_r', 'lowerarm_r', 4.8),
    ('forearm_r', 'lowerarm_r', 'hand_r', 3.8),
    ('hand_r', 'hand_r', 'middle_03_r', 2.8),
    ('thigh_l', 'thigh_l', 'calf_l', 7.5),
    ('shin_l', 'calf_l', 'foot_l', 5.0),
    ('thigh_r', 'thigh_r', 'calf_r', 7.5),
    ('shin_r', 'calf_r', 'foot_r', 5.0),
)
STICK_RADIUS = 3.0


def _segment_distance(p1, q1, p2, q2):
    """Exact closest distance between segments p1-q1 and p2-q2 (unreal.Vector)."""
    d1, d2, r = q1 - p1, q2 - p2, p1 - p2
    a, e, f = d1.dot(d1), d2.dot(d2), d2.dot(r)
    if a <= 1e-9 and e <= 1e-9:
        return (p1 - p2).length()
    if a <= 1e-9:
        s, t = 0.0, min(max(f / e, 0.0), 1.0)
    else:
        c = d1.dot(r)
        if e <= 1e-9:
            t, s = 0.0, min(max(-c / a, 0.0), 1.0)
        else:
            b = d1.dot(d2)
            denom = a * e - b * b
            s = min(max((b * f - c * e) / denom, 0.0), 1.0) if denom > 1e-9 else 0.0
            t = (b * s + f) / e
            if t < 0.0:
                t, s = 0.0, min(max(-c / a, 0.0), 1.0)
            elif t > 1.0:
                t, s = 1.0, min(max((b - c) / a, 0.0), 1.0)
    return ((p1 + d1 * s) - (p2 + d2 * t)).length()


def _world():
    return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()


def stick_capsule(component):
    """(start, end) of a stick prop's long axis in world space."""
    mesh = component.static_mesh
    bounds = mesh.get_bounds()
    xf = component.get_world_transform()
    centre = xf.transform_location(bounds.origin)
    axis = xf.transform_direction(unreal.Vector(0, bounds.box_extent.y, 0))
    return centre - axis, centre + axis


def sample(character):
    body = character.mesh
    bones = {b: body.get_socket_location(b) for _, a, c, _ in BODY for b in (a, c)}
    rows = []
    for stick in character.get_components_by_class(unreal.StaticMeshComponent):
        if not stick.static_mesh or 'DryBranches' not in stick.static_mesh.get_name() or not stick.is_visible():
            continue
        socket = str(stick.get_attach_socket_name())
        skip = {'hand_r', 'forearm_r'} if socket == 'hand_r' else {'forearm_l'} if socket == 'lowerarm_l' else set()
        start, end = stick_capsule(stick)
        for name, a, c, radius in BODY:
            if name in skip:
                continue
            gap = _segment_distance(start, end, bones[a], bones[c]) - radius - STICK_RADIUS
            rows.append((stick.get_name(), name, gap))
    return rows


def start():
    world = _world()
    character = unreal.GameplayStatics.get_player_controller(world, 0).get_controlled_pawn()
    builtins._prop_clearance = {'log': [], 't0': unreal.GameplayStatics.get_time_seconds(world)}

    def tick(_dt):
        t = unreal.GameplayStatics.get_time_seconds(world) - builtins._prop_clearance['t0']
        for stick, part, gap in sample(character):
            builtins._prop_clearance['log'].append((round(t, 3), stick, part, gap))

    builtins._prop_clearance['handle'] = unreal.register_slate_post_tick_callback(tick)


def stop(threshold=0.0):
    """Stop sampling. Returns {(stick, part): (worst gap cm, time s, frames below threshold)}."""
    state = builtins._prop_clearance
    unreal.unregister_slate_post_tick_callback(state['handle'])
    worst = {}
    for t, stick, part, gap in state['log']:
        key = (stick, part)
        best, when, count = worst.get(key, (1e9, 0.0, 0))
        if gap < best:
            best, when = gap, t
        worst[key] = (best, when, count + (gap < threshold))
    return {k: (round(v[0], 1), v[1], v[2]) for k, v in sorted(worst.items(), key=lambda kv: kv[1][0]) if v[0] < 6.0}
