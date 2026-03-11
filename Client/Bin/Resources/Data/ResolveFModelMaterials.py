#!/usr/bin/env python3
import argparse
import json
import shutil
from pathlib import Path


DEFAULT_MESH_ROOT = Path(
    r"D:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\StaticMesh\LobbyMapAssets\KonohaVilliage"
)

DEFAULT_MI_ROOT = Path(
    r"C:\Users\Moon In Su\Desktop\FModel\Output\Exports\NARUTO\Content\Environments\LobbyMapAssets\LM_KonohaVillage_BORUTO\Materials"
)

DEFAULT_TEXTURE_ROOT = Path(
    r"C:\Users\Moon In Su\Desktop\FModel\Output\Exports\NARUTO\Content\Environments\LobbyMapAssets\LM_KonohaVillage_BORUTO\Textures"
)

DEFAULT_COPY_TEXTURE_ROOT = Path(
    r"D:\GitDesktop\Dx11_Naruto\Client\Bin\Resources\Textures\FModel\KonohaVillage_BORUTO"
)

DEFAULT_PROFILE_CONFIG = {
    "flat_color_normal": [
        "MountainSide",
        "Mountain",
        "Cliff",
        "RockWall",
        "FaceRock",
        "Backdrop",
    ],
    "masked_foliage": [
        "Leaf",
        "Bush",
        "Canopy",
        "Vine",
        "Ivy",
    ],
}

DEFAULT_FLAT_COLOR = [0.77, 0.68, 0.58, 1.0]
IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".tga", ".dds", ".bmp"}


def parse_args():
    parser = argparse.ArgumentParser(description="Resolve FModel MI json into engine material.json")
    parser.add_argument("--mesh-root", type=Path, default=DEFAULT_MESH_ROOT)
    parser.add_argument("--mi-root", type=Path, default=DEFAULT_MI_ROOT)
    parser.add_argument("--texture-root", type=Path, default=DEFAULT_TEXTURE_ROOT)
    parser.add_argument("--copy-textures-to", type=Path, default=DEFAULT_COPY_TEXTURE_ROOT)
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--no-copy", action="store_true")
    return parser.parse_args()


def load_json(path: Path):
    with path.open("r", encoding="utf-8-sig") as f:
        return json.load(f)


def save_json(path: Path, data):
    with path.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)


def build_mi_index(mi_root: Path):
    index = {}
    for path in mi_root.rglob("*.json"):
        index[path.stem.lower()] = path
    return index


def build_texture_index(texture_root: Path):
    index = {}
    duplicates = {}

    for path in texture_root.rglob("*"):
        if not path.is_file():
            continue
        if path.suffix.lower() not in IMAGE_EXTENSIONS:
            continue

        key = path.stem.lower()
        if key in index:
            duplicates.setdefault(key, []).append(path)
            continue

        index[key] = path

    if duplicates:
        print("[WARN] duplicate texture stems found. first match will be used.")
        for key, paths in list(duplicates.items())[:20]:
            print(f"  {key} -> {index[key].name}, {len(paths)} more")

    return index


def extract_texture_asset_name(unreal_ref: str) -> str:
    if not unreal_ref:
        return ""

    value = unreal_ref.strip().strip('"').strip("'")

    if "." in value:
        value = value.rsplit(".", 1)[0]

    if "/" in value:
        value = value.rsplit("/", 1)[-1]

    return value


def find_first_key(d: dict, keys):
    for key in keys:
        value = d.get(key, "")
        if value:
            return value
    return ""


def get_textures(mi_data: dict):
    return mi_data.get("Textures", {})


def get_parameters(mi_data: dict):
    return mi_data.get("Parameters", {})


def get_colors(mi_data: dict):
    return get_parameters(mi_data).get("Colors", {})


def get_scalars(mi_data: dict):
    return get_parameters(mi_data).get("Scalars", {})


def get_color4(colors: dict, key: str, default=None):
    if default is None:
        default = [1.0, 1.0, 1.0, 1.0]

    value = colors.get(key)
    if not isinstance(value, dict):
        return default

    return [
        float(value.get("R", default[0])),
        float(value.get("G", default[1])),
        float(value.get("B", default[2])),
        float(value.get("A", default[3])),
    ]


def is_near_white(color):
    return color[0] >= 0.95 and color[1] >= 0.95 and color[2] >= 0.95


def apply_flat_color_tint(base_color, tint):
    return [
        base_color[0] * tint[0],
        base_color[1] * tint[1],
        base_color[2] * tint[2],
        1.0,
    ]


def detect_profile(material_name: str, profile_config: dict) -> str:
    lower = material_name.lower()

    for keyword in profile_config.get("flat_color_normal", []):
        if keyword.lower() in lower:
            return "flat_color_normal"

    for keyword in profile_config.get("masked_foliage", []):
        if keyword.lower() in lower:
            return "masked_foliage"

    return "generic_pbr"


def resolve_source_texture_path(unreal_ref: str, texture_index: dict) -> Path | None:
    tex_name = extract_texture_asset_name(unreal_ref)
    if not tex_name:
        return None
    return texture_index.get(tex_name.lower())


def copy_texture_and_make_relative(src_path: Path, material_json_path: Path, copy_root: Path, dry_run: bool) -> str:
    copy_root.mkdir(parents=True, exist_ok=True)
    dst_path = copy_root / src_path.name

    if not dry_run and not dst_path.exists():
        shutil.copy2(src_path, dst_path)

    relative = dst_path.relative_to(material_json_path.parent.anchor) if False else None
    relative = Path(Path.cwd()) if False else None

    rel = Path(
        Path(
            __import__("os").path.relpath(str(dst_path), str(material_json_path.parent))
        )
    )
    return rel.as_posix()


def make_texture_path(src_path: Path | None, material_json_path: Path, copy_root: Path | None, dry_run: bool) -> str:
    if src_path is None:
        return ""

    if copy_root is None:
        return str(src_path)

    return copy_texture_and_make_relative(src_path, material_json_path, copy_root, dry_run)


def resolve_generic_pbr(material_name: str, mi_data: dict, texture_index: dict, material_json_path: Path,
                        copy_root: Path | None, dry_run: bool):
    textures = get_textures(mi_data)
    colors = get_colors(mi_data)
    scalars = get_scalars(mi_data)
    params = get_parameters(mi_data)

    base_color_ref = find_first_key(textures, [
        "AA_Override_BaseColorMap",
        "AA_BaseColorMap",
        "PM_Diffuse",
    ])

    normal_ref = find_first_key(textures, [
        "BA_NomalMap",
        "BA_NormalMap",
        "PM_Normals",
    ])

    specular_ref = find_first_key(textures, [
        "PM_SpecularMasks",
    ])

    roughness_ref = find_first_key(textures, [
        "PM_Roughness",
    ])

    emissive_ref = find_first_key(textures, [
        "Emissive",
        "Emission",
    ])

    out = {
        "material_name": material_name,
        "profile": "generic_pbr",
        "base_color_factor": get_color4(colors, "AA_BaseMixColor", [1.0, 1.0, 1.0, 1.0]),
        "normal_strength": float(scalars.get("AA_NormalMapBoost", 1.0)),
        "blend_mode": int(params.get("BlendMode", 0)),
        "textures": [],
    }

    base_color_path = make_texture_path(
        resolve_source_texture_path(base_color_ref, texture_index),
        material_json_path,
        copy_root,
        dry_run,
    )
    normal_path = make_texture_path(
        resolve_source_texture_path(normal_ref, texture_index),
        material_json_path,
        copy_root,
        dry_run,
    )
    specular_path = make_texture_path(
        resolve_source_texture_path(specular_ref, texture_index),
        material_json_path,
        copy_root,
        dry_run,
    )
    roughness_path = make_texture_path(
        resolve_source_texture_path(roughness_ref, texture_index),
        material_json_path,
        copy_root,
        dry_run,
    )
    emissive_path = make_texture_path(
        resolve_source_texture_path(emissive_ref, texture_index),
        material_json_path,
        copy_root,
        dry_run,
    )

    if base_color_path:
        out["textures"].append({"slot": "base_color", "index": 0, "path": base_color_path})
    if normal_path:
        out["textures"].append({"slot": "normal", "index": 0, "path": normal_path})
    if specular_path:
        out["textures"].append({"slot": "specular", "index": 0, "path": specular_path})
    if roughness_path:
        out["textures"].append({"slot": "roughness", "index": 0, "path": roughness_path})
    if emissive_path:
        out["textures"].append({"slot": "emissive", "index": 0, "path": emissive_path})

    return out


def resolve_flat_color_normal(material_name: str, mi_data: dict, texture_index: dict, material_json_path: Path,
                              copy_root: Path | None, dry_run: bool):
    textures = get_textures(mi_data)
    colors = get_colors(mi_data)
    scalars = get_scalars(mi_data)
    params = get_parameters(mi_data)

    normal_ref = find_first_key(textures, [
        "BA_NomalMap",
        "BA_NormalMap",
        "PM_Normals",
    ])

    specular_ref = find_first_key(textures, [
        "PM_SpecularMasks",
    ])

    base_color = DEFAULT_FLAT_COLOR[:]
    mix_color = get_color4(colors, "AA_BaseMixColor", [1.0, 1.0, 1.0, 1.0])

    if not is_near_white(mix_color):
        base_color = apply_flat_color_tint(base_color, mix_color)

    shadow_color = get_color4(colors, "AA_Override_ShadowColor", None)
    if shadow_color is None:
        shadow_color = get_color4(colors, "AA_ShadowColor", [1.0, 1.0, 1.0, 1.0])

    out = {
        "material_name": material_name,
        "profile": "flat_color_normal",
        "base_color_factor": base_color,
        "shadow_color": shadow_color,
        "normal_strength": float(scalars.get("AA_NormalMapBoost", 1.0)),
        "blend_mode": int(params.get("BlendMode", 0)),
        "textures": [],
    }

    normal_path = make_texture_path(
        resolve_source_texture_path(normal_ref, texture_index),
        material_json_path,
        copy_root,
        dry_run,
    )
    specular_path = make_texture_path(
        resolve_source_texture_path(specular_ref, texture_index),
        material_json_path,
        copy_root,
        dry_run,
    )

    if normal_path:
        out["textures"].append({"slot": "normal", "index": 0, "path": normal_path})
    if specular_path:
        out["textures"].append({"slot": "specular", "index": 0, "path": specular_path})

    return out


def resolve_material_entry(entry: dict, mi_index: dict, texture_index: dict, profile_config: dict,
                           material_json_path: Path, copy_root: Path | None, dry_run: bool):
    material_name = entry.get("material_name", "").strip()
    if not material_name:
        return {
            "material_name": "Material",
            "profile": "unresolved",
            "textures": [],
        }

    mi_path = mi_index.get(material_name.lower())
    if not mi_path:
        return {
            "material_name": material_name,
            "profile": "unresolved",
            "textures": [],
        }

    mi_data = load_json(mi_path)
    profile = detect_profile(material_name, profile_config)

    if profile == "flat_color_normal":
        resolved = resolve_flat_color_normal(
            material_name, mi_data, texture_index, material_json_path, copy_root, dry_run
        )
    else:
        resolved = resolve_generic_pbr(
            material_name, mi_data, texture_index, material_json_path, copy_root, dry_run
        )

    resolved["source"] = {"mi_json": mi_path.name}
    return resolved


def process_material_file(path: Path, mi_index: dict, texture_index: dict, profile_config: dict,
                          copy_root: Path | None, dry_run: bool):
    root = load_json(path)
    materials = root.get("materials", [])

    if not isinstance(materials, list):
        print(f"[SKIP] invalid materials array: {path.name}")
        return False, 0

    new_materials = []
    unresolved_count = 0

    for entry in materials:
        if not isinstance(entry, dict):
            continue

        resolved = resolve_material_entry(
            entry, mi_index, texture_index, profile_config, path, copy_root, dry_run
        )
        if resolved.get("profile") == "unresolved":
            unresolved_count += 1

        new_materials.append(resolved)

    root["version"] = 3
    root["materials"] = new_materials

    if not dry_run:
        save_json(path, root)

    return True, unresolved_count


def main():
    args = parse_args()

    mesh_root = args.mesh_root
    mi_root = args.mi_root
    texture_root = args.texture_root
    copy_root = None if args.no_copy else args.copy_textures_to
    profile_config = DEFAULT_PROFILE_CONFIG

    if not mesh_root.exists():
        raise FileNotFoundError(f"mesh root not found: {mesh_root}")
    if not mi_root.exists():
        raise FileNotFoundError(f"mi root not found: {mi_root}")
    if not texture_root.exists():
        raise FileNotFoundError(f"texture root not found: {texture_root}")

    mi_index = build_mi_index(mi_root)
    texture_index = build_texture_index(texture_root)

    print(f"[Resolver] MI indexed      : {len(mi_index)}")
    print(f"[Resolver] Textures indexed: {len(texture_index)}")

    processed = 0
    failed = 0
    unresolved_total = 0

    for mat_path in mesh_root.rglob("*.material.json"):
        try:
            ok, unresolved = process_material_file(
                mat_path, mi_index, texture_index, profile_config, copy_root, args.dry_run
            )
            if ok:
                processed += 1
                unresolved_total += unresolved
                print(f"[OK] {mat_path.name} | unresolved={unresolved}")
            else:
                failed += 1
        except Exception as e:
            failed += 1
            print(f"[ERROR] {mat_path.name}: {e}")

    print("")
    print("========== RESULT ==========")
    print(f"Processed : {processed}")
    print(f"Failed    : {failed}")
    print(f"Unresolved: {unresolved_total}")


if __name__ == "__main__":
    main()
