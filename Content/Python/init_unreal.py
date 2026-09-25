"""Registers Homestead's agent-play toolset when the editor runs with the MCP/Toolset Registry plugins.

Ordinary editor sessions and commandlets (for example Scripts\\bootstrap_unreal.py) don't load the
Toolset Registry, so this does nothing there.
"""
import unreal

if hasattr(unreal, "ToolsetDefinition"):
    try:
        from homestead_agent import toolset

        toolset.register()
    except Exception as error:  # Never block editor startup for an optional agent tool.
        unreal.log_warning(f"Homestead agent toolset not registered: {error}")
