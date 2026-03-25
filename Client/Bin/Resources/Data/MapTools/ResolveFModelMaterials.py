#!/usr/bin/env python3
import argparse
import json
import shutil
import subprocess
import sys
import uuid
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
# [변경] 스크립트가 MapTools 폴더로 이동했으므로 실제 Data 루트는 부모 폴더다.
RESOURCE_DATA_DIR = SCRIPT_DIR.parent


def resolve_data_relative_path(relative_path: str) -> Path:
    # [추가] Data 루트 기준 상대경로를 절대경로로 고정해 배치 실행 위치 영향을 없앤다.
    return (RESOURCE_DATA_DIR / Path(relative_path)).resolve()

DEFAULT_MESH_ROOT = resolve_data_relative_path(
    r"..\StaticMesh\LobbyMapAssets\KonohaVilliage"
)

DEFAULT_MI_ROOT = resolve_data_relative_path(
    r"..\Materials\FModel\KonohaVillage_BORUTO"
)

DEFAULT_TEXTURE_ROOT = resolve_data_relative_path(
    r"..\Textures\FModel\KonohaVillage_BORUTO"
)

DEFAULT_COPY_TEXTURE_ROOT = resolve_data_relative_path(
    r"..\Textures\FModel\KonohaVillage_BORUTO"
)

# 새 MaterialInstance 에셋 출력 위치
DEFAULT_MATINST_ROOT = resolve_data_relative_path(
    r"..\Materials\FModel\KonohaVillage_BORUTO"
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
    parser = argparse.ArgumentParser(description="Resolve FModel MI json into engine MaterialInstance assets")
    parser.add_argument("--mesh-root", type=Path, default=DEFAULT_MESH_ROOT)
    parser.add_argument("--mi-root", type=Path, default=DEFAULT_MI_ROOT)
    parser.add_argument("--texture-root", type=Path, default=DEFAULT_TEXTURE_ROOT)
    parser.add_argument(
        "--extra-texture-root",
        type=Path,
        action="append",
        default=[],
        help="Additional texture roots to index together with --texture-root",
    )
    parser.add_argument("--copy-textures-to", type=Path, default=DEFAULT_COPY_TEXTURE_ROOT)
    parser.add_argument("--matinst-root", type=Path, default=DEFAULT_MATINST_ROOT)

    # 새 2-pass 모드
    parser.add_argument(
        "--mode",
        choices=["emit_matinst", "bind_mesh", "all"],
        required=True,
        help="emit_matinst: .matinst.json 생성, bind_mesh: GUID 연결, all: 생성+meta+연결 일괄 실행"
    )

    parser.add_argument("--run-level-convert", action="store_true")
    parser.add_argument("--level-map-root", type=Path, help="MapJSON folder to pass to ConvertFModelLevel.py")
    parser.add_argument("--level-guid-map", type=Path, help="Mesh GUID map json to pass to ConvertFModelLevel.py")
    parser.add_argument("--level-out-dir", type=Path, help="Output folder for generated .level.json files")
    parser.add_argument("--dry-run", action="store_true")
    parser.add_argument("--no-copy", action="store_true")
    return parser.parse_args()


def load_json(path: Path):
    with path.open("r", encoding="utf-8-sig") as f:
        return json.load(f)


def save_json(path: Path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, ensure_ascii=False)


def get_mi_key(path: Path) -> str:
    name = path.name
    if name.lower().endswith(".props.txt"):
        return name[:-len(".props.txt")].lower()
    return path.stem.lower()


def build_mi_index(mi_root: Path):
    index = {}

    # 우선순위:
    # 1. .json
    # 2. .props.txt
    # 3. .mat
    for pattern in ("*.json", "*.props.txt", "*.mat"):
        for path in mi_root.rglob(pattern):
            key = get_mi_key(path)
            if key not in index:
                index[key] = path

    return index


def parse_props_txt(path: Path):
    textures = {}
    scalars = {}
    colors = {}
    properties = {}

    current_section = None
    current_entry = None

    for raw_line in path.read_text(encoding="utf-8-sig", errors="ignore").splitlines():
        line = raw_line.strip()
        if not line:
            continue

        if line.startswith("Parent = "):
            properties["ParentObjectName"] = line[len("Parent = "):].strip()
            continue

        if line.startswith("TextureParameterValues["):
            current_section = "texture"
            current_entry = {}
            continue
        if line.startswith("ScalarParameterValues["):
            current_section = "scalar"
            current_entry = {}
            continue
        if line.startswith("VectorParameterValues["):
            current_section = "vector"
            current_entry = {}
            continue

        if line == "{":
            continue

        if line == "}":
            if current_section == "texture":
                name = current_entry.get("ParameterName", "")
                value = current_entry.get("ParameterValue", "")
                if name and value:
                    textures[name] = value
            elif current_section == "scalar":
                name = current_entry.get("ParameterName", "")
                value = current_entry.get("ParameterValue", "")
                if name and value != "":
                    try:
                        scalars[name] = float(value)
                    except ValueError:
                        pass
            elif current_section == "vector":
                name = current_entry.get("ParameterName", "")
                value = current_entry.get("ParameterValue", {})
                if name and value:
                    colors[name] = value

            current_section = None
            current_entry = None
            continue

        if current_entry is None:
            continue

        if line.startswith("ParameterName = "):
            current_entry["ParameterName"] = line[len("ParameterName = "):].strip()
            continue

        if line.startswith("ParameterValue = Texture2D"):
            current_entry["ParameterValue"] = line[len("ParameterValue = "):].strip()
            continue

        if line.startswith("ParameterValue = {") and current_section == "vector":
            current_entry["ParameterValue"] = {}
            inner = line[len("ParameterValue = {"):].rstrip("}").strip()
            for part in inner.split(","):
                key_value = part.strip().split("=", 1)
                if len(key_value) != 2:
                    continue
                key, value = key_value
                try:
                    current_entry["ParameterValue"][key.strip()] = float(value.strip())
                except ValueError:
                    current_entry["ParameterValue"][key.strip()] = value.strip()
            continue

        if line.startswith("ParameterValue = ") and current_section == "scalar":
            current_entry["ParameterValue"] = line[len("ParameterValue = "):].strip()
            continue

    return {
        "Textures": textures,
        "Parameters": {
            "BlendMode": 0,
            "ShadingModel": 0,
            "Colors": colors,
            "Scalars": scalars,
            "Switches": {},
            "Properties": properties,
            "HasTopDiffuse": False,
            "HasTopNormals": False,
            "HasTopSpecularMasks": False,
            "HasTopEmissive": False,
            "IsTranslucent": False,
            "IsNull": False,
        },
    }


def parse_mat_file(path: Path):
    textures = {}
    name_map = {
        "Diffuse": "AA_BaseColorMap",
        "Normal": "BA_NomalMap",
    }

    for raw_line in path.read_text(encoding="utf-8-sig", errors="ignore").splitlines():
        line = raw_line.strip()
        if "=" not in line:
            continue

        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip()
        if not value:
            continue

        mapped = name_map.get(key, key)
        textures[mapped] = value

    return {
        "Textures": textures,
        "Parameters": {
            "BlendMode": 0,
            "ShadingModel": 0,
            "Colors": {},
            "Scalars": {},
            "Switches": {},
            "Properties": {},
            "HasTopDiffuse": False,
            "HasTopNormals": False,
            "HasTopSpecularMasks": False,
            "HasTopEmissive": False,
            "IsTranslucent": False,
            "IsNull": False,
        },
    }


def load_mi_data(path: Path):
    suffixes = [part.lower() for part in path.suffixes]

    if suffixes[-1:] == [".json"]:
        return normalize_mi_data(load_json(path))

    if suffixes[-2:] == [".props", ".txt"]:
        return parse_props_txt(path)

    if suffixes[-1:] == [".mat"]:
        return parse_mat_file(path)

    raise TypeError(f"Unsupported MI source file: {path}")


def normalize_mi_data(raw_data):
    # 이미 엔진용으로 정리된 MI JSON이면 그대로 사용
    if isinstance(raw_data, dict) and ("Textures" in raw_data or "Parameters" in raw_data):
        return raw_data

    # FModel 원본 export는 [ { "Properties": ... } ] 형태가 많다.
    if isinstance(raw_data, list):
        for item in raw_data:
            if isinstance(item, dict) and "Properties" in item:
                return normalize_fmodel_material_instance(item)
        return {"Textures": {}, "Parameters": {}}

    if isinstance(raw_data, dict) and "Properties" in raw_data:
        return normalize_fmodel_material_instance(raw_data)

    raise TypeError(f"Unsupported MI json root type: {type(raw_data).__name__}")


def normalize_fmodel_material_instance(item: dict):
    # FModel MaterialInstance export를 기존 resolver가 기대하는 공통 포맷으로 평탄화
    props = item.get("Properties", {})

    textures = {}
    for entry in props.get("TextureParameterValues", []):
        if not isinstance(entry, dict):
            continue

        parameter_name = entry.get("ParameterName", "")
        parameter_value = entry.get("ParameterValue", {})
        if not parameter_name:
            continue

        object_path = ""
        if isinstance(parameter_value, dict):
            object_path = parameter_value.get("ObjectPath", "") or parameter_value.get("ObjectName", "")
        elif isinstance(parameter_value, str):
            object_path = parameter_value

        if object_path:
            textures[parameter_name] = object_path

    scalars = {}
    for entry in props.get("ScalarParameterValues", []):
        if not isinstance(entry, dict):
            continue

        parameter_name = entry.get("ParameterName", "")
        if not parameter_name:
            continue

        scalars[parameter_name] = float(entry.get("ParameterValue", 0.0))

    colors = {}
    for entry in props.get("VectorParameterValues", []):
        if not isinstance(entry, dict):
            continue

        parameter_name = entry.get("ParameterName", "")
        parameter_value = entry.get("ParameterValue", {})
        if not parameter_name or not isinstance(parameter_value, dict):
            continue

        colors[parameter_name] = {
            "R": float(parameter_value.get("R", 1.0)),
            "G": float(parameter_value.get("G", 1.0)),
            "B": float(parameter_value.get("B", 1.0)),
            "A": float(parameter_value.get("A", 1.0)),
            "Hex": parameter_value.get("Hex", ""),
        }

    switches = {}
    static_params = item.get("StaticParameters", {}) or {}
    for entry in static_params.get("StaticSwitchParameters", []):
        if not isinstance(entry, dict):
            continue

        parameter_info = entry.get("ParameterInfo", {})
        parameter_name = parameter_info.get("Name", "")
        if not parameter_name:
            continue

        switches[parameter_name] = bool(entry.get("Value", False))

    parent = props.get("Parent", {}) if isinstance(props.get("Parent", {}), dict) else {}
    properties = {}
    if parent:
        properties["ParentObjectName"] = parent.get("ObjectName", "")
        properties["ParentObjectPath"] = parent.get("ObjectPath", "")

    return {
        "Textures": textures,
        "Parameters": {
            "BlendMode": 0,
            "ShadingModel": 0,
            "Colors": colors,
            "Scalars": scalars,
            "Switches": switches,
            "Properties": properties,
            "HasTopDiffuse": False,
            "HasTopNormals": False,
            "HasTopSpecularMasks": False,
            "HasTopEmissive": False,
            "IsTranslucent": False,
            "IsNull": False,
        },
    }


def build_texture_roots(primary_root: Path, extra_roots: list[Path]):
    roots = []
    seen = set()

    for root in [primary_root, *extra_roots]:
        if root is None:
            continue

        resolved = root.resolve()
        key = str(resolved).lower()

        if key in seen:
            continue

        seen.add(key)
        roots.append(resolved)

    return roots


def build_texture_index(texture_roots: list[Path]):
    index = {}
    duplicates = {}

    for texture_root in texture_roots:
        if not texture_root.exists():
            print(f"[WARN] texture root not found: {texture_root}")
            continue

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


def build_matinst_index(matinst_root: Path):
    index = {}
    for path in matinst_root.rglob("*.matinst.json"):
        index[path.stem.replace(".matinst", "").lower()] = path
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


def resolve_source_texture_path(unreal_ref: str, texture_index: dict):
    tex_name = extract_texture_asset_name(unreal_ref)
    if not tex_name:
        return None
    return texture_index.get(tex_name.lower())


def copy_texture_and_make_relative(src_path: Path, owner_json_path: Path, copy_root: Path, dry_run: bool) -> str:
    # 새 동작: matinst 파일 기준 상대경로를 만들기 위해 owner_json_path를 받는다.
    copy_root.mkdir(parents=True, exist_ok=True)
    dst_path = copy_root / src_path.name

    if not dry_run and not dst_path.exists():
        shutil.copy2(src_path, dst_path)

    rel = Path(__import__("os").path.relpath(str(dst_path), str(owner_json_path.parent)))
    return rel.as_posix()


def make_texture_path(src_path, owner_json_path: Path, copy_root, dry_run: bool) -> str:
    if src_path is None:
        return ""

    if copy_root is None:
        return str(src_path)

    return copy_texture_and_make_relative(src_path, owner_json_path, copy_root, dry_run)


def normalize_generated_matinst_paths(matinst_root: Path, preferred_texture_root: Path, dry_run: bool):
    normalized = 0
    updated_files = 0

    if not matinst_root.exists() or not preferred_texture_root.exists():
        return normalized, updated_files

    for matinst_path in matinst_root.rglob("*.matinst.json"):
        root = load_json(matinst_path)
        textures = root.get("textures", [])

        if not isinstance(textures, list):
            continue

        changed = False

        for texture in textures:
            if not isinstance(texture, dict):
                continue

            raw_path = texture.get("path", "")
            if not raw_path:
                continue

            current_path = Path(raw_path)
            if current_path.is_absolute():
                continue

            preferred_path = preferred_texture_root / current_path.name
            if not preferred_path.exists():
                continue

            rel = Path(__import__("os").path.relpath(str(preferred_path), str(matinst_path.parent))).as_posix()
            if raw_path == rel:
                continue

            texture["path"] = rel
            normalized += 1
            changed = True

        if changed:
            updated_files += 1
            if not dry_run:
                save_json(matinst_path, root)

    return normalized, updated_files


def resolve_generic_pbr(material_name: str, mi_data: dict, texture_index: dict, owner_json_path: Path,
                        copy_root, dry_run: bool):
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
        "version": 1,
        "name": material_name,
        "profile": "generic_pbr",
        "parent": "M_GenericPBR",
        "base_color_factor": get_color4(colors, "AA_BaseMixColor", [1.0, 1.0, 1.0, 1.0]),
        "normal_strength": float(scalars.get("AA_NormalMapBoost", 1.0)),
        "blend_mode": int(params.get("BlendMode", 0)),
        "textures": [],
    }

    base_color_path = make_texture_path(resolve_source_texture_path(base_color_ref, texture_index), owner_json_path, copy_root, dry_run)
    normal_path = make_texture_path(resolve_source_texture_path(normal_ref, texture_index), owner_json_path, copy_root, dry_run)
    specular_path = make_texture_path(resolve_source_texture_path(specular_ref, texture_index), owner_json_path, copy_root, dry_run)
    roughness_path = make_texture_path(resolve_source_texture_path(roughness_ref, texture_index), owner_json_path, copy_root, dry_run)
    emissive_path = make_texture_path(resolve_source_texture_path(emissive_ref, texture_index), owner_json_path, copy_root, dry_run)

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


def resolve_flat_color_normal(material_name: str, mi_data: dict, texture_index: dict, owner_json_path: Path,
                              copy_root, dry_run: bool):
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
        "version": 1,
        "name": material_name,
        "profile": "flat_color_normal",
        "parent": "M_ENV_FlatColorNormal",
        "base_color_factor": base_color,
        "shadow_color": shadow_color,
        "normal_strength": float(scalars.get("AA_NormalMapBoost", 1.0)),
        "blend_mode": int(params.get("BlendMode", 0)),
        "textures": [],
    }

    normal_path = make_texture_path(resolve_source_texture_path(normal_ref, texture_index), owner_json_path, copy_root, dry_run)
    specular_path = make_texture_path(resolve_source_texture_path(specular_ref, texture_index), owner_json_path, copy_root, dry_run)

    if normal_path:
        out["textures"].append({"slot": "normal", "index": 0, "path": normal_path})
    if specular_path:
        out["textures"].append({"slot": "specular", "index": 0, "path": specular_path})

    return out


def write_material_instance_file(mi_name: str, resolved_data: dict, out_dir: Path, dry_run: bool) -> Path:
    # 새 함수: 정규화된 엔진용 MaterialInstance 에셋 파일을 쓴다.
    path = out_dir / f"{mi_name}.matinst.json"

    if not dry_run:
        save_json(path, resolved_data)

    return path


def ensure_material_instance_meta(matinst_path: Path, dry_run: bool) -> str:
    # .meta가 이미 있으면 기존 GUID를 재사용하고, 없으면 새로 만든다.
    meta_path = Path(str(matinst_path) + ".meta")
    if meta_path.exists():
        data = load_json(meta_path)
        guid = data.get("guid", "")
        if guid:
            return guid

    guid = str(uuid.uuid4())
    meta = {
        "guid": guid,
        "type": "material_instance",
    }

    if not dry_run:
        save_json(meta_path, meta)

    return guid


def get_or_create_material_instance_guid(matinst_path: Path, dry_run: bool) -> str:
    meta_path = Path(str(matinst_path) + ".meta")
    if meta_path.exists():
        data = load_json(meta_path)
        guid = data.get("guid", "")
        if guid:
            return guid

    return ensure_material_instance_meta(matinst_path, dry_run)


def build_material_instance_payload(material_name: str, mi_path: Path, mi_data: dict, texture_index: dict,
                                    profile_config: dict, matinst_root: Path, copy_root, dry_run: bool):
    matinst_path = matinst_root / f"{material_name}.matinst.json"
    profile = detect_profile(material_name, profile_config)

    if profile == "flat_color_normal":
        payload = resolve_flat_color_normal(material_name, mi_data, texture_index, matinst_path, copy_root, dry_run)
    else:
        payload = resolve_generic_pbr(material_name, mi_data, texture_index, matinst_path, copy_root, dry_run)

    payload["source"] = {"fmodel_mi_json": mi_path.name}
    return payload


def collect_material_names_from_mesh_files(mesh_root: Path):
    names = set()

    for mat_path in mesh_root.rglob("*.material.json"):
        root = load_json(mat_path)
        materials = root.get("materials", [])
        if not isinstance(materials, list):
            continue

        for entry in materials:
            if not isinstance(entry, dict):
                continue
            material_name = entry.get("material_name", "").strip()
            if material_name:
                names.add(material_name)

    return sorted(names)


def emit_material_instances(mesh_root: Path, mi_index: dict, texture_index: dict, profile_config: dict,
                            matinst_root: Path, copy_root, dry_run: bool):
    # 1-pass: 메시에서 실제로 쓰는 material_name만 뽑아서 .matinst.json 생성
    material_names = collect_material_names_from_mesh_files(mesh_root)

    emitted = 0
    unresolved = 0

    for material_name in material_names:
        mi_path = mi_index.get(material_name.lower())
        if not mi_path:
            unresolved += 1
            print(f"[MISS-MI] {material_name}")
            continue

        try:
            mi_data = load_mi_data(mi_path)

            payload = build_material_instance_payload(
                material_name, mi_path, mi_data, texture_index, profile_config, matinst_root, copy_root, dry_run
            )

            out_path = write_material_instance_file(material_name, payload, matinst_root, dry_run)
            emitted += 1
            print(f"[EMIT] {out_path.name}")
        except Exception as e:
            unresolved += 1
            print(f"[SKIP-ERR] {material_name} -> {e}")

    print("")
    print("========== EMIT RESULT ==========")
    print(f"Material names scanned : {len(material_names)}")
    print(f"MatInst emitted        : {emitted}")
    print(f"Missing MI json        : {unresolved}")


def rewrite_mesh_material_to_guid(entry: dict, guid: str) -> dict:
    # 새 함수: 메시 머티리얼 엔트리를 공용 MaterialInstance GUID 참조로 바꾼다.
    return {
        "material_name": entry.get("material_name", "Material"),
        "material_instance_guid": guid,
    }


def bind_mesh_materials(mesh_root: Path, matinst_root: Path, dry_run: bool):
    # 2-pass: .matinst.json.meta 에서 GUID를 읽거나 직접 생성해서 SM_*.material.json에 연결
    matinst_index = build_matinst_index(matinst_root)

    processed = 0
    unresolved = 0
    missing_meta = 0

    for mat_path in mesh_root.rglob("*.material.json"):
        root = load_json(mat_path)
        materials = root.get("materials", [])

        if not isinstance(materials, list):
            continue

        new_materials = []

        for entry in materials:
            if not isinstance(entry, dict):
                continue

            material_name = entry.get("material_name", "").strip()
            if not material_name:
                new_materials.append(entry)
                continue

            matinst_path = matinst_index.get(material_name.lower())
            if not matinst_path:
                unresolved += 1
                new_materials.append(entry)
                continue

            guid = get_or_create_material_instance_guid(matinst_path, dry_run)
            if not guid:
                missing_meta += 1
                new_materials.append(entry)
                continue

            new_materials.append(rewrite_mesh_material_to_guid(entry, guid))

        root["version"] = 4
        root["materials"] = new_materials

        if not dry_run:
            save_json(mat_path, root)

        processed += 1
        print(f"[BIND] {mat_path.name}")

    print("")
    print("========== BIND RESULT ==========")
    print(f"Processed mesh files : {processed}")
    print(f"Missing matinst/meta : {missing_meta}")
    print(f"Still unresolved     : {unresolved}")


def run_convert_level(dry_run: bool, map_root: Path | None, guid_map: Path | None, out_dir: Path | None):
    if dry_run:
        print("[LEVEL] dry-run: skipped ConvertFModelLevel.py")
        return

    script_path = Path(__file__).resolve().parent / "ConvertFModelLevel.py"
    command = [sys.executable, str(script_path)]

    if map_root is not None:
        command.extend(["--map-root", str(map_root)])
    if guid_map is not None:
        command.extend(["--guid-map", str(guid_map)])
    if out_dir is not None:
        command.extend(["--out-dir", str(out_dir)])

    result = subprocess.run(command, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"ConvertFModelLevel.py failed with exit code {result.returncode}")


def main():
    args = parse_args()

    mesh_root = args.mesh_root
    mi_root = args.mi_root
    texture_root = args.texture_root
    extra_texture_roots = args.extra_texture_root
    copy_root = None if args.no_copy else args.copy_textures_to
    matinst_root = args.matinst_root
    profile_config = DEFAULT_PROFILE_CONFIG
    texture_roots = build_texture_roots(texture_root, extra_texture_roots)

    if not mesh_root.exists():
        raise FileNotFoundError(f"mesh root not found: {mesh_root}")
    if args.mode != "bind_mesh":
        if not mi_root.exists():
            raise FileNotFoundError(f"mi root not found: {mi_root}")
        if not texture_roots:
            raise FileNotFoundError("texture roots are empty")
        if not any(root.exists() for root in texture_roots):
            raise FileNotFoundError("no valid texture roots found")

    if args.mode == "emit_matinst":
        mi_index = build_mi_index(mi_root)
        texture_index = build_texture_index(texture_roots)

        print(f"[Resolver] MI indexed      : {len(mi_index)}")
        print(f"[Resolver] Texture roots    : {len(texture_roots)}")
        for root in texture_roots:
            print(f"  - {root}")
        print(f"[Resolver] Textures indexed: {len(texture_index)}")

        emit_material_instances(
            mesh_root=mesh_root,
            mi_index=mi_index,
            texture_index=texture_index,
            profile_config=profile_config,
            matinst_root=matinst_root,
            copy_root=copy_root,
            dry_run=args.dry_run,
        )

        normalized, updated_files = normalize_generated_matinst_paths(
            matinst_root=matinst_root,
            preferred_texture_root=copy_root if copy_root is not None else Path(),
            dry_run=args.dry_run,
        )
        if updated_files:
            print(f"[Resolver] MatInst paths normalized: {normalized} entries in {updated_files} files")

    elif args.mode == "bind_mesh":
        normalized, updated_files = normalize_generated_matinst_paths(
            matinst_root=matinst_root,
            preferred_texture_root=copy_root if copy_root is not None else Path(),
            dry_run=args.dry_run,
        )
        if updated_files:
            print(f"[Resolver] MatInst paths normalized: {normalized} entries in {updated_files} files")

        bind_mesh_materials(
            mesh_root=mesh_root,
            matinst_root=matinst_root,
            dry_run=args.dry_run,
        )

    elif args.mode == "all":
        mi_index = build_mi_index(mi_root)
        texture_index = build_texture_index(texture_roots)

        print(f"[Resolver] MI indexed      : {len(mi_index)}")
        print(f"[Resolver] Texture roots    : {len(texture_roots)}")
        for root in texture_roots:
            print(f"  - {root}")
        print(f"[Resolver] Textures indexed: {len(texture_index)}")

        emit_material_instances(
            mesh_root=mesh_root,
            mi_index=mi_index,
            texture_index=texture_index,
            profile_config=profile_config,
            matinst_root=matinst_root,
            copy_root=copy_root,
            dry_run=args.dry_run,
        )

        normalized, updated_files = normalize_generated_matinst_paths(
            matinst_root=matinst_root,
            preferred_texture_root=copy_root if copy_root is not None else Path(),
            dry_run=args.dry_run,
        )
        if updated_files:
            print(f"[Resolver] MatInst paths normalized: {normalized} entries in {updated_files} files")

        bind_mesh_materials(
            mesh_root=mesh_root,
            matinst_root=matinst_root,
            dry_run=args.dry_run,
        )

        if args.run_level_convert:
            run_convert_level(
                args.dry_run,
                args.level_map_root,
                args.level_guid_map,
                args.level_out_dir,
            )


if __name__ == "__main__":
    main()
