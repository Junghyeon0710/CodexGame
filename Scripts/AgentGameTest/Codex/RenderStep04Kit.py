"""Render the existing Codex Blender kit as a labelled 1600 x 1200 contact sheet.

Run with the authored LS_Codex_Environment.blend open. Existing meshes, transforms,
UVs and materials are preserved. Only explicitly named preview camera/light/stage
data are added or refreshed, and all temporary render visibility is restored.
This script does not save the .blend. PNG composition uses the Python standard
library, so neither Pillow nor external image tools are required by Blender.
"""

import json
import math
import struct
import tempfile
import zlib
from pathlib import Path

import bpy
from mathutils import Vector


ROOT = Path(r"D:\Unreal Projects\CodexGame")
MODELS = ROOT / "ExternalAssets/LastStand/Codex/Models"
OUTPUT = ROOT / "Docs/AgentComparison/Codex/Evidence/Step04_BlenderKit.png"
SCENE_NAME = "LS_Codex_Environment"
SOURCE_OWNER = "Codex_LastStand_Step04_BlenderKit_v1"
PREVIEW_OWNER = "Codex_LastStand_Step04_KitPreview_v1"
OWNER_KEY = "last_stand_generator"
PREFIX = "LS_Codex_KitPreview_"
WIDTH, HEIGHT = 1600, 1200
CARD_WIDTH, CARD_HEIGHT = 376, 258
TILE_WIDTH, TILE_HEIGHT = 376, 203
MARGIN, GAP, HEADER = 24, 16, 96

LABELS = {
    "Container": "CONTAINER",
    "ConcreteBarrier": "CONCRETE BARRIER",
    "PipeSkid": "PIPE SKID",
    "UtilityBox": "UTILITY BOX",
    "Barrel": "BARREL",
    "Pallet": "PALLET",
    "FenceSection": "CHAIN-LINK FENCE",
    "Warehouse": "WAREHOUSE",
    "Crate": "CRATE",
    "IndustrialLamp": "INDUSTRIAL LAMP",
    "GroundTile": "GROUND TILE",
    "Drain": "DRAIN",
    "Curb": "CURB",
}

# Compact monospaced bitmap lettering keeps the contact sheet independent from
# OS fonts and Blender's UI scale. Source render pixels are copied losslessly.
GLYPHS = {
    "A": [14,17,17,31,17,17,17], "B": [30,17,17,30,17,17,30],
    "C": [14,17,16,16,16,17,14], "D": [30,17,17,17,17,17,30],
    "E": [31,16,16,30,16,16,31], "F": [31,16,16,30,16,16,16],
    "G": [14,17,16,23,17,17,15], "H": [17,17,17,31,17,17,17],
    "I": [14,4,4,4,4,4,14], "J": [7,2,2,2,2,18,12],
    "K": [17,18,20,24,20,18,17], "L": [16,16,16,16,16,16,31],
    "M": [17,27,21,21,17,17,17], "N": [17,25,21,19,17,17,17],
    "O": [14,17,17,17,17,17,14], "P": [30,17,17,30,16,16,16],
    "Q": [14,17,17,17,21,18,13], "R": [30,17,17,30,20,18,17],
    "S": [15,16,16,14,1,1,30], "T": [31,4,4,4,4,4,4],
    "U": [17,17,17,17,17,17,14], "V": [17,17,17,17,17,10,4],
    "W": [17,17,17,21,21,21,10], "X": [17,17,10,4,10,17,17],
    "Y": [17,17,10,4,4,4,4], "Z": [31,1,2,4,8,16,31],
    "0": [14,17,19,21,25,17,14], "1": [4,12,4,4,4,4,14],
    "2": [14,17,1,2,4,8,31], "3": [30,1,1,14,1,1,30],
    "4": [2,6,10,18,31,2,2], "5": [31,16,16,30,1,1,30],
    "6": [14,16,16,30,17,17,14], "7": [31,1,2,4,8,8,8],
    "8": [14,17,17,14,17,17,14], "9": [14,17,17,15,1,1,14],
    "-": [0,0,0,31,0,0,0], "/": [1,2,2,4,8,8,16],
    ".": [0,0,0,0,0,12,12], ",": [0,0,0,0,0,4,8],
    ":": [0,12,12,0,12,12,0], "|": [4,4,4,4,4,4,4],
    " ": [0,0,0,0,0,0,0],
}


def tag(data):
    data[OWNER_KEY] = PREVIEW_OWNER
    return data


def check_preview(data, name):
    if data is not None and data.get(OWNER_KEY) != PREVIEW_OWNER:
        raise RuntimeError(f"Preview name collision with unowned Blender data: {name}")


def preview_object(scene, suffix, data_factory):
    name = PREFIX + suffix
    obj = bpy.data.objects.get(name)
    check_preview(obj, name)
    if obj is None:
        data = data_factory(name + "Data")
        tag(data)
        obj = tag(bpy.data.objects.new(name, data))
        scene.collection.objects.link(obj)
    if obj.name not in scene.objects:
        raise RuntimeError(f"Preview object is linked outside the owned scene: {name}")
    return obj


def look_at(obj, point):
    obj.rotation_euler = (Vector(point) - obj.location).to_track_quat("-Z", "Y").to_euler()


def create_preview(scene):
    camera = preview_object(scene, "Camera", bpy.data.cameras.new)
    camera.data.type = "ORTHO"
    camera.data.sensor_fit = "VERTICAL"
    camera.data.lens = 50
    camera.data.clip_start = 0.01
    camera.data.clip_end = 1000
    camera.data.dof.use_dof = False
    lights = []
    for suffix in ("Key", "Fill", "Rim"):
        light = preview_object(scene, suffix, lambda n: bpy.data.lights.new(n, "AREA"))
        light.data.shape = "DISK"
        light.data.use_shadow = True
        lights.append(light)

    def new_stage(name):
        mesh = bpy.data.meshes.new(name)
        mesh.from_pydata([(-1,-1,0), (1,-1,0), (1,1,0), (-1,1,0)], [], [(0,1,2,3)])
        mesh.update()
        return mesh

    stage = preview_object(scene, "Stage", new_stage)
    material_name = PREFIX + "StageMaterial"
    material = bpy.data.materials.get(material_name)
    check_preview(material, material_name)
    if material is None:
        material = tag(bpy.data.materials.new(material_name))
    material.use_nodes = True
    bsdf = material.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (0.145, 0.170, 0.180, 1)
    bsdf.inputs["Roughness"].default_value = 0.94
    stage.data.materials.clear()
    stage.data.materials.append(material)

    world_name = PREFIX + "World"
    world = bpy.data.worlds.get(world_name)
    check_preview(world, world_name)
    if world is None:
        world = tag(bpy.data.worlds.new(world_name))
    world.use_nodes = True
    background = world.node_tree.nodes.get("Background")
    background.inputs["Color"].default_value = (0.22, 0.27, 0.32, 1)
    background.inputs["Strength"].default_value = 0.65
    scene.camera, scene.world = camera, world
    return camera, lights, stage


def configure_render(scene):
    engines = scene.render.bl_rna.properties["engine"].enum_items.keys()
    engine = next((name for name in ("BLENDER_EEVEE_NEXT", "BLENDER_EEVEE") if name in engines), None)
    if engine is None:
        raise RuntimeError(f"EEVEE unavailable; registered engines: {list(engines)}")
    scene.render.engine = engine
    scene.render.resolution_x = TILE_WIDTH
    scene.render.resolution_y = TILE_HEIGHT
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGB"
    scene.render.image_settings.color_depth = "8"
    scene.render.image_settings.compression = 20
    scene.render.use_file_extension = True
    scene.render.use_compositing = False
    scene.render.use_sequencer = False
    scene.view_settings.view_transform = "AgX"
    scene.view_settings.exposure = 0
    scene.view_settings.gamma = 1
    # Recent EEVEE versions changed sampling properties. Use available controls.
    if hasattr(scene, "eevee"):
        if hasattr(scene.eevee, "taa_render_samples"):
            scene.eevee.taa_render_samples = 32
        if hasattr(scene.eevee, "use_gtao"):
            scene.eevee.use_gtao = True
            scene.eevee.gtao_distance = 3
            scene.eevee.gtao_factor = 1.15
    return engine


def frame_mesh(scene, obj, camera, lights, stage):
    points = [obj.matrix_world @ Vector(corner) for corner in obj.bound_box]
    minimum = Vector([min(point[i] for point in points) for i in range(3)])
    maximum = Vector([max(point[i] for point in points) for i in range(3)])
    centre = (minimum + maximum) * 0.5
    span = max((maximum - minimum).length, 0.7)
    direction = Vector((1.10, -1.40, 1.25)).normalized()
    camera.location = centre + direction * (span * 4 + 5)
    look_at(camera, centre)
    rotation = camera.rotation_euler.to_quaternion()
    right, up = rotation @ Vector((1,0,0)), rotation @ Vector((0,1,0))
    projected_x = [(point - centre).dot(right) for point in points]
    projected_y = [(point - centre).dot(up) for point in points]
    width = max(projected_x) - min(projected_x)
    height = max(projected_y) - min(projected_y)
    camera.data.ortho_scale = max(height, width * TILE_HEIGHT / TILE_WIDTH) * 1.23
    stage.location = (centre.x, centre.y, minimum.z - max(0.006, span * 0.002))
    stage.scale = (span * 3, span * 3, 1)
    for light, offset, energy, colour in zip(
        lights,
        ((-0.70,-0.90,1.60), (0.90,-0.10,1.00), (-0.20,1.00,1.25)),
        (1350, 680, 950),
        ((1.0,0.88,0.72), (0.70,0.83,1.0), (0.89,0.94,1.0)),
    ):
        light.location = centre + Vector(offset) * span
        light.data.energy = energy * (span / 3) ** 2
        light.data.size = span * 1.15
        light.data.color = colour
        look_at(light, centre)
    scene.view_layers[0].update()


def paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    return a if pa <= pb and pa <= pc else b if pb <= pc else c


def read_rgb_png(path):
    """Read Blender's non-interlaced RGB/RGBA 8-bit output, preserving RGB bytes."""
    data = path.read_bytes()
    if not data.startswith(b"\x89PNG\r\n\x1a\n"):
        raise RuntimeError(f"Invalid PNG render: {path}")
    offset, compressed, width, height, channels = 8, bytearray(), 0, 0, 0
    while offset < len(data):
        length = struct.unpack_from(">I", data, offset)[0]
        kind = data[offset+4:offset+8]
        chunk = data[offset+8:offset+8+length]
        if kind == b"IHDR":
            width, height, depth, colour, compression, filtering, interlace = struct.unpack(">IIBBBBB", chunk)
            if depth != 8 or colour not in (2, 6) or compression or filtering or interlace:
                raise RuntimeError(f"Unsupported Blender PNG format: {path}")
            channels = 3 if colour == 2 else 4
        elif kind == b"IDAT":
            compressed.extend(chunk)
        elif kind == b"IEND":
            break
        offset += length + 12
    packed = zlib.decompress(compressed)
    stride, previous, rgb = width * channels, bytearray(width * channels), bytearray()
    if len(packed) != height * (stride + 1):
        raise RuntimeError(f"Unexpected PNG scanline size: {path}")
    for y in range(height):
        offset = y * (stride + 1)
        filter_type = packed[offset]
        row = bytearray(packed[offset+1:offset+1+stride])
        for index in range(stride):
            left = row[index-channels] if index >= channels else 0
            above = previous[index]
            upper_left = previous[index-channels] if index >= channels else 0
            if filter_type == 1:
                predictor = left
            elif filter_type == 2:
                predictor = above
            elif filter_type == 3:
                predictor = (left + above) // 2
            elif filter_type == 4:
                predictor = paeth(left, above, upper_left)
            elif filter_type == 0:
                predictor = 0
            else:
                raise RuntimeError(f"Unsupported PNG row filter: {filter_type}")
            row[index] = (row[index] + predictor) & 255
        if channels == 3:
            rgb.extend(row)
        else:
            for index in range(0, stride, 4):
                rgb.extend(row[index:index+3])
        previous = row
    return width, height, rgb


def rect(canvas, x, y, width, height, colour):
    row = bytes(colour) * width
    for yy in range(max(y, 0), min(y + height, HEIGHT)):
        canvas[(yy * WIDTH + x) * 3:(yy * WIDTH + x + width) * 3] = row


def text(canvas, x, y, value, scale=2, colour=(219,228,228)):
    for letter in value.upper():
        if letter not in GLYPHS:
            raise ValueError(f"Missing label glyph: {letter}")
        for row, mask in enumerate(GLYPHS[letter]):
            for column in range(5):
                if mask & (1 << (4 - column)):
                    rect(canvas, x + column * scale, y + row * scale, scale, scale, colour)
        x += 6 * scale


def paste(canvas, image, x, y):
    width, height, pixels = image
    for row in range(height):
        start = ((y + row) * WIDTH + x) * 3
        canvas[start:start + width * 3] = pixels[row * width * 3:(row + 1) * width * 3]


def write_png(path, canvas):
    def chunk(kind, contents):
        return (struct.pack(">I", len(contents)) + kind + contents +
                struct.pack(">I", zlib.crc32(kind + contents) & 0xFFFFFFFF))
    scanlines = b"".join(b"\x00" + canvas[y * WIDTH * 3:(y + 1) * WIDTH * 3] for y in range(HEIGHT))
    png = (b"\x89PNG\r\n\x1a\n" +
           chunk(b"IHDR", struct.pack(">IIBBBBB", WIDTH, HEIGHT, 8, 2, 0, 0, 0)) +
           chunk(b"sRGB", b"\x00") + chunk(b"IDAT", zlib.compress(scanlines, 6)) + chunk(b"IEND", b""))
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(png)


def render_contact_sheet():
    scene = bpy.data.scenes.get(SCENE_NAME)
    if scene is None or scene.get(OWNER_KEY) != SOURCE_OWNER:
        raise RuntimeError("Open the independently authored Codex source .blend before rendering.")
    manifest = json.loads((MODELS / "Step04BlenderKitManifest.json").read_text(encoding="utf-8"))
    if manifest.get("generator") != SOURCE_OWNER or len(manifest.get("assets", [])) != 13:
        raise RuntimeError("Expected the validated 13-mesh Codex STEP 4 manifest.")
    source_objects = []
    for asset in manifest["assets"]:
        obj = scene.objects.get(asset["name"])
        if obj is None or obj.type != "MESH" or obj.get(OWNER_KEY) != SOURCE_OWNER:
            raise RuntimeError(f"Missing independently authored source mesh: {asset['name']}")
        source_objects.append(obj)

    if bpy.context.window is not None:
        bpy.context.window.scene = scene
    visibility_before = {obj: obj.hide_render for obj in scene.objects}
    transforms_before = {obj: obj.matrix_world.copy() for obj in source_objects}
    camera_before, world_before = scene.camera, scene.world
    canvas = bytearray(bytes((18,28,33)) * WIDTH * HEIGHT)
    rect(canvas, MARGIN, 25, 7, 43, (211,157,58))
    text(canvas, MARGIN + 23, 25, "LAST STAND / INDUSTRIAL KIT", 4)
    text(canvas, MARGIN + 24, 65, "CODEX - STEP 04 / INDEPENDENT BLENDER ASSETS", 2, (144,164,171))
    preview_objects = []
    try:
        with bpy.context.temp_override(scene=scene, view_layer=scene.view_layers[0]):
            camera, lights, stage = create_preview(scene)
            preview_objects = [camera, stage] + lights
            engine = configure_render(scene)
            for obj in scene.objects:
                obj.hide_render = obj not in preview_objects
            for obj in preview_objects:
                obj.hide_render = False
            with tempfile.TemporaryDirectory(prefix="LastStandCodexStep04Kit_") as temporary:
                for index, (asset, obj) in enumerate(zip(manifest["assets"], source_objects)):
                    obj.hide_render = False
                    frame_mesh(scene, obj, camera, lights, stage)
                    tile_path = Path(temporary) / f"{index:02d}_{obj.name}.png"
                    scene.render.filepath = str(tile_path)
                    bpy.ops.render.render(write_still=True)
                    tile = read_rgb_png(tile_path)
                    if tile[:2] != (TILE_WIDTH, TILE_HEIGHT):
                        raise RuntimeError(f"Unexpected tile resolution: {tile[:2]}")
                    x = MARGIN + (index % 4) * (CARD_WIDTH + GAP)
                    y = HEADER + (index // 4) * (CARD_HEIGHT + GAP)
                    rect(canvas, x, y, CARD_WIDTH, CARD_HEIGHT, (32,44,49))
                    paste(canvas, tile, x, y)
                    rect(canvas, x, y + TILE_HEIGHT, CARD_WIDTH, 2, (92,115,118))
                    suffix = asset["name"].removeprefix("SM_LS_")
                    text(canvas, x + 12, y + 215, f"{index+1:02d} {LABELS[suffix]}", 2)
                    dimensions = " X ".join(str(round(value)) for value in asset["dimensions_cm"])
                    text(canvas, x + 12, y + 239, dimensions + " CM", 1, (153,174,180))
                    text(canvas, x + 226, y + 239, f"{asset['triangles']:,} TRIS", 1, (153,174,180))
                    obj.hide_render = True
                    print(f"CODEX_STEP4_KIT_RENDER_TILE {index+1}/13 {obj.name}")

            # Remaining three cards explain the verified kit without suggesting
            # viewport placeholder colours are the final Unreal PBR materials.
            notes = [
                ("GEOMETRY", ["13 AUTHORED MESHES", "13 UCX COLLISIONS", f"{manifest['total_render_triangles']:,} TOTAL TRIANGLES", "APPLIED SCALE 1.0"]),
                ("UV / SCALE", ["METRE-BASED MODELLING", "1 UV REPEAT / 2 METRES", "BOTTOM-CENTRE PIVOTS", "GROUND TOP AT Z 0"]),
                ("ARENA READY", ["MODULAR INDUSTRIAL KIT", "PLACEHOLDER COLOURS", "UNREAL PBR MATERIAL SLOTS", "CODEX SOURCE ONLY"]),
            ]
            for index, (title, lines) in enumerate(notes, start=13):
                x = MARGIN + (index % 4) * (CARD_WIDTH + GAP)
                y = HEADER + (index // 4) * (CARD_HEIGHT + GAP)
                rect(canvas, x, y, CARD_WIDTH, CARD_HEIGHT, (25,38,44))
                rect(canvas, x + 20, y + 28, 42, 4, (211,157,58))
                text(canvas, x + 20, y + 55, title, 3)
                for line_index, value in enumerate(lines):
                    text(canvas, x + 20, y + 112 + line_index * 29, value, 2, (148,171,179))
            write_png(OUTPUT, canvas)
            final_width, final_height, _ = read_rgb_png(OUTPUT)
            if (final_width, final_height) != (WIDTH, HEIGHT):
                raise RuntimeError("Final contact sheet dimensions failed validation.")
            print(f"CODEX_STEP4_KIT_RENDER_SUCCESS engine={engine} tiles=13 "
                  f"resolution={WIDTH}x{HEIGHT} output={OUTPUT}")
    finally:
        for obj, hidden in visibility_before.items():
            obj.hide_render = hidden
        for obj in preview_objects:
            obj.hide_render = True
        scene.camera, scene.world = camera_before, world_before
        for obj, transform in transforms_before.items():
            if obj.matrix_world != transform:
                obj.matrix_world = transform
                raise RuntimeError(f"Unexpected source transform change was restored: {obj.name}")
    return str(OUTPUT)


if __name__ == "__main__":
    render_contact_sheet()
