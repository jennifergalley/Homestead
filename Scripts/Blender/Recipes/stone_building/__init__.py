"""Stone building kit (``stone_wall.py``, ``stone_doorway.py``, ``stone_foundation.py``,
``stone_roof.py``): shared masonry geometry, split out because four recipes use it.

Pieces are laid out in **Unreal local space** (cm on paper, meters in code) and mirrored
to Blender at the end: Blender's FBX export (-Y forward, Z up) plus Unreal's FBX import
negates Y, so Blender (x, y, z) arrives as Unreal (x, -y, z). The kit therefore reads the
same as the game's wooden building pieces (``HomesteadWorld.cpp``: wall at local Y +144).
"""
