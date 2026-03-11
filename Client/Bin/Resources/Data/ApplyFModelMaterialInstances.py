import argparse
import json
import re
import sys
from pathlib import Path
from typing import Dict, Optional, Tuple

import bpy


IMAGE_EXTENSIONS = (".png", ".tga", ".jpg", ".jpeg", ".dds", ".bmp")
NUMERIC_SUFFIX_RE = re.compile(r"\.\d{3}$")

# Blender Text Editor에서 Run Script로 바로 테스트할 때 이 값을 수정해서 사용.
# CLI 인자가 들어오면 그 값이 우선한다.
DEFAULT_EXPORT_ROOT = r"C:\Users\Moon In Su\Desktop\FModel\Output\Exports"
DEFAULT_MATERIALS_DIR = (
    r"C:\Users\Moon In Su\Desktop\FModel\Output\Exports\NARUTO\Content\Environments"
    r"\LobbyMapAssets\LM_KonohaVillage_BORUTO\Materials"
)
DEFAULT_TEXTURES_DIR = (
    r"C:\Users\Moon In Su\Desktop\FModel\Output\Exports\NARUTO\Content\Environments"
    r"\LobbyMapAssets\LM_KonohaVillage_BORUTO\Textures"
)

BASE_COLOR_KEYS = (
    "AA_Override_BaseColorMap",
    "AA_BaseColorMap",
    "PM_Diffuse",
)
NORMAL_KEYS = (
    "BA_NomalMap",
    "BA_NormalMap",
    "PM_Normals",
)
SPECULAR_KEYS = (
    "PM_SpecularMasks",
    "Specular",
)
ROUGHNESS_KEYS = (
    "Roughness",
    "PM_Roughness",
)
EMISSIVE_KEYS = (
    "Emissive",
    "Emission",
)

FLAT_COLOR_KEYWORDS = (
    "mountainside",
    "mountain",
    "cliff",
    "rockwall",
    "facerock",
    "backdrop",
)

DEFAULT_FLAT_COLOR = (0.77, 0.68, 0.58, 1.0)


def parse_args():
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1 :]
    else:
        argv = []

    parser = argparse.ArgumentParser(
        description="Apply FModel Material Instance JSON data to Blender materials."
    )
    parser.add_argument(
        "--export-root",
        default="",
        help="FModel export root. Example: C:\\Users\\...\\Output\\Exports",
    )
    parser.add_argument(
        "--materials-dir",
        default="",
        help="Optional explicit Materials folder. Defaults to recursive search under export root.",
    )
    parser.add_argument(
        "--textures-dir",
        default="",
        help="Optional explicit Textures folder. Defaults to recursive search under export root.",
    )
    parser.add_argument(
        "--only-selected",
        action="store_true",
        help="Only process materials used by selected objects.",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print what would change without editing materials.",
    )
    parser.add_argument(
        "--skip-dedup",
        action="store_true",
        help="Do not merge duplicate materials like Material.001 into Material.",
    )
    return parser.parse_args(argv)


def normalize_material_name(name: str) -> str:
    return NUMERIC_SUFFIX_RE.sub("", name)


def build_image_index(search_root: Path) -> Dict[str, Path]:
    index: Dict[str, Path] = {}

    if not search_root.exists():
        return index

    for path in search_root.rglob("*"):
        if not path.is_file():
            continue

        ext = path.suffix.lower()
        if ext not in IMAGE_EXTENSIONS:
            continue

        index.setdefault(path.stem.lower(), path)

    return index


def build_material_json_index(search_root: Path) -> Dict[str, Path]:
    index: Dict[str, Path] = {}

    if not search_root.exists():
        return index

    for path in search_root.rglob("*.json"):
        if not path.is_file():
            continue

        index.setdefault(path.stem.lower(), path)

    return index


def get_target_materials(only_selected: bool):
    materials = set()

    if only_selected:
        objects = bpy.context.selected_objects
    else:
        objects = bpy.context.scene.objects

    for obj in objects:
        for slot in obj.material_slots:
            if slot.material is not None:
                materials.add(slot.material)

    return sorted(materials, key=lambda material: material.name)


def deduplicate_materials():
    materials = bpy.data.materials
    removed = 0

    for material in list(materials):
        base_name = normalize_material_name(material.name)
        if base_name == material.name:
            continue

        replacement = materials.get(base_name)
        if replacement is None:
            continue

        for obj in bpy.context.scene.objects:
            for slot in obj.material_slots:
                if slot.material == material:
                    slot.material = replacement

        bpy.data.materials.remove(material, do_unlink=True)
        removed += 1

    return removed


def extract_texture_asset_name(unreal_ref: str) -> str:
    if not unreal_ref:
        return ""

    trimmed = unreal_ref.strip().strip('"').strip("'")
    if "." in trimmed:
        trimmed = trimmed.rsplit(".", 1)[0]

    if "/" in trimmed:
        trimmed = trimmed.rsplit("/", 1)[-1]

    return trimmed


def parse_mi_json(json_path: Path):
    with json_path.open("r", encoding="utf-8") as file:
        data = json.load(file)

    textures = data.get("Textures", {})
    parameters = data.get("Parameters", {})
    colors = parameters.get("Colors", {})
    scalars = parameters.get("Scalars", {})

    return textures, colors, scalars


def find_first_texture(textures: dict, keys: tuple[str, ...], fallback_terms: tuple[str, ...] = ()) -> str:
    for key in keys:
        value = textures.get(key)
        if value:
            return value

    for key, value in textures.items():
        lower = key.lower()
        if any(term in lower for term in fallback_terms):
            return value

    return ""


def resolve_image_path(texture_ref: str, image_index: Dict[str, Path]) -> Optional[Path]:
    asset_name = extract_texture_asset_name(texture_ref)
    if not asset_name:
        return None

    return image_index.get(asset_name.lower())


def load_image_cached(path: Path):
    existing = bpy.data.images.get(path.name)
    if existing and Path(bpy.path.abspath(existing.filepath_raw)).resolve() == path.resolve():
        return existing

    return bpy.data.images.load(str(path), check_existing=True)


def clear_material_nodes(material):
    material.use_nodes = True
    nodes = material.node_tree.nodes

    for node in list(nodes):
        nodes.remove(node)


def create_tex_image_node(material, image_path: Path, location, non_color: bool):
    image = load_image_cached(image_path)
    node = material.node_tree.nodes.new(type="ShaderNodeTexImage")
    node.image = image
    node.location = location
    node.image.colorspace_settings.name = "Non-Color" if non_color else "sRGB"
    return node


def create_rgb_node(material, color, location):
    node = material.node_tree.nodes.new(type="ShaderNodeRGB")
    node.location = location
    node.outputs["Color"].default_value = color
    return node


def color_to_rgba(color_value: dict) -> Tuple[float, float, float, float]:
    return (
        float(color_value.get("R", 1.0)),
        float(color_value.get("G", 1.0)),
        float(color_value.get("B", 1.0)),
        float(color_value.get("A", 1.0)),
    )


def should_force_flat_color(material_name: str) -> bool:
    lower = material_name.lower()
    return any(keyword in lower for keyword in FLAT_COLOR_KEYWORDS)


def build_flat_color(material_name: str, colors: dict) -> Tuple[float, float, float, float]:
    base = list(DEFAULT_FLAT_COLOR)

    mix_color = colors.get("AA_BaseMixColor") or colors.get("BaseColor")
    if mix_color:
        rgba = color_to_rgba(mix_color)
        # Unreal 머티리얼 인스턴스에서 1.0~1.2 계열의 흰색 보정값은
        # Blender 단색 대체 시 결과를 과도하게 하얗게 만들기 쉬워서 무시한다.
        if rgba[0] < 0.95 or rgba[1] < 0.95 or rgba[2] < 0.95:
            base[0] *= rgba[0]
            base[1] *= rgba[1]
            base[2] *= rgba[2]

    return (base[0], base[1], base[2], 1.0)


def configure_material(material, image_paths: dict, colors: dict, dry_run: bool):
    force_flat_color = should_force_flat_color(material.name)

    if dry_run:
        print(f"[dry-run] {material.name}: {image_paths} | force_flat={force_flat_color}")
        return

    clear_material_nodes(material)

    nodes = material.node_tree.nodes
    links = material.node_tree.links

    output_node = nodes.new(type="ShaderNodeOutputMaterial")
    output_node.location = (600, 0)

    shader_node = nodes.new(type="ShaderNodeBsdfPrincipled")
    shader_node.location = (300, 0)
    shader_node.inputs["Roughness"].default_value = 0.85
    links.new(shader_node.outputs["BSDF"], output_node.inputs["Surface"])

    if force_flat_color:
        shader_node.inputs["Base Color"].default_value = build_flat_color(material.name, colors)
    elif image_paths.get("base_color"):
        base_node = create_tex_image_node(material, image_paths["base_color"], (-600, 180), False)

        mix_color = colors.get("AA_BaseMixColor") or colors.get("BaseColor")
        if mix_color:
            tint_node = create_rgb_node(material, color_to_rgba(mix_color), (-600, 20))
            multiply_node = nodes.new(type="ShaderNodeMixRGB")
            multiply_node.blend_type = "MULTIPLY"
            multiply_node.inputs["Fac"].default_value = 1.0
            multiply_node.location = (-250, 120)

            links.new(base_node.outputs["Color"], multiply_node.inputs["Color1"])
            links.new(tint_node.outputs["Color"], multiply_node.inputs["Color2"])
            links.new(multiply_node.outputs["Color"], shader_node.inputs["Base Color"])
        else:
            links.new(base_node.outputs["Color"], shader_node.inputs["Base Color"])
    else:
        flat_color = colors.get("AA_BaseMixColor") or colors.get("BaseColor")
        if flat_color:
            shader_node.inputs["Base Color"].default_value = color_to_rgba(flat_color)
        else:
            shader_node.inputs["Base Color"].default_value = DEFAULT_FLAT_COLOR

    if image_paths.get("normal"):
        normal_texture = create_tex_image_node(material, image_paths["normal"], (-600, -140), True)

        try:
            separate_node = nodes.new(type="ShaderNodeSeparateColor")
            combine_node = nodes.new(type="ShaderNodeCombineColor")
            separate_red_output = "Red"
            separate_green_output = "Green"
            separate_blue_output = "Blue"
            combine_red_input = "Red"
            combine_green_input = "Green"
            combine_blue_input = "Blue"
        except RuntimeError:
            separate_node = nodes.new(type="ShaderNodeSeparateRGB")
            combine_node = nodes.new(type="ShaderNodeCombineRGB")
            separate_red_output = "R"
            separate_green_output = "G"
            separate_blue_output = "B"
            combine_red_input = "R"
            combine_green_input = "G"
            combine_blue_input = "B"

        separate_node.location = (-360, -140)

        invert_node = nodes.new(type="ShaderNodeInvert")
        invert_node.location = (-160, -210)

        combine_node.location = (40, -140)

        normal_map_node = nodes.new(type="ShaderNodeNormalMap")
        normal_map_node.location = (180, -140)
        normal_map_node.inputs["Strength"].default_value = 1.0

        links.new(normal_texture.outputs["Color"], separate_node.inputs["Color"])
        links.new(separate_node.outputs[separate_red_output], combine_node.inputs[combine_red_input])
        links.new(separate_node.outputs[separate_blue_output], combine_node.inputs[combine_blue_input])
        links.new(separate_node.outputs[separate_green_output], invert_node.inputs["Color"])
        links.new(invert_node.outputs["Color"], combine_node.inputs[combine_green_input])
        links.new(combine_node.outputs["Color"], normal_map_node.inputs["Color"])
        links.new(normal_map_node.outputs["Normal"], shader_node.inputs["Normal"])

    if image_paths.get("emissive"):
        emissive_node = create_tex_image_node(material, image_paths["emissive"], (-600, -420), False)
        emissive_input_name = "Emission Color" if "Emission Color" in shader_node.inputs else "Emission"
        links.new(emissive_node.outputs["Color"], shader_node.inputs[emissive_input_name])

    material.blend_method = "OPAQUE"
    material.use_backface_culling = False


def main():
    args = parse_args()

    export_root_str = args.export_root or DEFAULT_EXPORT_ROOT
    materials_dir_str = args.materials_dir or DEFAULT_MATERIALS_DIR
    textures_dir_str = args.textures_dir or DEFAULT_TEXTURES_DIR

    export_root = Path(export_root_str)
    if not export_root.exists():
        raise FileNotFoundError(f"export root not found: {export_root}")

    materials_dir = Path(materials_dir_str) if materials_dir_str else export_root
    textures_dir = Path(textures_dir_str) if textures_dir_str else export_root

    if not args.skip_dedup:
        removed_count = deduplicate_materials()
        print(f"[MI] deduplicated materials: {removed_count}")

    material_json_index = build_material_json_index(materials_dir)
    image_index = build_image_index(textures_dir)
    target_materials = get_target_materials(args.only_selected)

    print(f"[MI] material json files indexed: {len(material_json_index)}")
    print(f"[MI] texture files indexed      : {len(image_index)}")
    print(f"[MI] target materials          : {len(target_materials)}")

    applied = 0
    skipped = 0

    for material in target_materials:
        normalized_name = normalize_material_name(material.name)
        json_path = material_json_index.get(normalized_name.lower())

        if json_path is None:
            print(f"[MI] skip: {material.name} -> json not found")
            skipped += 1
            continue

        textures, colors, _scalars = parse_mi_json(json_path)

        image_paths = {
            "base_color": resolve_image_path(
                find_first_texture(textures, BASE_COLOR_KEYS, ("basecolor", "diffuse", "_bc")),
                image_index,
            ),
            "normal": resolve_image_path(
                find_first_texture(textures, NORMAL_KEYS, ("normal", "_n")),
                image_index,
            ),
            "specular": resolve_image_path(
                find_first_texture(textures, SPECULAR_KEYS, ("specular", "_m", "mask")),
                image_index,
            ),
            "roughness": resolve_image_path(
                find_first_texture(textures, ROUGHNESS_KEYS, ("roughness",)),
                image_index,
            ),
            "emissive": resolve_image_path(
                find_first_texture(textures, EMISSIVE_KEYS, ("emissive", "emission")),
                image_index,
            ),
        }

        print(
            f"[MI] apply: {material.name} | "
            f"base={image_paths['base_color']} | normal={image_paths['normal']}"
        )

        configure_material(material, image_paths, colors, args.dry_run)
        applied += 1

    print("")
    print("========== RESULT ==========")
    print(f"Applied: {applied}")
    print(f"Skipped: {skipped}")


if __name__ == "__main__":
    main()
