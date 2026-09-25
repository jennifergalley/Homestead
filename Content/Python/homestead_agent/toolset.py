"""MCP-callable tools for playing Homestead in Play-In-Editor through real controller/keyboard input."""
import unreal

import toolset_registry
from toolset_registry.registration import Registration

_library = unreal.HomesteadAgentPlayLibrary


@unreal.uclass()
class HomesteadPlayTools(unreal.ToolsetDefinition):
    """Plays Homestead during Play-In-Editor by injecting simulated controller and keyboard input
    through the player controller (the same path as the game's own playtest harnesses), and reports
    player, menu and nearby-resource state. Start PIE first (EditorAppToolset.StartPIE).
    Input tools return immediately; poll get_play_state and capture images to observe results.
    """

    @toolset_registry.tool_call
    @staticmethod
    def get_play_state(nearby_count: int = 8, radius_cm: float = 4000.0) -> str:
        """Returns a JSON snapshot of the Play-In-Editor session.

        Includes whether the world is ready, menu state (bookOpen, bookPage, selectedRow,
        bookTitle, bookSummary, bookFooter), the focused interaction (focusTitle, focusActions),
        the latest toast message, needs (hunger, energy, warmth), hour, inventory summary,
        player location (cm), controlYaw, speed, walk status, and the nearest resource nodes with
        id, kind, x, y, distanceCm, bearingDeg (relative to the camera; positive is to the right),
        cleared and ready.

        Args:
            nearby_count: Maximum number of nearby resource nodes to list.
            radius_cm: Search radius for nearby resource nodes, in centimeters.
        """
        return _library.get_play_state(nearby_count, radius_cm)

    @toolset_registry.tool_call
    @staticmethod
    def tap_key(key: str) -> str:
        """Presses and releases one key or controller button.

        Keyboard: E or Enter interact/activate, F clear/weed/fuel/till, I or Tab open the book menu,
        Escape back/close, Up/Down/Left/Right navigate menus, C craft page, B build page, H notes,
        R rotate placement, 1-9 hotbar. Controller: Gamepad_FaceButton_Bottom (A),
        Gamepad_FaceButton_Right (B), Gamepad_FaceButton_Left (X), Gamepad_FaceButton_Top (Y),
        Gamepad_Special_Right (Menu), Gamepad_Special_Left (View), Gamepad_LeftShoulder (LB),
        Gamepad_RightShoulder (RB), Gamepad_DPad_Up/Down/Left/Right. Any Unreal FKey name works.

        Args:
            key: Unreal key name, e.g. "E", "Tab", "Escape", "Gamepad_FaceButton_Bottom".
        """
        return _library.tap_key(key)

    @toolset_registry.tool_call
    @staticmethod
    def hold_key(key: str, seconds: float) -> str:
        """Holds a key or button down for a duration, then releases it (e.g. LeftShift to sprint).

        Args:
            key: Unreal key name.
            seconds: How long to hold it.
        """
        return _library.hold_key(key, seconds)

    @toolset_registry.tool_call
    @staticmethod
    def set_sticks(move_x: float, move_y: float, look_x: float, look_y: float, seconds: float) -> str:
        """Holds the controller sticks for a duration, then centers them. Cancels walk_to.

        Args:
            move_x: Left stick X, -1 (left) to 1 (right), relative to the camera.
            move_y: Left stick Y, -1 (back) to 1 (forward), relative to the camera.
            look_x: Right stick X, -1 (turn left) to 1 (turn right).
            look_y: Right stick Y, -1 (look down) to 1 (look up).
            seconds: How long to apply the input.
        """
        return _library.set_sticks(move_x, move_y, look_x, look_y, seconds)

    @toolset_registry.tool_call
    @staticmethod
    def walk_to(x: float, y: float, stop_distance_cm: float = 150.0, timeout_seconds: float = 20.0) -> str:
        """Walks the character toward a world XY point with the left stick, turning the camera toward it.

        Stops within stop_distance_cm, after timeout_seconds, or when progress stalls. Poll
        get_play_state until "walk" is "arrived", "timeout", "stuck" or "cancelled". Use a
        resource's x/y from get_play_state.nearbyResources.

        Args:
            x: World X in centimeters.
            y: World Y in centimeters.
            stop_distance_cm: Distance at which to stop.
            timeout_seconds: Maximum walking time.
        """
        return _library.walk_to(x, y, stop_distance_cm, timeout_seconds)

    @toolset_registry.tool_call
    @staticmethod
    def release_all() -> str:
        """Centers the sticks, releases held keys and cancels walk_to."""
        return _library.release_all()


_registration = Registration([HomesteadPlayTools])


def _python_allowed() -> bool:
    return "-homesteadagentpython" in unreal.SystemLibrary.get_command_line().lower()


@unreal.uclass()
class HomesteadEditorPython(unreal.ToolsetDefinition):
    """Runs arbitrary Python in the editor for development automation (full `unreal` API).
    Registered only when the editor was started with Scripts\\Start-EditorMcp.ps1 -AllowPython.
    """

    @toolset_registry.tool_call
    @staticmethod
    def run_python(code: str) -> str:
        """Executes Python in the editor's interpreter on the game thread and returns captured output.

        The code runs with `unreal` imported. Anything printed is returned; assign a JSON-serializable
        value to `result` to return it as well. Exceptions are returned as a traceback string.
        Long-running work blocks the editor; keep calls short.

        Args:
            code: Python source to execute.
        """
        import contextlib
        import io
        import json
        import traceback

        scope = {"unreal": unreal, "__name__": "__homestead_agent__"}
        output = io.StringIO()
        try:
            with contextlib.redirect_stdout(output), contextlib.redirect_stderr(output):
                exec(compile(code, "<run_python>", "exec"), scope)
        except Exception:
            output.write(traceback.format_exc())
        if "result" in scope:
            try:
                output.write("\nresult: " + json.dumps(scope["result"], default=str))
            except Exception as error:
                output.write(f"\nresult not serializable: {error}")
        return output.getvalue()


if _python_allowed():
    _registration = Registration([HomesteadPlayTools, HomesteadEditorPython])


def register() -> bool:
    return _registration.register()
