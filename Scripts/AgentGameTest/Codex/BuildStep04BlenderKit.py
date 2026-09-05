"""Build the independently authored Codex STEP 4 industrial kit in Blender.

Run in Blender (including through Blender MCP):
    exec(compile(open(SCRIPT_PATH, encoding="utf-8").read(), SCRIPT_PATH, "exec"))

Only data tagged with this script's OWNER may be replaced. No existing scene is
copied, no unrelated object is deleted, and no .blend file is saved. Geometry is
authored in metres, exported individually at the origin, then arranged on a grid
in the dedicated scene for inspection. Unreal should import with scale 1.0,
Convert Scene Unit enabled, and custom collision enabled. Exported FBX unit
metadata handles the metre-to-centimetre conversion; do not add import scale 100.
"""

import json
import math
from datetime import datetime, timezone
from pathlib import Path

import bpy
from mathutils import Euler, Matrix, Vector


PROJECT_ROOT = Path(r"D:\Unreal Projects\CodexGame")
MODEL_ROOT = PROJECT_ROOT / "ExternalAssets" / "LastStand" / "Codex" / "Models"
MANIFEST_PATH = MODEL_ROOT / "Step04BlenderKitManifest.json"
SCENE_NAME = "LS_Codex_Environment"
OWNER = "Codex_LastStand_Step04_BlenderKit_v1"
OWNER_KEY = "last_stand_generator"
METRES_PER_UV_REPEAT = 2.0

# Material slots are deliberately namespaced. Logical names in the manifest let
# Unreal replace these viewport placeholders with the STEP 4 PBR instances.
MATERIAL_SPECS = {
    "Concrete": ((0.38, 0.37, 0.33, 1.0), 0.0, 0.88),
    "PaintedMetal": ((0.20, 0.29, 0.31, 1.0), 0.65, 0.62),
    "WornMetal": ((0.33, 0.20, 0.11, 1.0), 0.80, 0.72),
    "DarkMetal": ((0.065, 0.075, 0.078, 1.0), 0.85, 0.63),
    "Wood": ((0.32, 0.21, 0.11, 1.0), 0.0, 0.83),
    "Emissive": ((1.0, 0.64, 0.27, 1.0), 0.0, 0.50),
    "Yellow": ((0.58, 0.39, 0.035, 1.0), 0.25, 0.70),
    "Rubber": ((0.022, 0.025, 0.023, 1.0), 0.0, 0.92),
}


def owned(data):
    return data.get(OWNER_KEY) == OWNER


def mark(data):
    data[OWNER_KEY] = OWNER
    return data


def require_owned_or_missing(data, label):
    if data is not None and not owned(data):
        raise RuntimeError(f"Refusing to change unowned Blender data: {label}")


class MeshBuilder:
    """Disconnected closed solids combined into one efficient export object."""

    def __init__(self):
        self.vertices = []
        self.faces = []
        self.materials = []
        self.smooth = []

    def append(self, vertices, faces, material, smooth=False):
        offset = len(self.vertices)
        self.vertices.extend(tuple(v) for v in vertices)
        for face in faces:
            self.faces.append(tuple(offset + index for index in face))
            self.materials.append(material)
            self.smooth.append(smooth)

    def box(self, centre, dimensions, material, rotation=(0.0, 0.0, 0.0)):
        half = Vector(dimensions) * 0.5
        transform = Euler(rotation, "XYZ").to_matrix()
        centre = Vector(centre)
        local = [
            (-half.x, -half.y, -half.z),
            (half.x, -half.y, -half.z),
            (half.x, half.y, -half.z),
            (-half.x, half.y, -half.z),
            (-half.x, -half.y, half.z),
            (half.x, -half.y, half.z),
            (half.x, half.y, half.z),
            (-half.x, half.y, half.z),
        ]
        self.append(
            [transform @ Vector(v) + centre for v in local],
            [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4),
             (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)],
            material,
        )

    def prism_x(self, x_min, x_max, yz_outline, material):
        """Extrude a counter-clockwise outline as seen along positive X."""
        count = len(yz_outline)
        vertices = [(x, y, z) for x in (x_min, x_max) for y, z in yz_outline]
        faces = [tuple(reversed(range(count))), tuple(range(count, count * 2))]
        faces.extend(
            (i, (i + 1) % count, (i + 1) % count + count, i + count)
            for i in range(count)
        )
        self.append(vertices, faces, material)

    def cylinder(self, start, end, radius, material, segments=20, radius_end=None):
        start, end = Vector(start), Vector(end)
        axis = (end - start).normalized()
        auxiliary = Vector((0, 0, 1)) if abs(axis.z) < 0.9 else Vector((0, 1, 0))
        basis_u = auxiliary.cross(axis).normalized()
        basis_v = axis.cross(basis_u).normalized()
        radius_end = radius if radius_end is None else radius_end
        vertices = []
        for centre, ring_radius in ((start, radius), (end, radius_end)):
            for index in range(segments):
                angle = math.tau * index / segments
                vertices.append(centre + ring_radius * (
                    math.cos(angle) * basis_u + math.sin(angle) * basis_v
                ))
        side_faces = [
            (i, (i + 1) % segments, (i + 1) % segments + segments, i + segments)
            for i in range(segments)
        ]
        self.append(vertices, side_faces, material, smooth=True)
        # Separate cap vertices keep the side smooth without rounding the caps.
        self.append(vertices[:segments], [tuple(reversed(range(segments)))], material)
        self.append(vertices[segments:], [tuple(range(segments))], material)

    def lathe(self, profile, material, segments=24):
        vertices = [
            (radius * math.cos(math.tau * i / segments),
             radius * math.sin(math.tau * i / segments), z)
            for z, radius in profile for i in range(segments)
        ]
        faces = []
        for ring in range(len(profile) - 1):
            for i in range(segments):
                a, b = ring * segments + i, ring * segments + (i + 1) % segments
                faces.append((a, b, b + segments, a + segments))
        self.append(vertices, faces, material, smooth=True)
        self.append(vertices[:segments], [tuple(reversed(range(segments)))], material)
        self.append(vertices[-segments:], [tuple(range(segments))], material)

    def torus(self, centre, major_radius, minor_radius, material,
              rotation=(0.0, 0.0, 0.0), major_segments=20, minor_segments=6):
        transform, centre = Euler(rotation, "XYZ").to_matrix(), Vector(centre)
        vertices = []
        for i in range(major_segments):
            a = math.tau * i / major_segments
            for j in range(minor_segments):
                b = math.tau * j / minor_segments
                radius = major_radius + minor_radius * math.cos(b)
                p = Vector((radius * math.cos(a), radius * math.sin(a),
                            minor_radius * math.sin(b)))
                vertices.append(transform @ p + centre)
        faces = []
        for i in range(major_segments):
            for j in range(minor_segments):
                faces.append((
                    i * minor_segments + j,
                    ((i + 1) % major_segments) * minor_segments + j,
                    ((i + 1) % major_segments) * minor_segments + (j + 1) % minor_segments,
                    i * minor_segments + (j + 1) % minor_segments,
                ))
        self.append(vertices, faces, material, smooth=True)


def build_container():
    m = MeshBuilder()
    m.box((0, 0, 1.295), (5.89, 2.28, 2.44), "PaintedMetal")
    for y in (-1.17, 1.17):
        for z in (0.07, 2.52):
            m.box((0, y, z), (6.06, 0.10, 0.14), "WornMetal")
        for i in range(28):
            x = -2.78 + i * 5.56 / 27
            m.box((x, y, 1.295), (0.085, 0.07, 2.27), "PaintedMetal")
    for x in (-2.95, 2.95):
        for y in (-1.14, 1.14):
            m.box((x, y, 1.295), (0.16, 0.16, 2.59), "WornMetal")
        m.box((x, 0, 2.52), (0.16, 2.44, 0.14), "WornMetal")
        m.box((x, 0, 0.07), (0.16, 2.44, 0.14), "WornMetal")
    # End doors, recessed seams, hinge blocks and four vertical locking rods.
    for y in (-0.55, 0.55):
        m.box((3.005, y, 1.295), (0.025, 1.06, 2.25), "PaintedMetal")
        for dy in (-0.26, 0.26):
            m.cylinder((3.02, y + dy, 0.24), (3.02, y + dy, 2.34),
                       0.012, "DarkMetal", segments=8)
            m.box((3.022, y + dy + 0.07, 1.02), (0.012, 0.16, 0.035), "WornMetal")
        for z in (0.45, 1.3, 2.12):
            m.box((3.012, math.copysign(1.06, y), z), (0.032, 0.09, 0.08), "WornMetal")
    for i in range(23):
        m.box((-2.65 + i * 5.3 / 22, 0, 2.535), (0.075, 2.22, 0.045), "PaintedMetal")
    return m, [("box", (0, 0, 1.295), (6.06, 2.44, 2.59))]


def build_barrier():
    m = MeshBuilder()
    outline = [(-0.275, 0), (0.275, 0), (0.275, 0.18), (0.14, 0.50),
               (0.14, 0.9), (-0.14, 0.9), (-0.14, 0.50), (-0.275, 0.18)]
    m.prism_x(-1.2, 1.2, outline, "Concrete")
    for x in (-0.85, -0.28, 0.28, 0.85):
        m.box((x, -0.141, 0.74), (0.21, 0.008, 0.23), "Yellow")
        m.box((x, 0.141, 0.74), (0.21, 0.008, 0.23), "Yellow")
    for x in (-0.70, 0.70):
        m.box((x, 0, 0.895), (0.11, 0.065, 0.009), "DarkMetal")
    return m, [("box", (0, 0, 0.45), (2.4, 0.55, 0.9))]


def build_pipe_skid():
    m = MeshBuilder()
    for y in (-1.13, 1.13):
        m.box((0, y, 0.14), (6, 0.24, 0.28), "WornMetal")
    for x in (-2.72, 0, 2.72):
        m.box((x, 0, 0.20), (0.20, 2.5, 0.20), "DarkMetal")
    for y, radius in ((-0.77, 0.22), (0, 0.30), (0.77, 0.22)):
        m.cylinder((-2.86, y, 0.92), (2.86, y, 0.92), radius, "PaintedMetal", 24)
        for x in (-2.67, -1.68, 1.68, 2.67):
            m.cylinder((x - 0.04, y, 0.92), (x + 0.04, y, 0.92),
                       radius + 0.055, "WornMetal", 20)
            for a in range(0, 360, 90):
                theta = math.radians(a)
                yy = y + (radius + 0.032) * math.cos(theta)
                zz = 0.92 + (radius + 0.032) * math.sin(theta)
                m.cylinder((x - 0.047, yy, zz), (x + 0.047, yy, zz),
                           0.012, "DarkMetal", 6)
        for x in (-1.68, 1.68):
            m.box((x, y, 0.48), (0.25, radius * 1.7, 0.51), "DarkMetal")
    m.cylinder((0.7, 0, 1.15), (0.7, 0, 1.56), 0.038, "WornMetal", 12)
    m.torus((0.7, 0, 1.57), 0.19, 0.03, "Yellow")
    m.box((0.7, 0, 1.57), (0.35, 0.04, 0.035), "Yellow")
    m.box((0.7, 0, 1.57), (0.04, 0.35, 0.035), "Yellow")
    return m, [("box", (0, 0, 0.80), (6, 2.5, 1.6))]


def build_utility_box():
    m = MeshBuilder()
    m.box((0, 0, 0.075), (1.3, 0.6, 0.15), "Concrete")
    m.box((0, 0, 0.84), (1.19, 0.51, 1.38), "PaintedMetal")
    m.box((0, 0, 1.54), (1.3, 0.6, 0.12), "WornMetal")
    m.box((0, -0.263, 0.84), (1.07, 0.025, 1.24), "PaintedMetal")
    for x in (-0.30, 0.25):
        for z in (0.33, 0.40, 0.47, 0.54, 1.18, 1.25, 1.32):
            m.box((x, -0.282, z), (0.35, 0.024, 0.025), "DarkMetal")
    for z in (0.43, 1.23):
        m.box((-0.545, -0.279, z), (0.055, 0.028, 0.11), "WornMetal")
    m.box((0.44, -0.289, 0.87), (0.035, 0.022, 0.17), "DarkMetal")
    m.box((0.23, -0.281, 0.85), (0.12, 0.013, 0.15), "Yellow")
    return m, [("box", (0, 0, 0.8), (1.3, 0.6, 1.6))]


def build_barrel():
    m = MeshBuilder()
    profile = [(0, 0.28), (0.025, 0.30), (0.065, 0.30), (0.09, 0.278),
               (0.18, 0.278), (0.20, 0.294), (0.235, 0.294), (0.25, 0.278),
               (0.65, 0.278), (0.665, 0.294), (0.70, 0.294), (0.72, 0.278),
               (0.82, 0.278), (0.845, 0.30), (0.88, 0.30), (0.90, 0.28)]
    m.lathe(profile, "PaintedMetal")
    m.cylinder((0, 0, 0.883), (0, 0, 0.894), 0.27, "WornMetal", 24)
    for x, y, radius in ((0.15, 0, 0.037), (-0.13, 0.08, 0.021)):
        m.cylinder((x, y, 0.893), (x, y, 0.9), radius, "DarkMetal", 8)
    return m, [("cylinder", (0, 0, 0.45), (0.30, 0.90))]


def build_pallet():
    m = MeshBuilder()
    for y in (-0.405, 0, 0.405):
        m.box((0, y, 0.0125), (1.2, 0.15, 0.025), "Wood")
        for x in (-0.48, 0, 0.48):
            m.box((x, y, 0.0675), (0.15, 0.15, 0.085), "Wood")
    for i in range(5):
        y = -0.41 + i * 0.205
        m.box((0, y, 0.1325), (1.2, 0.18, 0.035), "Wood")
        for x in (-0.48, 0.48):
            m.cylinder((x, y, 0.149), (x, y, 0.15), 0.005, "DarkMetal", 6)
    return m, [("box", (0, 0, 0.075), (1.2, 1.0, 0.15))]


def build_fence():
    m = MeshBuilder()
    for x in (-1.95, 1.95):
        m.box((x, 0, 1.0), (0.10, 0.12, 2.0), "DarkMetal")
    for z in (0.12, 1.94):
        m.box((0, 0, z), (3.8, 0.08, 0.08), "WornMetal")
    # Two clipped diagonal wire families form actual chain-link diamonds.
    x_min, x_max, z_min, z_max = -1.9, 1.9, 0.17, 1.90
    for slope in (-1.0, 1.0):
        for index in range(28):
            intercept = -4.0 + index * 0.29
            points = []
            for x in (x_min, x_max):
                z = slope * x + intercept
                if z_min <= z <= z_max:
                    points.append((x, 0.015, z))
            for z in (z_min, z_max):
                x = (z - intercept) / slope
                if x_min <= x <= x_max:
                    points.append((x, 0.015, z))
            if len(points) == 2 and (Vector(points[1]) - Vector(points[0])).length > 0.02:
                m.cylinder(points[0], points[1], 0.006, "WornMetal", 4)
    return m, [("box", (0, 0, 1), (4, 0.12, 2))]


def build_warehouse():
    m = MeshBuilder()
    m.box((0, 0, 0.125), (6, 4, 0.25), "Concrete")
    m.box((0, 0, 1.72), (5.88, 3.88, 3.19), "PaintedMetal")
    for x in (-2.93, 2.93):
        for y in (-1.93, 1.93):
            m.box((x, y, 1.68), (0.14, 0.14, 3.36), "WornMetal")
    for side in (-1, 1):
        for i in range(19):
            m.box((-2.76 + i * 5.52 / 18, side * 1.945, 1.72),
                  (0.045, 0.05, 3.1), "PaintedMetal")
    m.prism_x(-3, 3, [(-2, 3.3), (2, 3.3), (0, 3.97)], "WornMetal")
    roof_angle = math.atan2(0.67, 2)
    for i in range(17):
        x = -2.92 + i * 5.84 / 16
        for side in (-1, 1):
            m.box((x, side * 0.995, 3.648), (0.040, 2.087, 0.025), "WornMetal",
                  rotation=(-side * roof_angle, 0, 0))
    m.box((0, 0, 3.985), (6, 0.075, 0.03), "DarkMetal")
    # Front shutter and service door are deliberate wall details, not entrances.
    m.box((-0.65, -1.969, 1.30), (2.44, 0.055, 2.6), "DarkMetal")
    m.box((-0.65, -1.997, 1.30), (2.20, 0.006, 2.42), "PaintedMetal")
    for i in range(20):
        m.box((-0.65, -1.995, 0.17 + i * 0.118), (2.19, 0.01, 0.025), "WornMetal")
    m.box((1.8, -1.973, 1.13), (0.92, 0.042, 2.12), "DarkMetal")
    m.box((1.8, -1.997, 1.13), (0.80, 0.006, 2.0), "PaintedMetal")
    m.box((1.52, -1.998, 1.08), (0.035, 0.004, 0.15), "Yellow")
    m.box((-0.65, -1.98, 2.93), (1.0, 0.04, 0.28), "Yellow")
    return m, [("box", (0, 0, 2), (6, 4, 4))]


def build_crate():
    m = MeshBuilder()
    m.box((0, 0, 0.425), (0.78, 0.78, 0.81), "Wood")
    for side in (-1, 1):
        for i in range(5):
            z = 0.085 + i * 0.17
            m.box((0, side * 0.405, z), (0.85, 0.04, 0.158), "Wood")
            m.box((side * 0.405, 0, z), (0.04, 0.77, 0.158), "Wood")
        for x in (-0.28, 0.28):
            m.box((x, side * 0.419, 0.425), (0.042, 0.012, 0.85), "DarkMetal")
    for i in range(5):
        m.box((0, -0.332 + i * 0.166, 0.829), (0.85, 0.15, 0.042), "Wood")
    for x in (-0.28, 0.28):
        m.box((x, 0, 0.844), (0.042, 0.85, 0.012), "DarkMetal")
    return m, [("box", (0, 0, 0.425), (0.85, 0.85, 0.85))]


def build_lamp():
    m = MeshBuilder()
    m.cylinder((0, 0, 0), (0, 0, 0.11), 0.15, "WornMetal", 12)
    m.cylinder((0, 0, 0.08), (0, 0, 3.27), 0.065, "DarkMetal", 16, radius_end=0.045)
    m.cylinder((0, 0, 3.24), (0.66, 0, 3.34), 0.038, "DarkMetal", 12)
    m.box((0.61, 0, 3.40), (0.53, 0.34, 0.20), "PaintedMetal")
    m.box((0.61, 0, 3.293), (0.44, 0.27, 0.014), "Emissive")
    for x in (-0.09, 0.09):
        for y in (-0.07, 0.07):
            m.cylinder((x, y, 0.105), (x, y, 0.12), 0.014, "DarkMetal", 6)
    return m, [("cylinder", (0, 0, 1.635), (0.15, 3.27))]


def build_ground():
    m = MeshBuilder()
    # This is the single intentional pivot exception: the walking surface is Z=0.
    m.box((0, 0, -0.075), (10, 10, 0.15), "Concrete")
    return m, [("box", (0, 0, -0.075), (10, 10, 0.15))]


def build_drain():
    m = MeshBuilder()
    m.box((0, 0, 0.0125), (0.97, 0.37, 0.025), "DarkMetal")
    for y in (-0.185, 0.185):
        m.box((0, y, 0.028), (1, 0.03, 0.044), "WornMetal")
    for x in (-0.485, 0.485):
        m.box((x, 0, 0.028), (0.03, 0.37, 0.044), "WornMetal")
    for i in range(22):
        m.box((-0.449 + i * 0.898 / 21, 0, 0.041), (0.019, 0.345, 0.018), "WornMetal")
    return m, [("box", (0, 0, 0.025), (1, 0.4, 0.05))]


def build_curb():
    m = MeshBuilder()
    m.box((0, 0, 0.125), (4, 0.25, 0.25), "Concrete")
    for x in (-1.65, -0.55, 0.55, 1.65):
        m.box((x, -0.124, 0.15), (0.43, 0.002, 0.16), "Yellow")
    return m, [("box", (0, 0, 0.125), (4, 0.25, 0.25))]


ASSET_SPECS = [
    ("SM_LS_Container", build_container, 0.008, (6.06, 2.44, 2.59)),
    ("SM_LS_ConcreteBarrier", build_barrier, 0.018, (2.4, 0.55, 0.9)),
    ("SM_LS_PipeSkid", build_pipe_skid, 0.007, (6, 2.5, 1.6)),
    ("SM_LS_UtilityBox", build_utility_box, 0.008, (1.3, 0.6, 1.6)),
    ("SM_LS_Barrel", build_barrel, 0.002, (0.6, 0.6, 0.9)),
    ("SM_LS_Pallet", build_pallet, 0.003, (1.2, 1, 0.15)),
    ("SM_LS_FenceSection", build_fence, 0.003, (4, 0.12, 2)),
    ("SM_LS_Warehouse", build_warehouse, 0.006, (6, 4, 4)),
    ("SM_LS_Crate", build_crate, 0.003, (0.85, 0.85, 0.85)),
    ("SM_LS_IndustrialLamp", build_lamp, 0.005, (1.025, 0.34, 3.5)),
    ("SM_LS_GroundTile", build_ground, 0.0, (10, 10, 0.15)),
    ("SM_LS_Drain", build_drain, 0.002, (1, 0.4, 0.05)),
    ("SM_LS_Curb", build_curb, 0.014, (4, 0.25, 0.25)),
]


def create_materials():
    result = {}
    for logical_name, (colour, metallic, roughness) in MATERIAL_SPECS.items():
        name = f"LS_Codex_{logical_name}"
        material = bpy.data.materials.get(name)
        require_owned_or_missing(material, name)
        if material is None:
            material = mark(bpy.data.materials.new(name))
        material.diffuse_color = colour
        material.use_nodes = True
        material["logical_slot"] = logical_name
        bsdf = material.node_tree.nodes.get("Principled BSDF")
        if bsdf is None:
            raise RuntimeError(f"Owned placeholder material has no Principled BSDF: {name}")
        bsdf.inputs["Base Color"].default_value = colour
        bsdf.inputs["Metallic"].default_value = metallic
        bsdf.inputs["Roughness"].default_value = roughness
        if logical_name == "Emissive":
            emission = bsdf.inputs.get("Emission Color") or bsdf.inputs.get("Emission")
            if emission is not None:
                emission.default_value = colour
            strength = bsdf.inputs.get("Emission Strength")
            if strength is not None:
                strength.default_value = 0.5
        result[logical_name] = material
    return result


def ensure_scene_and_collection():
    scene = bpy.data.scenes.get(SCENE_NAME)
    require_owned_or_missing(scene, SCENE_NAME)
    if scene is None:
        scene = mark(bpy.data.scenes.new(SCENE_NAME))
    collection = bpy.data.collections.get(SCENE_NAME)
    require_owned_or_missing(collection, SCENE_NAME)
    if collection is None:
        collection = mark(bpy.data.collections.new(SCENE_NAME))
    if collection.name not in scene.collection.children:
        scene.collection.children.link(collection)
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.unit_settings.length_unit = "METERS"
    if bpy.context.window is not None:
        bpy.context.window.scene = scene
    return scene, collection


def remove_exact_owned_object(name):
    obj = bpy.data.objects.get(name)
    require_owned_or_missing(obj, name)
    if obj is not None:
        mesh = obj.data if obj.type == "MESH" else None
        bpy.data.objects.remove(obj, do_unlink=True)
        if mesh is not None and mesh.users == 0 and owned(mesh):
            bpy.data.meshes.remove(mesh)


def preflight_names():
    # Check every name first so a naming conflict cannot lead to partial deletion.
    require_owned_or_missing(bpy.data.scenes.get(SCENE_NAME), SCENE_NAME)
    require_owned_or_missing(bpy.data.collections.get(SCENE_NAME), SCENE_NAME)
    for logical_name in MATERIAL_SPECS:
        name = f"LS_Codex_{logical_name}"
        require_owned_or_missing(bpy.data.materials.get(name), name)
    for name, _, _, _ in ASSET_SPECS:
        for object_name in (name, f"UCX_{name}_00"):
            require_owned_or_missing(bpy.data.objects.get(object_name), object_name)
            require_owned_or_missing(bpy.data.meshes.get(f"{object_name}_Mesh"), object_name)


def metric_uv(mesh):
    """Per-face orthonormal projection: exact metric density without stretching.

    UVs intentionally overlap and extend outside 0..1 for tileable PBR surfaces.
    UV0 is not a lightmap atlas. Unreal can generate UV1 if baked lighting is used.
    """
    mesh.update()
    uv = mesh.uv_layers.get("UVMap") or mesh.uv_layers.new(name="UVMap")
    for polygon in mesh.polygons:
        normal = polygon.normal.normalized()
        # On vertical faces preserve vertical V and a consistent horizontal U.
        if abs(normal.z) < 0.8:
            tangent = Vector((0, 0, 1)).cross(normal).normalized()
        else:
            tangent = (Vector((1, 0, 0)) - normal * normal.x).normalized()
        bitangent = normal.cross(tangent).normalized()
        for loop_index in polygon.loop_indices:
            position = mesh.vertices[mesh.loops[loop_index].vertex_index].co
            uv.data[loop_index].uv = (position.dot(tangent) / METRES_PER_UV_REPEAT,
                                      position.dot(bitangent) / METRES_PER_UV_REPEAT)


def create_mesh_object(name, builder, collection, materials=None, bevel=0):
    remove_exact_owned_object(name)
    mesh_name = f"{name}_Mesh"
    orphan = bpy.data.meshes.get(mesh_name)
    require_owned_or_missing(orphan, mesh_name)
    if orphan is not None:
        if orphan.users:
            raise RuntimeError(f"Owned mesh is shared outside its expected object: {mesh_name}")
        bpy.data.meshes.remove(orphan)
    mesh = mark(bpy.data.meshes.new(mesh_name))
    mesh.from_pydata(builder.vertices, [], builder.faces)
    mesh.update()
    if mesh.validate(verbose=True):
        raise RuntimeError(f"Generated mesh required topology repair: {name}")
    used_slots = list(dict.fromkeys(builder.materials)) if materials else []
    for slot in used_slots:
        mesh.materials.append(materials[slot])
    for index, polygon in enumerate(mesh.polygons):
        if materials:
            polygon.material_index = used_slots.index(builder.materials[index])
        polygon.use_smooth = builder.smooth[index]
    obj = mark(bpy.data.objects.new(name, mesh))
    collection.objects.link(obj)
    obj.matrix_world = Matrix.Identity(4)
    if bevel:
        modifier = obj.modifiers.new("LS_Codex_SmallEdgeBevel", "BEVEL")
        modifier.width = bevel
        modifier.segments = 1
        modifier.limit_method = "ANGLE"
        modifier.angle_limit = math.radians(35)
        modifier.use_clamp_overlap = True
        modifier.affect = "EDGES"
        bpy.context.view_layer.update()
        depsgraph = bpy.context.evaluated_depsgraph_get()
        baked = mark(bpy.data.meshes.new_from_object(
            obj.evaluated_get(depsgraph), preserve_all_data_layers=True, depsgraph=depsgraph
        ))
        obj.modifiers.clear()
        obj.data = baked
        bpy.data.meshes.remove(mesh)
        baked.name = mesh_name
        mesh = baked
    if materials:
        metric_uv(mesh)
    mesh.calc_loop_triangles()
    obj["metres_per_uv_repeat"] = METRES_PER_UV_REPEAT
    obj["pivot_policy"] = "ground_top" if name == "SM_LS_GroundTile" else "bottom_centre"
    return obj, used_slots


def collision_builder(shape, centre, dimensions):
    builder = MeshBuilder()
    if shape == "box":
        builder.box(centre, dimensions, "DarkMetal")
    elif shape == "cylinder":
        radius, height = dimensions
        x, y, z = centre
        builder.cylinder((x, y, z - height / 2), (x, y, z + height / 2),
                         radius, "DarkMetal", segments=12)
    else:
        raise ValueError(f"Unsupported simple collision shape: {shape}")
    return builder


def bounds(mesh):
    return (
        [min(v.co[index] for v in mesh.vertices) for index in range(3)],
        [max(v.co[index] for v in mesh.vertices) for index in range(3)],
    )


def rounded(values):
    return [round(float(value), 6) for value in values]


def validate_asset(obj, target_dimensions):
    low, high = bounds(obj.data)
    actual = [high[i] - low[i] for i in range(3)]
    # Bevel rounding can reduce extents by a few millimetres; authored pieces may
    # protrude up to two centimetres for hardware, never metre-scale mistakes.
    for index in range(3):
        if abs(actual[index] - target_dimensions[index]) > 0.021:
            raise RuntimeError(f"Dimensions outside tolerance: {obj.name}: {actual}")
    target_min_z = -0.15 if obj.name == "SM_LS_GroundTile" else 0.0
    if abs(low[2] - target_min_z) > 0.004:
        raise RuntimeError(f"Incorrect bottom pivot: {obj.name}: minimum Z={low[2]}")
    if tuple(obj.scale) != (1.0, 1.0, 1.0):
        raise RuntimeError(f"Unapplied object scale: {obj.name}")
    if len(obj.data.uv_layers) != 1 or len(obj.data.uv_layers[0].data) != len(obj.data.loops):
        raise RuntimeError(f"Missing or incomplete UV0: {obj.name}")
    for item in obj.data.uv_layers[0].data:
        if not all(math.isfinite(component) for component in item.uv):
            raise RuntimeError(f"Non-finite UV in {obj.name}")
    return low, high, actual


def export_asset(scene, obj, collisions, output_path):
    for item in scene.objects:
        item.select_set(False)
    for item in [obj] + collisions:
        item.hide_set(False)
        item.hide_viewport = False
        item.select_set(True)
        item.location = (0, 0, 0)
    scene.view_layers[0].objects.active = obj
    result = bpy.ops.export_scene.fbx(
        filepath=str(output_path), use_selection=True, object_types={"MESH"},
        global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Y", axis_up="Z", use_space_transform=True,
        bake_space_transform=False, use_mesh_modifiers=True, mesh_smooth_type="FACE",
        use_triangles=True, use_tspace=True, add_leaf_bones=False,
        bake_anim=False, path_mode="AUTO", embed_textures=False,
    )
    if "FINISHED" not in result or not output_path.is_file() or output_path.stat().st_size < 1024:
        raise RuntimeError(f"FBX export failed: {obj.name}: {result}")


def build():
    preflight_names()
    MODEL_ROOT.mkdir(parents=True, exist_ok=True)
    scene, collection = ensure_scene_and_collection()
    materials = create_materials()
    records, generated_objects = [], []
    with bpy.context.temp_override(scene=scene, view_layer=scene.view_layers[0]):
        for name, factory, bevel, target_dimensions in ASSET_SPECS:
            builder, collision_specs = factory()
            obj, used_slots = create_mesh_object(name, builder, collection, materials, bevel)
            low, high, dimensions = validate_asset(obj, target_dimensions)
            collisions, collision_records = [], []
            for index, (shape, centre, extent) in enumerate(collision_specs):
                collision_name = f"UCX_{name}_{index:02d}"
                collision, _ = create_mesh_object(
                    collision_name, collision_builder(shape, centre, extent), collection
                )
                collision.display_type = "WIRE"
                collision.hide_render = True
                collision["collision_for"] = name
                collisions.append(collision)
                collision_records.append({
                    "name": collision_name, "shape": shape,
                    "centre_m": list(centre), "parameters_m": list(extent),
                    "vertices": len(collision.data.vertices),
                    "triangles": len(collision.data.loop_triangles),
                })
            output_path = MODEL_ROOT / f"{name}.fbx"
            export_asset(scene, obj, collisions, output_path)
            records.append({
                "name": name, "export_path": str(output_path),
                "relative_export_path": output_path.relative_to(PROJECT_ROOT).as_posix(),
                "dimensions_m": rounded(dimensions),
                "dimensions_cm": rounded([d * 100 for d in dimensions]),
                "target_dimensions_m": list(target_dimensions),
                "bounds_min_m": rounded(low), "bounds_max_m": rounded(high),
                "pivot": obj["pivot_policy"], "export_location_m": [0, 0, 0],
                "applied_scale": [1, 1, 1], "vertices": len(obj.data.vertices),
                "triangles": len(obj.data.loop_triangles),
                "uv_count": len(obj.data.uv_layers), "uv_loops": len(obj.data.loops),
                "metres_per_uv_repeat": METRES_PER_UV_REPEAT,
                "material_slots": [{
                    "index": index, "logical_name": slot,
                    "blender_material": materials[slot].name,
                } for index, slot in enumerate(used_slots)],
                "collision_count": len(collisions), "collisions": collision_records,
                "fbx_bytes": output_path.stat().st_size,
            })
            generated_objects.append((obj, collisions))
            print(f"CODEX_STEP4_BLENDER_ASSET {name} tris={len(obj.data.loop_triangles)} "
                  f"dimensions_m={rounded(dimensions)}")

        # Preview offsets are intentionally not baked into mesh vertices. Every
        # exported FBX above has the same clean origin and unapplied offset zero.
        for index, (obj, collisions) in enumerate(generated_objects):
            preview_location = ((index % 4) * 12.0, (index // 4) * 12.0, 0)
            obj.location = preview_location
            obj.select_set(False)
            for collision in collisions:
                collision.location = preview_location
                collision.select_set(False)
                collision.hide_set(True)
            records[index]["blender_preview_location_m"] = list(preview_location)
        scene.view_layers[0].objects.active = generated_objects[0][0]
        generated_objects[0][0].select_set(True)
        scene.view_layers[0].update()

    manifest = {
        "generator": OWNER, "generated_utc": datetime.now(timezone.utc).isoformat(),
        "scene": SCENE_NAME, "collection": SCENE_NAME,
        "blender_version": bpy.app.version_string,
        "authoring_units": "metres", "fbx_apply_unit_scale": True,
        "fbx_apply_scale_options": "FBX_SCALE_UNITS",
        "fbx_axis_forward": "-Y", "fbx_axis_up": "Z",
        "unreal_import_scale": 1.0, "unreal_convert_scene_unit": True,
        "uv_policy": "UV0 orthonormal face projection; one repeat per two metres",
        "lightmap_uv": "not authored; generate UV1 on Unreal import if required",
        "blend_saved": False, "assets": records,
        "render_mesh_count": len(records),
        "collision_mesh_count": sum(record["collision_count"] for record in records),
        "blender_objects_created": sum(1 + len(items) for _, items in generated_objects),
        "placeholder_materials": [materials[key].name for key in MATERIAL_SPECS],
        "total_render_vertices": sum(record["vertices"] for record in records),
        "total_render_triangles": sum(record["triangles"] for record in records),
        "preservation_policy": "Only exact named, generator-tagged Codex data replaced",
    }
    MANIFEST_PATH.write_text(json.dumps(manifest, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"CODEX_STEP4_BLENDER_KIT_SUCCESS meshes={len(records)} "
          f"objects={manifest['blender_objects_created']} "
          f"triangles={manifest['total_render_triangles']} manifest={MANIFEST_PATH}")
    return manifest


if __name__ == "__main__":
    build()
