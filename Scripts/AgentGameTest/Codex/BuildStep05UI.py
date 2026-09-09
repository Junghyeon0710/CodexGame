"""Build STEP 5 CommonUI assets for PROJECT: LAST STAND.

Run with UnrealEditor-Cmd after the CodexGameEditor module has been built.
Only the exact assets in GENERATED_ASSETS are replaced when ``--force`` is used.
"""

from __future__ import annotations

import json
import sys
from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir())
REPORT_PATH = PROJECT_ROOT / "Saved" / "AgentGameTest" / "Codex" / "Step05UIBuildReport.json"
ROOT = "/Game/AgentGameTest/Codex/UI"
LAYOUT_DIR = f"{ROOT}/Layout"
HUD_DIR = f"{ROOT}/HUD"
MENU_DIR = f"{ROOT}/Menu"
RESULT_DIR = f"{ROOT}/Result"
COMMON_DIR = f"{ROOT}/Common"
STYLE_DIR = f"{ROOT}/Styles"

ASSET_PATHS = {
    "style": f"{STYLE_DIR}/BP_UI_LastStandButtonStyle_Codex",
    "button": f"{COMMON_DIR}/W_CommonButton_Codex",
    "layout": f"{LAYOUT_DIR}/W_PrimaryGameLayout_Codex",
    "hud": f"{HUD_DIR}/W_GameHUD_Codex",
    "main": f"{MENU_DIR}/W_MainMenu_Codex",
    "pause": f"{MENU_DIR}/W_PauseMenu_Codex",
    "game_over": f"{RESULT_DIR}/W_GameOver_Codex",
    "victory": f"{RESULT_DIR}/W_Victory_Codex",
}
GENERATED_ASSETS = tuple(ASSET_PATHS.values())


def color(r: int, g: int, b: int, a: int = 255) -> unreal.LinearColor:
    return unreal.LinearColor(r / 255.0, g / 255.0, b / 255.0, a / 255.0)


WHITE = color(239, 242, 243)
MUTED = color(151, 164, 170)
ACCENT = color(240, 166, 49)
ACCENT_SOFT = color(240, 166, 49, 90)
BLUE = color(62, 133, 166)
RED = color(198, 55, 49)
GREEN = color(78, 183, 121)
PANEL = color(13, 20, 24, 235)
PANEL_LIGHT = color(28, 39, 45, 240)
WASH = color(3, 7, 9, 205)
TRANSPARENT = color(255, 255, 255, 0)


def name(value: str | None) -> unreal.Name:
    return unreal.Name(value or "None")


def slate_color(value: unreal.LinearColor) -> unreal.SlateColor:
    result = unreal.SlateColor()
    result.set_editor_property("specified_color", value)
    return result


def brush(fill: unreal.LinearColor, outline=TRANSPARENT, width: float = 0.0, radius: float = 3.0):
    result = unreal.SlateBrush()
    result.set_editor_property("draw_as", unreal.SlateBrushDrawType.ROUNDED_BOX)
    result.set_editor_property("tint_color", slate_color(fill))
    settings = unreal.SlateBrushOutlineSettings()
    settings.set_editor_property("corner_radii", unreal.Vector4(radius, radius, radius, radius))
    settings.set_editor_property("rounding_type", unreal.SlateBrushRoundingType.FIXED_RADIUS)
    settings.set_editor_property("color", slate_color(outline))
    settings.set_editor_property("width", width)
    result.set_editor_property("outline_settings", settings)
    return result


def add(bp, widget_class, widget_name: str, parent: str | None = None):
    widget = unreal.EditorUtilityLibrary.add_source_widget(
        bp, widget_class, name(widget_name), name(parent)
    )
    if not widget:
        raise RuntimeError(f"Failed to add {widget_name} below {parent or '<root>'}")
    return widget


def fill(widget):
    slot = widget.get_editor_property("slot")
    if slot:
        if hasattr(slot, "set_horizontal_alignment"):
            slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        if hasattr(slot, "set_vertical_alignment"):
            slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
    return widget


def canvas(widget, position, size, alignment=(0.0, 0.0), anchor=(0.0, 0.0), z=None):
    slot = widget.get_editor_property("slot")
    anchors = unreal.Anchors()
    anchors.set_editor_property("minimum", unreal.Vector2D(*anchor))
    anchors.set_editor_property("maximum", unreal.Vector2D(*anchor))
    slot.set_anchors(anchors)
    slot.set_auto_size(False)
    slot.set_position(unreal.Vector2D(*position))
    slot.set_size(unreal.Vector2D(*size))
    slot.set_alignment(unreal.Vector2D(*alignment))
    if z is not None and hasattr(slot, "set_z_order"):
        slot.set_z_order(z)
    return widget


def configure_border(widget, fill_color, outline=TRANSPARENT, width=0.0, radius=3.0):
    widget.set_editor_property("background", brush(fill_color, outline, width, radius))
    widget.set_brush_color(color(255, 255, 255))
    widget.set_padding(unreal.Margin(left=0, top=0, right=0, bottom=0))
    return widget


def configure_text(widget, value: str, size: int, tint=WHITE, justify=unreal.TextJustify.LEFT):
    widget.set_text(value)
    font = widget.get_editor_property("font")
    font.set_editor_property("size", size)
    roboto = unreal.load_asset("/Engine/EngineFonts/Roboto.Roboto")
    if roboto:
        font.set_editor_property("font_object", roboto)
    widget.set_editor_property("font", font)
    widget.set_color_and_opacity(slate_color(tint))
    widget.set_editor_property("justification", justify)
    widget.set_auto_wrap_text(False)
    return widget


def text(bp, parent, widget_name, value, position, size_xy, font_size, tint=WHITE,
         justify=unreal.TextJustify.LEFT, anchor=(0.0, 0.0), alignment=(0.0, 0.0), z=None):
    widget = configure_text(
        add(bp, unreal.CommonTextBlock, widget_name, parent), value, font_size, tint, justify
    )
    return canvas(widget, position, size_xy, alignment, anchor, z)


def border(bp, parent, widget_name, position, size_xy, fill_color, outline=TRANSPARENT,
           width=0.0, radius=3.0, anchor=(0.0, 0.0), alignment=(0.0, 0.0), z=None):
    widget = configure_border(
        add(bp, unreal.Border, widget_name, parent), fill_color, outline, width, radius
    )
    return canvas(widget, position, size_xy, alignment, anchor, z)


def create_widget(asset_path: str, parent_class):
    directory, asset_name = asset_path.rsplit("/", 1)
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, directory, unreal.WidgetBlueprint, factory
    )
    if not bp:
        raise RuntimeError(f"Failed to create {asset_path}")
    return bp


def compile_save(bp):
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    if not bp.generated_class():
        raise RuntimeError(f"{bp.get_name()} generated no class")
    status = str(bp.get_editor_property("status"))
    if "ERROR" in status.upper():
        raise RuntimeError(f"{bp.get_name()} compile status is {status}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False):
        raise RuntimeError(f"Could not save {bp.get_path_name()}")
    return bp


def load_native(path: str):
    cls = unreal.load_class(None, path)
    if not cls:
        raise RuntimeError(f"Native class unavailable: {path}")
    return cls


def make_design_root(bp, prefix: str):
    root = add(bp, unreal.ScaleBox, f"{prefix}ScaleBox")
    root.set_stretch(unreal.Stretch.SCALE_TO_FIT)
    root.set_stretch_direction(unreal.StretchDirection.BOTH)
    design = add(bp, unreal.SizeBox, f"{prefix}DesignSize", f"{prefix}ScaleBox")
    design.set_width_override(1920.0)
    design.set_height_override(1080.0)
    add(bp, unreal.Overlay, f"{prefix}Overlay", f"{prefix}DesignSize")
    fill(add(bp, unreal.CanvasPanel, f"{prefix}Canvas", f"{prefix}Overlay"))
    return f"{prefix}Canvas", f"{prefix}Overlay"


def create_style():
    parent = load_native("/Script/CommonUI.CommonButtonStyle")
    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        ASSET_PATHS["style"].rsplit("/", 1)[1], STYLE_DIR, unreal.Blueprint, factory
    )
    if not bp:
        raise RuntimeError("Failed to create CommonButtonStyle")
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.modify()
    cdo.set_editor_property("single_material", False)
    cdo.set_editor_property("normal_base", brush(color(22, 31, 36, 245), color(75, 89, 95), 1.0))
    cdo.set_editor_property("normal_hovered", brush(color(36, 52, 60, 250), ACCENT, 1.5))
    cdo.set_editor_property("normal_pressed", brush(color(85, 62, 28, 250), ACCENT, 2.0))
    cdo.set_editor_property("selected_base", brush(color(36, 52, 60, 250), ACCENT, 1.5))
    cdo.set_editor_property("selected_hovered", brush(color(43, 62, 70, 250), ACCENT, 2.0))
    cdo.set_editor_property("selected_pressed", brush(color(85, 62, 28, 250), ACCENT, 2.0))
    cdo.set_editor_property("disabled", brush(color(22, 25, 27, 180), color(45, 50, 52), 1.0))
    padding = unreal.Margin(left=4, top=4, right=4, bottom=4)
    cdo.set_editor_property("button_padding", padding)
    cdo.set_editor_property("custom_padding", unreal.Margin(left=20, top=10, right=20, bottom=10))
    bp.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
    return bp


def create_common_button(style_class):
    bp = create_widget(
        ASSET_PATHS["button"], load_native("/Script/CodexGame.CodexLSCommonButton")
    )
    root = add(bp, unreal.SizeBox, "ButtonSize")
    root.set_width_override(360.0)
    root.set_height_override(64.0)
    add(bp, unreal.Overlay, "ButtonOverlay", "ButtonSize")
    accent = configure_border(add(bp, unreal.Border, "ButtonAccent", "ButtonOverlay"), ACCENT)
    slot = accent.get_editor_property("slot")
    slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
    slot.set_padding(unreal.Margin(left=0, top=0, right=354, bottom=0))
    label = configure_text(
        add(bp, unreal.CommonTextBlock, "ButtonLabel", "ButtonOverlay"),
        "ACTION", 24, WHITE, unreal.TextJustify.CENTER
    )
    fill(label)
    compile_save(bp)
    cdo = unreal.get_default_object(bp.generated_class())
    cdo.modify()
    cdo.set_editor_property("style", style_class)
    bp.modify()
    unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
    return bp


def create_primary_layout():
    bp = create_widget(
        ASSET_PATHS["layout"], load_native("/Script/CodexGame.CodexLSPrimaryGameLayout")
    )
    add(bp, unreal.Overlay, "RootOverlay")
    for layer_name in ("GameLayer", "MenuLayer", "ModalLayer"):
        fill(add(bp, unreal.CommonActivatableWidgetStack, layer_name, "RootOverlay"))
    return compile_save(bp)


def progress(bp, parent, widget_name, position, size_xy, percent, tint, anchor=(0.0, 0.0),
             alignment=(0.0, 0.0)):
    widget = add(bp, unreal.ProgressBar, widget_name, parent)
    widget.set_percent(percent)
    widget.set_fill_color_and_opacity(tint)
    try:
        widget.set_editor_property("background_image", brush(color(18, 26, 30, 230), color(69, 80, 84), 1.0, 2.0))
        widget.set_editor_property("fill_image", brush(tint, tint, 0.0, 2.0))
    except Exception:
        pass
    return canvas(widget, position, size_xy, alignment, anchor)


def create_hud():
    bp = create_widget(ASSET_PATHS["hud"], load_native("/Script/CodexGame.CodexLSHUDWidget"))
    root_canvas, root_overlay = make_design_root(bp, "HUD")

    border(bp, root_canvas, "TopWavePanel", (0, 34), (430, 76), PANEL,
           color(83, 97, 103, 180), 1.0, 4.0, (0.5, 0.0), (0.5, 0.0))
    text(bp, root_canvas, "WaveText", "PREPARING", (0, 43), (390, 36), 27, WHITE,
         unreal.TextJustify.CENTER, (0.5, 0.0), (0.5, 0.0), 2)
    text(bp, root_canvas, "EnemyCountText", "ENEMIES ALIVE  0", (0, 79), (390, 24), 15, MUTED,
         unreal.TextJustify.CENTER, (0.5, 0.0), (0.5, 0.0), 2)

    border(bp, root_canvas, "ScorePanel", (-56, 38), (320, 66), PANEL,
           color(83, 97, 103, 180), 1.0, 4.0, (1.0, 0.0), (1.0, 0.0))
    text(bp, root_canvas, "ScoreCaption", "MISSION SCORE", (-350, 49), (140, 18), 12, MUTED,
         unreal.TextJustify.LEFT, (1.0, 0.0), (0.0, 0.0), 2)
    text(bp, root_canvas, "ScoreText", "SCORE  0", (-350, 67), (270, 30), 24, ACCENT,
         unreal.TextJustify.LEFT, (1.0, 0.0), (0.0, 0.0), 2)

    border(bp, root_canvas, "PhasePanel", (56, 38), (250, 66), PANEL,
           color(83, 97, 103, 180), 1.0, 4.0)
    text(bp, root_canvas, "PhaseCaption", "COMBAT STATUS", (76, 49), (200, 18), 12, MUTED)
    text(bp, root_canvas, "PhaseText", "NONE", (76, 67), (200, 30), 22, WHITE)

    border(bp, root_canvas, "HealthPanel", (56, -52), (500, 104), PANEL,
           color(83, 97, 103, 180), 1.0, 4.0, (0.0, 1.0), (0.0, 1.0))
    text(bp, root_canvas, "HealthCaption", "VITAL SIGNS", (78, -132), (180, 20), 13, MUTED,
         anchor=(0.0, 1.0), alignment=(0.0, 1.0), z=2)
    text(bp, root_canvas, "HealthText", "100 / 100", (78, -105), (210, 32), 27, WHITE,
         anchor=(0.0, 1.0), alignment=(0.0, 1.0), z=2)
    progress(bp, root_canvas, "HealthBar", (78, -70), (456, 18), 1.0, RED,
             (0.0, 1.0), (0.0, 1.0))

    border(bp, root_canvas, "DashPanel", (0, -52), (420, 104), PANEL,
           color(83, 97, 103, 180), 1.0, 4.0, (0.5, 1.0), (0.5, 1.0))
    text(bp, root_canvas, "DashCaption", "SPACE  //  EVASIVE DRIVE", (0, -132), (372, 20), 13,
         MUTED, unreal.TextJustify.CENTER, (0.5, 1.0), (0.5, 1.0), 2)
    text(bp, root_canvas, "DashText", "DASH  READY", (0, -105), (372, 32), 25, WHITE,
         unreal.TextJustify.CENTER, (0.5, 1.0), (0.5, 1.0), 2)
    progress(bp, root_canvas, "DashProgressBar", (0, -70), (372, 12), 1.0, BLUE,
             (0.5, 1.0), (0.5, 1.0))

    text(bp, root_canvas, "ControlHint", "WASD MOVE   //   MOUSE AIM   //   LMB FIRE   //   ESC PAUSE",
         (-56, -54), (620, 20), 12, MUTED, unreal.TextJustify.RIGHT,
         (1.0, 1.0), (1.0, 1.0), 2)

    border(bp, root_canvas, "AnnouncementPlate", (0, -5), (620, 92), color(8, 13, 16, 218),
           ACCENT_SOFT, 1.0, 3.0, (0.5, 0.42), (0.5, 0.5), 5)
    announcement = text(
        bp, root_canvas, "AnnouncementText", "GET READY", (0, -4), (570, 56), 42,
        ACCENT, unreal.TextJustify.CENTER, (0.5, 0.42), (0.5, 0.5), 6
    )
    announcement.set_visibility(unreal.SlateVisibility.COLLAPSED)
    return compile_save(bp)


def placed_button(bp, parent, widget_class, style_class, widget_name, label, position, size_xy,
                  anchor=(0.0, 0.0), alignment=(0.0, 0.0), z=3):
    widget = add(bp, widget_class, widget_name, parent)
    widget.set_editor_property("style", style_class)
    try:
        widget.set_editor_property("button_text", label)
    except Exception:
        pass
    return canvas(widget, position, size_xy, alignment, anchor, z)


def add_full_wash(bp, root_canvas, tint=WASH):
    return border(bp, root_canvas, "ScreenWash", (0, 0), (1920, 1080), tint, z=0)


def create_main_menu(button_class, style_class):
    bp = create_widget(
        ASSET_PATHS["main"], load_native("/Script/CodexGame.CodexLSMainMenuWidget")
    )
    root_canvas, root_overlay = make_design_root(bp, "Main")
    add_full_wash(bp, root_canvas, color(4, 9, 12, 238))

    border(bp, root_canvas, "LeftAmberRail", (0, 0), (18, 1080), ACCENT, z=1)
    border(bp, root_canvas, "TopRule", (88, 86), (1744, 2), color(91, 105, 111, 150), z=1)
    border(bp, root_canvas, "BottomRule", (88, 988), (1744, 2), color(91, 105, 111, 150), z=1)
    text(bp, root_canvas, "ProjectEyebrow", "PROJECT", (132, 188), (500, 55), 34, MUTED, z=2)
    text(bp, root_canvas, "GameTitle", "LAST STAND", (124, 228), (900, 132), 94, WHITE, z=2)
    text(bp, root_canvas, "MissionSubtitle", "OLD INDUSTRIAL SECTOR  //  SURVIVAL PROTOCOL",
         (132, 365), (780, 30), 18, ACCENT, z=2)
    text(bp, root_canvas, "GameDescription",
         "THREE WAVES. NO EXTRACTION. HOLD THE LINE.",
         (132, 421), (780, 34), 21, MUTED, z=2)

    border(bp, root_canvas, "MenuPanel", (122, 568), (410, 204), color(11, 18, 22, 210),
           color(77, 92, 98, 180), 1.0, 4.0, z=1)
    placed_button(bp, root_canvas, button_class, style_class, "PlayButton", "PLAY",
                  (146, 590), (360, 64))
    placed_button(bp, root_canvas, button_class, style_class, "ExitButton", "EXIT",
                  (146, 680), (360, 64))

    border(bp, root_canvas, "SectorPanel", (-104, 188), (530, 584), color(17, 27, 32, 190),
           color(68, 83, 89, 140), 1.0, 5.0, (1.0, 0.0), (1.0, 0.0), 1)
    text(bp, root_canvas, "SectorCode", "SECTOR 07", (-580, 230), (420, 48), 32, ACCENT,
         anchor=(1.0, 0.0), z=2)
    text(bp, root_canvas, "MissionLabel", "MISSION BRIEF", (-580, 302), (420, 28), 17, MUTED,
         anchor=(1.0, 0.0), z=2)
    text(bp, root_canvas, "MissionLine1", "SURVIVE ALL HOSTILE WAVES", (-580, 346), (420, 34),
         20, WHITE, anchor=(1.0, 0.0), z=2)
    text(bp, root_canvas, "MissionLine2", "MAINTAIN COMBAT MOBILITY", (-580, 392), (420, 34),
         20, WHITE, anchor=(1.0, 0.0), z=2)
    text(bp, root_canvas, "MissionLine3", "SECURE THE INDUSTRIAL YARD", (-580, 438), (420, 34),
         20, WHITE, anchor=(1.0, 0.0), z=2)
    text(bp, root_canvas, "ControlBlock",
         "WASD  MOVE\nMOUSE  AIM\nLMB  FIRE\nSPACE  DASH",
         (-580, 526), (420, 170), 20, MUTED, anchor=(1.0, 0.0), z=2)
    text(bp, root_canvas, "FooterText", "CODEX COMBAT SYSTEM  //  BUILD STEP 05",
         (132, 1010), (720, 24), 13, MUTED, z=2)
    return compile_save(bp)


def create_pause_menu(button_class, style_class):
    bp = create_widget(
        ASSET_PATHS["pause"], load_native("/Script/CodexGame.CodexLSPauseMenuWidget")
    )
    root_canvas, root_overlay = make_design_root(bp, "Pause")
    add_full_wash(bp, root_canvas, color(2, 6, 8, 205))
    border(bp, root_canvas, "PausePanel", (0, 0), (600, 620), PANEL,
           color(92, 106, 112, 190), 1.0, 5.0, (0.5, 0.5), (0.5, 0.5), 1)
    border(bp, root_canvas, "PauseAccent", (0, -307), (600, 6), ACCENT,
           anchor=(0.5, 0.5), alignment=(0.5, 0.5), z=2)
    text(bp, root_canvas, "PauseTitle", "PAUSED", (0, -230), (520, 76), 54, WHITE,
         unreal.TextJustify.CENTER, (0.5, 0.5), (0.5, 0.5), 2)
    text(bp, root_canvas, "PauseSubtitle", "COMBAT SIMULATION SUSPENDED", (0, -165), (520, 28),
         16, MUTED, unreal.TextJustify.CENTER, (0.5, 0.5), (0.5, 0.5), 2)
    placed_button(bp, root_canvas, button_class, style_class, "ResumeButton", "RESUME",
                  (0, -66), (360, 64), (0.5, 0.5), (0.5, 0.5))
    placed_button(bp, root_canvas, button_class, style_class, "RestartButton", "RESTART",
                  (0, 24), (360, 64), (0.5, 0.5), (0.5, 0.5))
    placed_button(bp, root_canvas, button_class, style_class, "MainMenuButton", "MAIN MENU",
                  (0, 114), (360, 64), (0.5, 0.5), (0.5, 0.5))
    text(bp, root_canvas, "PauseHint", "ESC  RESUME", (0, 234), (400, 28), 15, MUTED,
         unreal.TextJustify.CENTER, (0.5, 0.5), (0.5, 0.5), 2)
    return compile_save(bp)


def create_result(asset_key, native_class, title_value, accent_color, button_class, style_class):
    bp = create_widget(ASSET_PATHS[asset_key], load_native(native_class))
    prefix = "Victory" if asset_key == "victory" else "GameOver"
    root_canvas, root_overlay = make_design_root(bp, prefix)
    add_full_wash(bp, root_canvas, color(2, 6, 8, 225))
    border(bp, root_canvas, "ResultPanel", (0, 0), (700, 690), PANEL,
           color(92, 106, 112, 190), 1.0, 5.0, (0.5, 0.5), (0.5, 0.5), 1)
    border(bp, root_canvas, "ResultAccent", (0, -342), (700, 7), accent_color,
           anchor=(0.5, 0.5), alignment=(0.5, 0.5), z=2)
    text(bp, root_canvas, "ResultKicker", "MISSION STATUS", (0, -256), (590, 28), 16, MUTED,
         unreal.TextJustify.CENTER, (0.5, 0.5), (0.5, 0.5), 2)
    text(bp, root_canvas, "ResultTitleText", title_value, (0, -194), (610, 86), 62,
         accent_color, unreal.TextJustify.CENTER, (0.5, 0.5), (0.5, 0.5), 2)
    text(bp, root_canvas, "FinalScoreLabel", "FINAL SCORE", (0, -74), (400, 28), 17, MUTED,
         unreal.TextJustify.CENTER, (0.5, 0.5), (0.5, 0.5), 2)
    text(bp, root_canvas, "FinalScoreText", "0", (0, -12), (440, 76), 52, WHITE,
         unreal.TextJustify.CENTER, (0.5, 0.5), (0.5, 0.5), 2)
    placed_button(bp, root_canvas, button_class, style_class, "RestartButton", "RESTART",
                  (0, 120), (360, 64), (0.5, 0.5), (0.5, 0.5))
    placed_button(bp, root_canvas, button_class, style_class, "MainMenuButton", "MAIN MENU",
                  (0, 210), (360, 64), (0.5, 0.5), (0.5, 0.5))
    return compile_save(bp)


def validate(bp, required_names):
    missing = []
    classes = {}
    for widget_name in required_names:
        widget = unreal.EditorUtilityLibrary.find_source_widget_by_name(bp, name(widget_name))
        if not widget:
            missing.append(widget_name)
        else:
            classes[widget_name] = widget.get_class().get_name()
    if missing:
        raise RuntimeError(f"{bp.get_name()} missing widgets: {', '.join(missing)}")
    return {
        "asset": bp.get_path_name(),
        "generated_class": bp.generated_class().get_path_name(),
        "widgets": classes,
    }


def configure_arena_game_mode():
    bp = unreal.load_asset("/Game/AgentGameTest/Codex/Blueprints/BP_GameMode_Arena_Codex")
    if not bp or not bp.generated_class():
        raise RuntimeError("Arena GameMode Blueprint unavailable")
    cdo = unreal.get_default_object(bp.generated_class())
    changed = False
    for property_name in ("start_with_main_menu", "b_start_with_main_menu"):
        try:
            cdo.set_editor_property(property_name, True)
            changed = True
            break
        except Exception:
            continue
    if not changed:
        raise RuntimeError("Could not set bStartWithMainMenu on Arena GameMode")
    cdo.modify()
    bp.modify()
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    unreal.EditorAssetLibrary.save_loaded_asset(bp, only_if_is_dirty=False)
    return bp.get_path_name()


def main():
    command_line = unreal.SystemLibrary.get_command_line().lower()
    force = "--force" in sys.argv or "-step05uiforce" in command_line
    for directory in (LAYOUT_DIR, HUD_DIR, MENU_DIR, RESULT_DIR, COMMON_DIR, STYLE_DIR):
        unreal.EditorAssetLibrary.make_directory(directory)

    existing = [p for p in GENERATED_ASSETS if unreal.EditorAssetLibrary.does_asset_exist(p)]
    if existing and not force:
        raise RuntimeError(
            "STEP 5 assets already exist and were preserved. Use --force to replace only: "
            + ", ".join(existing)
        )
    if force:
        for asset_path in reversed(GENERATED_ASSETS):
            if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
                if not unreal.EditorAssetLibrary.delete_asset(asset_path):
                    raise RuntimeError(f"Could not replace {asset_path}")

    style_bp = create_style()
    style_class = style_bp.generated_class()
    button_bp = create_common_button(style_class)
    button_class = button_bp.generated_class()
    layout_bp = create_primary_layout()
    hud_bp = create_hud()
    main_bp = create_main_menu(button_class, style_class)
    pause_bp = create_pause_menu(button_class, style_class)
    game_over_bp = create_result(
        "game_over", "/Script/CodexGame.CodexLSGameOverWidget", "GAME OVER", RED,
        button_class, style_class
    )
    victory_bp = create_result(
        "victory", "/Script/CodexGame.CodexLSVictoryWidget", "VICTORY", GREEN,
        button_class, style_class
    )

    validations = [
        validate(layout_bp, ("GameLayer", "MenuLayer", "ModalLayer")),
        validate(hud_bp, (
            "HealthBar", "HealthText", "WaveText", "EnemyCountText", "ScoreText",
            "DashProgressBar", "DashText", "PhaseText", "AnnouncementText", "AnnouncementPlate",
        )),
        validate(main_bp, ("PlayButton", "ExitButton", "GameTitle")),
        validate(pause_bp, ("ResumeButton", "RestartButton", "MainMenuButton", "PauseTitle")),
        validate(game_over_bp, (
            "ResultTitleText", "FinalScoreText", "RestartButton", "MainMenuButton",
        )),
        validate(victory_bp, (
            "ResultTitleText", "FinalScoreText", "RestartButton", "MainMenuButton",
        )),
        validate(button_bp, ("ButtonLabel",)),
    ]
    arena_game_mode = configure_arena_game_mode()
    unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)

    report = {
        "success": True,
        "design_resolution": [1920, 1080],
        "generated_assets": list(GENERATED_ASSETS),
        "asset_count": len(GENERATED_ASSETS),
        "widget_blueprint_count": 7,
        "style_blueprint_count": 1,
        "common_ui": {
            "root_layers": ["UI.Layer.Game", "UI.Layer.Menu", "UI.Layer.Modal"],
            "common_button": ASSET_PATHS["button"],
            "common_button_style": ASSET_PATHS["style"],
        },
        "arena_game_mode": arena_game_mode,
        "validations": validations,
    }
    REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
    REPORT_PATH.write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log(f"CODEX_STEP5_UI_BUILD_SUCCESS {json.dumps(report, ensure_ascii=False)}")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        unreal.log_error(f"CODEX_STEP5_UI_BUILD_FAILED {type(exc).__name__}: {exc}")
        raise
