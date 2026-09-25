[CmdletBinding()]
param([Parameter(Mandatory, Position = 0)][string]$Name)
# Loads a built asset set (Assets\Props\<Name>\<Name>.blend) into the live Blender
# window with photoreal EEVEE viewport shading under the review sky.
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$blend = Join-Path $root "Assets\Props\$Name\$Name.blend"
if (-not (Test-Path -LiteralPath $blend)) { throw "Not built yet: $blend" }
$code = @"
import bpy, sys, importlib
sys.path.insert(0, r'$PSScriptRoot')
import homestead_kit as kit
kit = importlib.reload(kit)
kit.reset()
with bpy.data.libraries.load(r'$blend', link=False) as (src, dst):
    dst.objects = [n for n in src.objects if n.startswith('SM_')]
shown = []
for obj in dst.objects:
    kit._link(obj)
    obj.hide_set('_LOD' in obj.name)
    if '_LOD' not in obj.name:
        shown.append(obj)
kit._sky(kit.DEFAULT_HDRI, rotation=40)
bpy.context.scene.render.engine = 'BLENDER_EEVEE'
bpy.context.scene.view_settings.view_transform = 'AgX'
kit.focus(shown)
for window in bpy.context.window_manager.windows:
    for area in window.screen.areas:
        if area.type == 'VIEW_3D':
            area.spaces.active.shading.type = 'RENDERED'
for obj in shown:
    obj.select_set(False)
print('SHOWN', [o.name for o in shown])
"@
& (Join-Path $PSScriptRoot 'Invoke-BlenderLive.ps1') $code
