"""Import Codex-owned STEP 4 FBX/PBR assets and build one reusable surface graph.

Run with UnrealEditor-Cmd -run=PythonScript -script=<this file>, or in the editor.
Only /Game/AgentGameTest/Codex/Environment is written. Repeated runs reuse assets
whose source hash is unchanged and rebuild the same master graph, not new copies.
"""

import datetime
import hashlib
import json
from pathlib import Path
import re
import traceback

import unreal


ROOT = "/Game/AgentGameTest/Codex/Environment"
PROJECT = Path(unreal.Paths.project_dir()).resolve()
EXTERNAL = PROJECT / "ExternalAssets/LastStand/Codex"
TEXTURES = EXTERNAL / "Textures"
MODELS = EXTERNAL / "Models"
REPORT_PATH = PROJECT / "Saved/AgentComparison/Codex/Step04_ImportReport.json"
MASTER_NAME = "M_LastStand_Surface_Codex"
SOURCE_HASH_TAG = "LastStandStep04SourceSHA256"
REPORT = {
    "schema_version": 1,
    "agent": "Codex",
    "step": 4,
    "status": "running",
    "started_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
    "asset_root": ROOT,
    "textures": [],
    "meshes": [],
    "materials": [],
    "material_instances": [],
    "logical_materials": {},
    "optional_property_failures": [],
    "errors": [],
    "counts": {"textures_created": 0, "static_meshes_created": 0,
               "materials_created": 0, "material_instances_created": 0,
               "source_reimports": 0, "source_reuses": 0},
    "validation_scope": "Import/settings/source/graph validation only; visual/PIE QA is separate.",
}


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def safe_source(root, relative):
    path = (root / relative).resolve()
    require(path.is_relative_to(root.resolve()), f"Source leaves Codex folder: {path}")
    require(path.is_file(), f"Source file missing: {path}")
    return path


def asset_path(folder, name):
    require(re.fullmatch(r"[A-Za-z0-9_]+", name) is not None, f"Unsafe asset name: {name}")
    return f"{ROOT}/{folder}/{name}"


def load_manifest(path):
    require(path.is_file(), f"Manifest missing: {path}")
    with path.open("r", encoding="utf-8-sig") as handle:
        data = json.load(handle)
    require(data.get("agent", "Codex") == "Codex", f"Non-Codex manifest: {path}")
    return data


def optional_property(obj, name, value):
    try:
        obj.set_editor_property(name, value)
        return True
    except Exception as error:
        item = {"object": obj.get_name(), "property": name, "error": str(error)}
        REPORT["optional_property_failures"].append(item)
        unreal.log_warning(f"CODEX_STEP4_IMPORT_OPTIONAL {item}")
        return False


def save_asset(asset):
    require(asset.get_path_name().startswith(ROOT + "/"), "Attempt to save outside STEP 4 scope")
    require(unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False),
            f"Asset save failed: {asset.get_path_name()}")


def create_or_load(folder, name, asset_class, factory, count_key):
    path = asset_path(folder, name)
    existing = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if existing:
        require(isinstance(existing, asset_class), f"Existing asset has unexpected class: {path}")
        return existing, False
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, f"{ROOT}/{folder}", asset_class, factory)
    require(asset is not None, f"Asset creation failed: {path}")
    REPORT["counts"][count_key] += 1
    return asset, True


def import_file(source, folder, name, asset_class, factory, options, count_key):
    path = asset_path(folder, name)
    digest = hashlib.sha256(source.read_bytes()).hexdigest()
    exists = unreal.EditorAssetLibrary.does_asset_exist(path)
    asset = unreal.EditorAssetLibrary.load_asset(path) if exists else None
    if asset and unreal.EditorAssetLibrary.get_metadata_tag(asset, SOURCE_HASH_TAG) == digest:
        require(isinstance(asset, asset_class), f"Unexpected existing class: {path}")
        REPORT["counts"]["source_reuses"] += 1
        return asset, digest, False
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", str(source))
    task.set_editor_property("destination_path", f"{ROOT}/{folder}")
    task.set_editor_property("destination_name", name)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", False)
    task.set_editor_property("factory", factory)
    if options is not None:
        task.set_editor_property("options", options)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    asset = unreal.EditorAssetLibrary.load_asset(path)
    require(asset is not None and isinstance(asset, asset_class), f"Import failed or wrong result type: {path}")
    imported_paths = list(task.get_editor_property("imported_object_paths"))
    require(all(str(item).startswith(ROOT + "/") for item in imported_paths),
            f"Importer created an out-of-scope asset: {imported_paths}")
    unreal.EditorAssetLibrary.set_metadata_tag(asset, SOURCE_HASH_TAG, digest)
    REPORT["counts"]["source_reimports" if exists else count_key] += 1
    return asset, digest, not exists


def import_textures(manifest):
    sets = {}
    suffixes = {"BaseColor": "BaseColor", "NormalDX": "Normal",
                "Roughness": "Roughness", "AmbientOcclusion": "AO",
                "Packed_AO_Roughness_Metallic": "ARM"}
    for texture_set in manifest["sets"]:
        set_id = texture_set["id"]
        maps = {}
        for entry in texture_set["maps"]:
            role = entry["role"]
            require(role in suffixes, f"Unsupported texture role: {role}")
            source = safe_source(TEXTURES, entry["relative_path"])
            name = f"T_LS_{set_id}_{suffixes[role]}"
            texture, digest, created = import_file(source, "Textures", name, unreal.Texture2D,
                                                  unreal.TextureFactory(), None, "textures_created")
            if entry.get("sha256"):
                require(digest == entry["sha256"], f"Texture manifest hash mismatch: {source}")
            is_color = role == "BaseColor"
            is_normal = role == "NormalDX"
            texture.set_editor_property("srgb", is_color)
            texture.set_editor_property("compression_settings", (
                unreal.TextureCompressionSettings.TC_DEFAULT if is_color else
                unreal.TextureCompressionSettings.TC_NORMALMAP if is_normal else
                unreal.TextureCompressionSettings.TC_MASKS))
            texture.set_editor_property("flip_green_channel", False)
            texture.set_editor_property("max_texture_size", 2048)
            optional_property(texture, "virtual_texture_streaming", False)
            save_asset(texture)
            maps[role] = texture
            REPORT["textures"].append({
                "asset": texture.get_path_name(), "source": str(source), "set": set_id,
                "role": role, "sha256": digest, "created": created,
                "srgb": bool(texture.get_editor_property("srgb")),
                "compression": str(texture.get_editor_property("compression_settings")),
                "green_flipped": bool(texture.get_editor_property("flip_green_channel")),
                "width": entry.get("width"), "height": entry.get("height"),
                "tile_width_m": texture_set["width_m"],
            })
        require(all(role in maps for role in ("BaseColor", "NormalDX", "Roughness", "AmbientOcclusion")),
                f"Incomplete PBR set: {set_id}")
        sets[set_id] = {"maps": maps, "width_m": float(texture_set["width_m"])}
    return sets


def make_master(sets):
    lib = unreal.MaterialEditingLibrary
    material, created = create_or_load("Materials", MASTER_NAME, unreal.Material,
                                       unreal.MaterialFactoryNew(), "materials_created")
    lib.delete_all_material_expressions(material)
    material.set_editor_property("two_sided", False)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)

    def node(class_name, x, y, **properties):
        expression = lib.create_material_expression(material, getattr(unreal, class_name), x, y)
        require(expression is not None, f"Material expression failed: {class_name}")
        for key, value in properties.items():
            expression.set_editor_property(key, value)
        return expression

    def link(source, output, destination, pin):
        require(lib.connect_material_expressions(source, output, destination, pin),
                f"Material connection failed: {source.get_name()}.{output} -> {destination.get_name()}.{pin}")

    def property_link(source, output, prop):
        require(lib.connect_material_property(source, output, prop), f"Material output failed: {prop}")

    def scalar(name, value, x, y):
        return node("MaterialExpressionScalarParameter", x, y, parameter_name=name, default_value=value)

    def vector(name, value, x, y):
        return node("MaterialExpressionVectorParameter", x, y, parameter_name=name,
                    default_value=unreal.LinearColor(*value))

    uv = node("MaterialExpressionTextureCoordinate", -1200, -650)
    uv_scale = scalar("UVTiling", 1.0, -1200, -500)
    scaled_uv = node("MaterialExpressionMultiply", -960, -620)
    link(uv, "", scaled_uv, "A")
    link(uv_scale, "", scaled_uv, "B")
    defaults = sets["concrete_floor_02"]["maps"]

    def sample(name, texture, sampler, y):
        result = node("MaterialExpressionTextureSampleParameter2D", -730, y,
                      parameter_name=name, texture=texture, sampler_type=sampler)
        link(scaled_uv, "", result, "UVs")
        return result

    color = sample("BaseColor", defaults["BaseColor"], unreal.MaterialSamplerType.SAMPLERTYPE_COLOR, -600)
    normal = sample("Normal", defaults["NormalDX"], unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL, -200)
    rough = sample("Roughness", defaults["Roughness"], unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 200)
    ao = sample("AO", defaults["AmbientOcclusion"], unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 550)
    arm = sample("MetallicMap", sets["rusty_metal_sheet"]["maps"]["Packed_AO_Roughness_Metallic"],
                 unreal.MaterialSamplerType.SAMPLERTYPE_MASKS, 900)

    weight = scalar("BaseColorTexWeight", 1.0, -730, -820)
    color_blend = node("MaterialExpressionLinearInterpolate", -400, -610, const_a=1.0)
    link(color, "RGB", color_blend, "B")
    link(weight, "", color_blend, "Alpha")
    tint = vector("ColorTint", (1.0, 1.0, 1.0, 1.0), -400, -810)
    tinted = node("MaterialExpressionMultiply", -130, -600)
    link(color_blend, "", tinted, "A")
    link(tint, "RGB", tinted, "B")
    property_link(tinted, "", unreal.MaterialProperty.MP_BASE_COLOR)

    flat_normal = node("MaterialExpressionConstant3Vector", -730, -370,
                       constant=unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    normal_strength = scalar("NormalStrength", 1.0, -420, -380)
    normal_blend = node("MaterialExpressionLinearInterpolate", -400, -170)
    link(flat_normal, "", normal_blend, "A")
    link(normal, "RGB", normal_blend, "B")
    link(normal_strength, "", normal_blend, "Alpha")
    normalized = node("MaterialExpressionNormalize", -140, -160)
    link(normal_blend, "", normalized, "VectorInput")
    property_link(normalized, "", unreal.MaterialProperty.MP_NORMAL)

    rough_mult = scalar("RoughnessMultiplier", 1.0, -730, 40)
    rough_scaled = node("MaterialExpressionMultiply", -410, 160)
    link(rough, "R", rough_scaled, "A")
    link(rough_mult, "", rough_scaled, "B")
    rough_clamp = node("MaterialExpressionClamp", -140, 160, min_default=0.15, max_default=1.0)
    # Clamp's first input is unnamed in UE 5.8's reflected material graph.
    link(rough_scaled, "", rough_clamp, "")
    property_link(rough_clamp, "", unreal.MaterialProperty.MP_ROUGHNESS)
    property_link(ao, "R", unreal.MaterialProperty.MP_AMBIENT_OCCLUSION)

    metallic = scalar("Metallic", 0.0, -730, 1170)
    metallic_weight = scalar("MetallicMapWeight", 0.0, -440, 1170)
    metallic_blend = node("MaterialExpressionLinearInterpolate", -150, 950)
    link(metallic, "", metallic_blend, "A")
    link(arm, "B", metallic_blend, "B")
    link(metallic_weight, "", metallic_blend, "Alpha")
    property_link(metallic_blend, "", unreal.MaterialProperty.MP_METALLIC)

    emission = vector("EmissiveColor", (1.0, 0.72, 0.36, 1.0), -700, 1430)
    emission_strength = scalar("EmissiveStrength", 0.0, -700, 1610)
    emission_mult = node("MaterialExpressionMultiply", -160, 1450)
    link(emission, "RGB", emission_mult, "A")
    link(emission_strength, "", emission_mult, "B")
    property_link(emission_mult, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    compile_errors = list(lib.recompile_material(material) or [])
    require(not compile_errors, f"Master material compile failed: {compile_errors}")
    save_asset(material)
    REPORT["materials"].append({"asset": material.get_path_name(), "created": created,
                                "compile_errors": compile_errors, "master_graph_count": 1})
    return material


def make_instances(master, sets):
    # Kit UV0 uses one tile per two meters. These ratios preserve source scale.
    specifications = [
        ("Concrete", "concrete_floor_02", (0.36, 0.40, 0.43), 1.0, 0.35, 0.0, 0.45),
        ("Asphalt", "asphalt_floor", (0.26, 0.30, 0.34), 1.05, 0.72, 0.0, 1.0),
        ("AsphaltBackground", "asphalt_floor", (0.26, 0.30, 0.34), 1.05, 0.72, 0.0, 1.0),
        ("PaintedBlue", "rusty_metal_sheet", (0.13, 0.35, 0.49), 1.0, 0.45, 0.0, 0.32),
        ("PaintedRed", "rusty_metal_sheet", (0.51, 0.10, 0.065), 1.0, 0.45, 0.0, 0.32),
        ("PaintedYellow", "rusty_metal_sheet", (0.73, 0.48, 0.055), 1.0, 0.45, 0.0, 0.25),
        ("PaintedGreen", "rusty_metal_sheet", (0.16, 0.29, 0.21), 1.0, 0.45, 0.0, 0.32),
        ("WornMetal", "rusty_metal_sheet", (0.88, 0.82, 0.75), 1.0, 0.8, 0.0, 1.0),
        ("DarkMetal", "rusty_metal_sheet", (0.13, 0.145, 0.16), 0.9, 0.35, 0.7, 0.2),
        ("Wood", "wooden_planks", (0.7, 0.61, 0.48), 1.1, 0.75, 0.0, 1.0),
        ("Rubber", "concrete_floor_02", (0.027, 0.032, 0.034), 1.2, 0.15, 0.0, 0.08),
        ("Lamp", "concrete_floor_02", (0.8, 0.7, 0.45), 0.7, 0.0, 0.0, 0.0),
        ("PaintWhite", "concrete_floor_02", (0.62, 0.66, 0.63), 1.1, 0.15, 0.0, 0.08),
        ("PaintYellow", "concrete_floor_02", (0.78, 0.49, 0.035), 1.1, 0.15, 0.0, 0.08),
    ]
    results = {}
    lib = unreal.MaterialEditingLibrary
    for logical, set_id, tint, rough, normal, metallic, color_weight in specifications:
        instance, created = create_or_load("Materials", f"MI_LS_{logical}_Codex",
                                           unreal.MaterialInstanceConstant,
                                           unreal.MaterialInstanceConstantFactoryNew(),
                                           "material_instances_created")
        lib.set_material_instance_parent(instance, master)
        texture_set = sets[set_id]
        parameters = {"UVTiling": 2.0 / texture_set["width_m"],
                      "RoughnessMultiplier": rough, "NormalStrength": normal,
                      "Metallic": metallic, "MetallicMapWeight": 1.0 if logical == "WornMetal" else 0.0,
                      "BaseColorTexWeight": color_weight,
                      "EmissiveStrength": 2.0 if logical == "Lamp" else 0.0}
        if logical == "AsphaltBackground":
            parameters["UVTiling"] *= 9.0
        for name, value in parameters.items():
            # UE 5.8.2 setters return false even after a successful write
            # (MaterialEditingLibrary.cpp). Validate the actual read-back value.
            lib.set_material_instance_scalar_parameter_value(instance, name, value)
            require(abs(lib.get_material_instance_scalar_parameter_value(instance, name) - value) < 0.0001,
                    f"Scalar read-back mismatch: {logical}.{name}")
        for name, value in {"ColorTint": unreal.LinearColor(*tint, 1.0),
                            "EmissiveColor": unreal.LinearColor(1.0, 0.72, 0.36, 1.0)}.items():
            lib.set_material_instance_vector_parameter_value(instance, name, value)
            actual = lib.get_material_instance_vector_parameter_value(instance, name)
            require(all(abs(getattr(actual, c) - getattr(value, c)) < 0.0001 for c in ("r", "g", "b", "a")),
                    f"Vector read-back mismatch: {logical}.{name}")
        for role, parameter in (("BaseColor", "BaseColor"), ("NormalDX", "Normal"),
                                ("Roughness", "Roughness"), ("AmbientOcclusion", "AO")):
            expected = texture_set["maps"][role]
            lib.set_material_instance_texture_parameter_value(instance, parameter, expected)
            require(lib.get_material_instance_texture_parameter_value(instance, parameter) == expected,
                    f"Texture read-back mismatch: {logical}.{parameter}")
        expected = sets["rusty_metal_sheet"]["maps"]["Packed_AO_Roughness_Metallic"]
        lib.set_material_instance_texture_parameter_value(instance, "MetallicMap", expected)
        require(lib.get_material_instance_texture_parameter_value(instance, "MetallicMap") == expected,
                "MetallicMap read-back mismatch")
        lib.update_material_instance(instance)
        save_asset(instance)
        results[logical] = instance
        REPORT["logical_materials"][logical] = instance.get_path_name().split(".")[0]
        REPORT["material_instances"].append({"asset": instance.get_path_name(), "created": created,
             "logical": logical, "texture_set": set_id, "scalar_parameters": parameters,
             "color_tint": list(tint), "parent": master.get_path_name()})
    for alias, logical in {"PaintedMetal": "PaintedBlue", "Yellow": "PaintYellow", "Emissive": "Lamp"}.items():
        results[alias] = results[logical]
        REPORT["logical_materials"][alias] = REPORT["logical_materials"][logical]
    return results


def fbx_options():
    options = unreal.FbxImportUI()
    for name, value in {"automated_import_should_detect_type": False,
                        "mesh_type_to_import": unreal.FBXImportType.FBXIT_STATIC_MESH,
                        "original_import_type": unreal.FBXImportType.FBXIT_STATIC_MESH,
                        "import_as_skeletal": False, "import_mesh": True,
                        "import_materials": False, "import_textures": False,
                        "import_animations": False, "override_full_name": True}.items():
        options.set_editor_property(name, value)
    data = options.get_editor_property("static_mesh_import_data")
    for name, value in {"import_uniform_scale": 1.0, "convert_scene": True,
                        "convert_scene_unit": True, "force_front_x_axis": False,
                        "combine_meshes": True, "auto_generate_collision": False,
                        "one_convex_hull_per_ucx": True, "generate_lightmap_u_vs": True,
                        "transform_vertex_to_absolute": True, "bake_pivot_in_vertex": False,
                        "normal_import_method": unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS,
                        "reorder_material_to_fbx_order": True}.items():
        data.set_editor_property(name, value)
    optional_property(data, "build_nanite", False)
    return options


def logical_slot(slot_name, fallback, instances):
    text = str(slot_name)
    if text in instances:
        return text
    for logical in sorted(instances, key=len, reverse=True):
        if re.search(r"(?:^|_)" + re.escape(logical) + r"(?:_Codex)?(?:\.\d+)?$", text, re.IGNORECASE):
            return logical
    if fallback in instances:
        return fallback
    raise RuntimeError(f"Unknown logical material slot: {text}, fallback={fallback}")


def import_meshes(manifest, instances):
    entries = manifest.get("meshes", manifest.get("assets", []))
    require(entries, "Models manifest contains no meshes/assets")
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)
    for entry in entries:
        name = entry.get("name", entry.get("mesh_name"))
        require(name and name.startswith("SM_LS_"), f"Non-Codex STEP 4 mesh name: {name}")
        relative = entry.get("relative_path", entry.get("file", entry.get("filename", f"{name}.fbx")))
        source = safe_source(MODELS, relative)
        require(source.suffix.lower() == ".fbx", f"Unexpected mesh format: {source}")
        mesh, digest, created = import_file(source, "Meshes", name, unreal.StaticMesh,
                                            unreal.FbxFactory(), fbx_options(), "static_meshes_created")
        slots = list(mesh.get_editor_property("static_materials"))
        fallbacks = entry.get("material_slots", entry.get("logical_material_slots", []))
        assignments = []
        require(slots, f"Mesh has no material slots: {name}")
        for index, slot in enumerate(slots):
            imported_name = slot.get_editor_property("imported_material_slot_name")
            slot_name = imported_name if str(imported_name) not in ("", "None") else slot.get_editor_property("material_slot_name")
            fallback = fallbacks[index] if isinstance(fallbacks, list) and index < len(fallbacks) else None
            if isinstance(fallback, dict):
                fallback = fallback.get("logical_name", fallback.get("logical", fallback.get("name")))
            logical = logical_slot(slot_name, fallback, instances)
            mesh.set_material(index, instances[logical])
            assignments.append({"index": index, "imported_slot": str(slot_name), "logical": logical,
                                "material": instances[logical].get_path_name()})
        collision_count = (subsystem.get_simple_collision_count(mesh)
                           + subsystem.get_convex_collision_count(mesh))
        uv_channels = subsystem.get_num_uv_channels(mesh, 0)
        require(collision_count > 0, f"UCX simple collision missing: {name}")
        require(uv_channels >= 1, f"UV0 missing: {name}")
        bounds = mesh.get_bounds()
        extent = bounds.box_extent
        origin = bounds.origin
        dimensions = [float(extent.x * 2.0), float(extent.y * 2.0), float(extent.z * 2.0)]
        require(all(value > 0 for value in dimensions), f"Degenerate imported bounds: {name}")
        expected_dimensions = entry.get("dimensions_cm")
        if expected_dimensions:
            # FBX front-axis conversion may exchange X/Y, but unit scale must match.
            require(all(abs(actual - expected) <= max(2.0, expected * 0.02)
                        for actual, expected in zip(sorted(dimensions), sorted(expected_dimensions))),
                    f"FBX scale mismatch: {name}; actual={dimensions}, expected={expected_dimensions}")
        save_asset(mesh)
        REPORT["meshes"].append({"asset": mesh.get_path_name(), "source": str(source),
             "sha256": digest, "created": created, "dimensions_cm": dimensions,
             "bounds_min_cm": [float(origin.x - extent.x), float(origin.y - extent.y), float(origin.z - extent.z)],
             "simple_collision_count": collision_count, "uv_channels": uv_channels,
             "vertices_lod0": subsystem.get_number_verts(mesh, 0),
             "expected_dimensions_cm": expected_dimensions,
             "material_slots": assignments, "nanite_requested": False, "import_scale": 1.0})


def main():
    try:
        texture_manifest = load_manifest(TEXTURES / "manifest.json")
        model_manifest = load_manifest(MODELS / "Step04BlenderKitManifest.json")
        for directory in (ROOT, f"{ROOT}/Textures", f"{ROOT}/Meshes", f"{ROOT}/Materials"):
            if not unreal.EditorAssetLibrary.does_directory_exist(directory):
                require(unreal.EditorAssetLibrary.make_directory(directory), f"Folder creation failed: {directory}")
        sets = import_textures(texture_manifest)
        master = make_master(sets)
        instances = make_instances(master, sets)
        import_meshes(model_manifest, instances)
        REPORT["counts"].update({"textures_total": len(REPORT["textures"]),
                                 "static_meshes_total": len(REPORT["meshes"]),
                                 "materials_total": len(REPORT["materials"]),
                                 "material_instances_total": len(REPORT["material_instances"])})
        REPORT["status"] = "success"
        unreal.log("CODEX_STEP4_IMPORT_SUCCESS " + json.dumps(REPORT["counts"], sort_keys=True))
    except Exception as error:
        REPORT["status"] = "failed"
        REPORT["errors"].append({"message": str(error), "traceback": traceback.format_exc()})
        unreal.log_error(f"CODEX_STEP4_IMPORT_FAILED {error}")
        raise
    finally:
        REPORT["ended_utc"] = datetime.datetime.now(datetime.timezone.utc).isoformat()
        REPORT_PATH.parent.mkdir(parents=True, exist_ok=True)
        REPORT_PATH.write_text(json.dumps(REPORT, ensure_ascii=False, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
