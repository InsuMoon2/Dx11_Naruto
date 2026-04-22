#!/usr/bin/env python3
import argparse
import json
import re
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

# [추가] KonohaVillage02 바닥 StaticMesh를 마스크 바닥으로 강제 치환할 대상 머티리얼 이름 목록이다.
# [추가] 범위를 너무 넓히지 않기 위해 실제 Floor 청크에서 확인된 이름만 사용한다.
GROUND_OVERRIDE_MATERIAL_NAMES = (
    "MI_ENV_KNVLLG02_GROUNDSOIL_A",
    "MI_ENV_KNVLLG02_DISTANCEGROUND_A",
    "MI_ENV_KNVLLG02_SANDYFLOORTILES",
)

# [추가] 바닥 오버라이드에 사용할 기본 모래 텍스처 stem이다.
GROUND_OVERRIDE_BASE_COLOR_STEM = "T_ENV_KNVLLG_Ground_Sand_BC"
# [추가] 바닥 오버라이드에 사용할 잔디 블렌드 텍스처 stem이다.
GROUND_OVERRIDE_BLEND_COLOR_STEM = "T_ENV_KNVLLG_GrassBase_BC"
# [추가] 바닥 오버라이드에 사용할 분포 마스크 텍스처 stem이다.
GROUND_OVERRIDE_MASK_STEM = "T_ENV_KNVLLG_Mask_01_M"

# [추가] 바닥 오버라이드 텍스처의 기본 타일링 배율이다.
GROUND_OVERRIDE_BASE_SCALE = 8.0
# [추가] 바닥 오버라이드 블렌드 텍스처의 기본 타일링 배율이다.
GROUND_OVERRIDE_BLEND_SCALE = 8.0
# [추가] 바닥 오버라이드 마스크는 메시 UV를 그대로 쓰도록 기본 배율 1을 사용한다.
GROUND_OVERRIDE_MASK_SCALE = 1.0

# Snow-related texture names are stripped when the batch opts into snow-free output.
SNOW_NAME_TOKENS = (
    "snow",
)


# [추가] props.txt 안의 TextureStreamingData 블록에서 texture별 UV 메타를 복원하는 함수다.
def parse_texture_streaming_entries(lines: list[str]):
    entries = []
    text = "\n".join(lines)

    # [변경] 중첩 블록을 수동으로 세던 기존 방식 대신,
    # [변경] SamplingScale / UVChannelIndex / TextureName 3줄 패턴을 직접 찾아
    # [변경] 최상위 TextureStreamingData 배열 안쪽 항목들을 안정적으로 복원한다.
    pattern = re.compile(
        r"SamplingScale\s*=\s*([0-9.+-eE]+)\s+"
        r"UVChannelIndex\s*=\s*(\d+)\s+"
        r"TextureName\s*=\s*([^\s}]+)",
        re.MULTILINE,
    )

    for match in pattern.finditer(text):
        sampling_scale = 1.0
        uv_channel = 0
        texture_name = match.group(3).strip()

        try:
            sampling_scale = float(match.group(1))
        except ValueError:
            sampling_scale = 1.0

        try:
            uv_channel = int(match.group(2))
        except ValueError:
            uv_channel = 0

        if texture_name:
            entries.append({
                "texture_name": texture_name,
                "sampling_scale": sampling_scale,
                "uv_channel": uv_channel,
            })

    return entries


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
    # Remove snow-related textures from generated material instances.
    parser.add_argument("--strip-snow", action="store_true")
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
    # [추가] props.txt 원문 줄 목록이다. TextureStreamingData 복원과 기존 파라미터 파싱에 함께 사용한다.
    raw_lines = path.read_text(encoding="utf-8-sig", errors="ignore").splitlines()
    # [추가] texture별 UV 채널/샘플링 스케일 메타를 별도로 수집한다.
    texture_streaming_entries = parse_texture_streaming_entries(raw_lines)

    current_section = None
    current_entry = None

    for raw_line in raw_lines:
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
            "TextureStreamingData": texture_streaming_entries,
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

    # [추가] FModel json의 TextureStreamingData를 props.txt와 같은 공통 포맷으로 정규화한다.
    texture_streaming_entries = []
    for entry in props.get("TextureStreamingData", []):
        if not isinstance(entry, dict):
            continue

        texture_name = str(entry.get("TextureName", "")).strip()
        if not texture_name:
            continue

        texture_streaming_entries.append({
            "texture_name": texture_name,
            "sampling_scale": float(entry.get("SamplingScale", 1.0)),
            "uv_channel": int(entry.get("UVChannelIndex", 0)),
        })

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
            "TextureStreamingData": texture_streaming_entries,
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


# [추가] texture별 UV 채널/샘플링 스케일 메타에 접근하는 helper다.
def get_texture_streaming_entries(mi_data: dict):
    return get_parameters(mi_data).get("TextureStreamingData", [])


# [추가] texture name 기준으로 UV 메타 lookup을 만드는 함수다.
def build_texture_streaming_lookup(mi_data: dict):
    lookup = {}

    for entry in get_texture_streaming_entries(mi_data):
        if not isinstance(entry, dict):
            continue

        texture_name = str(entry.get("texture_name", "")).strip()
        if not texture_name:
            continue

        key = texture_name.lower()
        if key in lookup:
            continue

        lookup[key] = {
            "uv_channel": int(entry.get("uv_channel", 0)),
            "sampling_scale": float(entry.get("sampling_scale", 1.0)),
        }

    return lookup


# [추가] unreal texture ref가 어떤 UV 채널/샘플링 스케일을 써야 하는지 복원한다.
def resolve_texture_sampling_meta(unreal_ref: str, texture_streaming_lookup: dict):
    texture_name = extract_texture_asset_name(unreal_ref)
    if not texture_name:
        return 0, 1.0

    entry = texture_streaming_lookup.get(texture_name.lower())
    if not entry:
        return 0, 1.0

    return int(entry.get("uv_channel", 0)), float(entry.get("sampling_scale", 1.0))


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


def normalize_opaque_color_alpha(color: list[float], blend_mode: int) -> list[float]:
    # [추가] Unreal 머티리얼의 BaseMixColor/ShadowColor 는 opaque 머티리얼이어도 A=0 으로 들어오는 경우가 있다.
    # [추가] 우리 런타임은 base_color_factor.a 를 그대로 쓰므로, opaque(BlendMode 0)는 알파를 1로 강제해
    # [추가] 합성 단계에서 불필요하게 discard 되거나 반투명처럼 보이는 문제를 막는다.
    normalized = list(color)

    if blend_mode == 0 and len(normalized) >= 4:
        normalized[3] = 1.0

    return normalized


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


def append_texture_slot(out_data: dict, slot_name: str, texture_path: str, texture_index: int = 0,
                        uv_channel: int = 0, sampling_scale: float = 1.0):
    # [추가] 공통 texture append helper.
    # [추가] 비어 있는 경로는 건너뛰고, 생성 포맷을 한 곳에서 맞춘다.
    if not texture_path:
        return

    out_data["textures"].append({
        "slot": slot_name,
        "index": texture_index,
        "path": texture_path,
        "uv_channel": uv_channel,
        "sampling_scale": sampling_scale,
    })


# [추가] 현재 머티리얼이 KonohaVillage02 바닥 강제 마스킹 대상인지 판정하는 helper다.
# [추가] 바닥 청크에 실제로 쓰이는 머티리얼 이름만 좁게 잡아서 다른 맵 재질까지 건드리지 않도록 한다.
# Decide whether a texture path or material name should be treated as snow content.
def is_snow_related_name(name: str) -> bool:
    lowered_name = name.lower().strip()
    return any(token in lowered_name for token in SNOW_NAME_TOKENS)


# Strip snow slots from a generated material instance so the converted map stays snow-free.
def strip_snow_textures_from_material(material_name: str, out_data: dict):
    textures = out_data.get("textures", [])
    if not isinstance(textures, list) or not textures:
        return

    filtered_textures = []
    removed_slots = set()

    for texture in textures:
        texture_path = str(texture.get("path", ""))
        slot_name = str(texture.get("slot", ""))

        if is_snow_related_name(texture_path):
            removed_slots.add(slot_name)
            continue

        filtered_textures.append(texture)

    if not removed_slots:
        return

    out_data["textures"] = filtered_textures

    # Reset blend-oriented controls once their snow inputs have been removed.
    if "blend_base_color" in removed_slots:
        out_data["mask_scale"] = 1.0
        out_data["mask_threshold"] = 1.0

    if "blend_normal" in removed_slots:
        out_data["blend_normal_strength"] = 0.0

    # Dedicated snow materials should not keep a snowy tint after their snow textures are stripped.
    if is_snow_related_name(material_name):
        out_data["base_color_factor"] = [1.0, 1.0, 1.0, 1.0]
        out_data["shadow_color"] = [1.0, 1.0, 1.0, 1.0]


def is_ground_override_material(material_name: str) -> bool:
    upper_name = material_name.upper().strip()
    return upper_name in GROUND_OVERRIDE_MATERIAL_NAMES


# [추가] 강제 주입용 texture stem을 texture_index에서 찾는 helper다.
# [추가] FModel props를 거치지 않고도 지정된 텍스처를 바로 matinst에 심기 위해 사용한다.
def resolve_forced_texture_path(texture_stem: str, texture_index: dict, owner_json_path: Path, copy_root, dry_run: bool) -> str:
    source_path = texture_index.get(texture_stem.lower())
    return make_texture_path(source_path, owner_json_path, copy_root, dry_run)


# [추가] KonohaVillage02 바닥 머티리얼을 Sand + Grass + Mask 조합으로 강제 덮어쓰는 helper다.
# [추가] Terrain으로 갈아타지 않고 기존 StaticMesh 바닥에 마스킹을 주기 위한 1차 규칙이다.
def apply_ground_material_override(material_name: str, out_data: dict, texture_index: dict,
                                   owner_json_path: Path, copy_root, dry_run: bool):
    if not is_ground_override_material(material_name):
        return

    # [추가] 바닥 오버라이드에서는 원본 tint가 과하게 들어오지 않도록 베이스/그림자 색을 중립값으로 고정한다.
    out_data["base_color_factor"] = [1.0, 1.0, 1.0, 1.0]
    out_data["shadow_color"] = [1.0, 1.0, 1.0, 1.0]
    # [변경] 바닥 오버라이드에서는 잔디 블렌드가 더 잘 보이도록 threshold를 올린다.
    # [추가] 다른 건물 머티리얼에는 영향을 주지 않고, ground override 대상에만 적용된다.
    out_data["mask_scale"] = 1.0
    out_data["mask_threshold"] = 2.5

    # [추가] 기존 FModel 텍스처 중 base/blend/mask 슬롯은 제거하고 바닥용 강제 텍스처로 교체한다.
    out_data["textures"] = [
        texture for texture in out_data["textures"]
        if texture.get("slot") not in {"base_color", "blend_base_color", "mask"}
    ]

    # [추가] 모래/잔디/마스크 텍스처를 프로젝트용 상대경로로 복사/변환한다.
    base_color_path = resolve_forced_texture_path(
        GROUND_OVERRIDE_BASE_COLOR_STEM, texture_index, owner_json_path, copy_root, dry_run
    )
    blend_color_path = resolve_forced_texture_path(
        GROUND_OVERRIDE_BLEND_COLOR_STEM, texture_index, owner_json_path, copy_root, dry_run
    )
    mask_path = resolve_forced_texture_path(
        GROUND_OVERRIDE_MASK_STEM, texture_index, owner_json_path, copy_root, dry_run
    )

    append_texture_slot(
        out_data, "base_color", base_color_path,
        uv_channel=0, sampling_scale=GROUND_OVERRIDE_BASE_SCALE
    )
    append_texture_slot(
        out_data, "blend_base_color", blend_color_path,
        uv_channel=0, sampling_scale=GROUND_OVERRIDE_BLEND_SCALE
    )
    append_texture_slot(
        out_data, "mask", mask_path,
        uv_channel=0, sampling_scale=GROUND_OVERRIDE_MASK_SCALE
    )


def resolve_generic_pbr(material_name: str, mi_data: dict, texture_index: dict, owner_json_path: Path,
                        copy_root, dry_run: bool):
    textures = get_textures(mi_data)
    colors = get_colors(mi_data)
    scalars = get_scalars(mi_data)
    # [추가] texture별 UV 채널/샘플링 스케일 lookup이다.
    texture_streaming_lookup = build_texture_streaming_lookup(mi_data)
    params = get_parameters(mi_data)
    # [추가] 생성 전에 blend mode 를 먼저 고정해 두면 opaque 알파 정규화를 같은 기준으로 재사용할 수 있다.
    blend_mode = int(params.get("BlendMode", 0))

    base_color_ref = find_first_key(textures, [
        "AA_Override_BaseColorMap",
        "AA_BaseColorMap",
        "PM_Diffuse",
    ])
    blend_base_color_ref = find_first_key(textures, [
        "AB_BlendBaseColorMap",
    ])
    normal_ref = find_first_key(textures, [
        "BA_NomalMap",
        "BA_NormalMap",
        "PM_Normals",
    ])
    blend_normal_ref = find_first_key(textures, [
        "BB_BlendNormalMap",
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
    uneven_color_ref = find_first_key(textures, [
        "DA_UnevenColorMap",
    ])
    mask_ref = find_first_key(textures, [
        "EA_MaskMap",
    ])

    out = {
        "version": 1,
        "name": material_name,
        "profile": "generic_pbr",
        "parent": "M_GenericPBR",
        "base_color_factor": normalize_opaque_color_alpha(
            get_color4(colors, "AA_BaseMixColor", [1.0, 1.0, 1.0, 1.0]),
            blend_mode,
        ),
        "shadow_color": normalize_opaque_color_alpha(
            get_color4(colors, "AA_ShadowColor", [1.0, 1.0, 1.0, 1.0]),
            blend_mode,
        ),
        "normal_strength": float(scalars.get("AA_NormalMapBoost", 1.0)),
        "blend_normal_strength": float(scalars.get("AB_BlendNormalMapBoost", 1.0)),
        "mask_scale": float(scalars.get("AA_MaskScale", 1.0)),
        "mask_threshold": float(scalars.get("AC_Mask_Threshold", 1.0)),
        "uneven_color_scale": float(scalars.get("AA_UnevenColorScale", 1.0)),
        "blend_mode": blend_mode,
        "textures": [],
    }

    base_color_path = make_texture_path(resolve_source_texture_path(base_color_ref, texture_index), owner_json_path, copy_root, dry_run)
    blend_base_color_path = make_texture_path(resolve_source_texture_path(blend_base_color_ref, texture_index), owner_json_path, copy_root, dry_run)
    normal_path = make_texture_path(resolve_source_texture_path(normal_ref, texture_index), owner_json_path, copy_root, dry_run)
    blend_normal_path = make_texture_path(resolve_source_texture_path(blend_normal_ref, texture_index), owner_json_path, copy_root, dry_run)
    specular_path = make_texture_path(resolve_source_texture_path(specular_ref, texture_index), owner_json_path, copy_root, dry_run)
    roughness_path = make_texture_path(resolve_source_texture_path(roughness_ref, texture_index), owner_json_path, copy_root, dry_run)
    emissive_path = make_texture_path(resolve_source_texture_path(emissive_ref, texture_index), owner_json_path, copy_root, dry_run)
    uneven_color_path = make_texture_path(resolve_source_texture_path(uneven_color_ref, texture_index), owner_json_path, copy_root, dry_run)
    mask_path = make_texture_path(resolve_source_texture_path(mask_ref, texture_index), owner_json_path, copy_root, dry_run)

    # [추가] 슬롯별 UV 메타를 복원해 런타임 셰이더가 올바른 UV 채널을 선택할 수 있게 한다.
    base_color_uv_channel, base_color_sampling_scale = resolve_texture_sampling_meta(base_color_ref, texture_streaming_lookup)
    blend_base_uv_channel, blend_base_sampling_scale = resolve_texture_sampling_meta(blend_base_color_ref, texture_streaming_lookup)
    normal_uv_channel, normal_sampling_scale = resolve_texture_sampling_meta(normal_ref, texture_streaming_lookup)
    blend_normal_uv_channel, blend_normal_sampling_scale = resolve_texture_sampling_meta(blend_normal_ref, texture_streaming_lookup)
    specular_uv_channel, specular_sampling_scale = resolve_texture_sampling_meta(specular_ref, texture_streaming_lookup)
    roughness_uv_channel, roughness_sampling_scale = resolve_texture_sampling_meta(roughness_ref, texture_streaming_lookup)
    emissive_uv_channel, emissive_sampling_scale = resolve_texture_sampling_meta(emissive_ref, texture_streaming_lookup)
    uneven_uv_channel, uneven_sampling_scale = resolve_texture_sampling_meta(uneven_color_ref, texture_streaming_lookup)
    mask_uv_channel, mask_sampling_scale = resolve_texture_sampling_meta(mask_ref, texture_streaming_lookup)

    append_texture_slot(out, "base_color", base_color_path,
                        uv_channel=base_color_uv_channel, sampling_scale=base_color_sampling_scale)
    append_texture_slot(out, "blend_base_color", blend_base_color_path,
                        uv_channel=blend_base_uv_channel, sampling_scale=blend_base_sampling_scale)
    append_texture_slot(out, "normal", normal_path,
                        uv_channel=normal_uv_channel, sampling_scale=normal_sampling_scale)
    append_texture_slot(out, "blend_normal", blend_normal_path,
                        uv_channel=blend_normal_uv_channel, sampling_scale=blend_normal_sampling_scale)
    append_texture_slot(out, "specular", specular_path,
                        uv_channel=specular_uv_channel, sampling_scale=specular_sampling_scale)
    append_texture_slot(out, "roughness", roughness_path,
                        uv_channel=roughness_uv_channel, sampling_scale=roughness_sampling_scale)
    append_texture_slot(out, "emissive", emissive_path,
                        uv_channel=emissive_uv_channel, sampling_scale=emissive_sampling_scale)
    append_texture_slot(out, "uneven_color", uneven_color_path,
                        uv_channel=uneven_uv_channel, sampling_scale=uneven_sampling_scale)
    append_texture_slot(out, "mask", mask_path,
                        uv_channel=mask_uv_channel, sampling_scale=mask_sampling_scale)

    # [추가] KonohaVillage02 바닥 계열은 원본 머티리얼 대신 모래/잔디/마스크 조합으로 강제 치환한다.
    apply_ground_material_override(
        material_name, out, texture_index, owner_json_path, copy_root, dry_run
    )

    return out


def resolve_flat_color_normal(material_name: str, mi_data: dict, texture_index: dict, owner_json_path: Path,
                              copy_root, dry_run: bool):
    textures = get_textures(mi_data)
    colors = get_colors(mi_data)
    scalars = get_scalars(mi_data)
    # [추가] flat profile도 texture streaming 메타를 함께 보존한다.
    texture_streaming_lookup = build_texture_streaming_lookup(mi_data)
    params = get_parameters(mi_data)
    # [추가] flat_color_normal 도 generic_pbr 와 같은 opaque 알파 정규화 기준을 사용한다.
    blend_mode = int(params.get("BlendMode", 0))

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

    base_color = normalize_opaque_color_alpha(base_color, blend_mode)
    shadow_color = normalize_opaque_color_alpha(shadow_color, blend_mode)

    out = {
        "version": 1,
        "name": material_name,
        "profile": "flat_color_normal",
        "parent": "M_ENV_FlatColorNormal",
        "base_color_factor": base_color,
        "shadow_color": shadow_color,
        "normal_strength": float(scalars.get("AA_NormalMapBoost", 1.0)),
        "blend_mode": blend_mode,
        "textures": [],
    }

    normal_path = make_texture_path(resolve_source_texture_path(normal_ref, texture_index), owner_json_path, copy_root, dry_run)
    specular_path = make_texture_path(resolve_source_texture_path(specular_ref, texture_index), owner_json_path, copy_root, dry_run)
    normal_uv_channel, normal_sampling_scale = resolve_texture_sampling_meta(normal_ref, texture_streaming_lookup)
    specular_uv_channel, specular_sampling_scale = resolve_texture_sampling_meta(specular_ref, texture_streaming_lookup)

    if normal_path:
        out["textures"].append({
            "slot": "normal",
            "index": 0,
            "path": normal_path,
            "uv_channel": normal_uv_channel,
            "sampling_scale": normal_sampling_scale,
        })
    if specular_path:
        out["textures"].append({
            "slot": "specular",
            "index": 0,
            "path": specular_path,
            "uv_channel": specular_uv_channel,
            "sampling_scale": specular_sampling_scale,
        })

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
                                    profile_config: dict, matinst_root: Path, copy_root, dry_run: bool,
                                    strip_snow: bool):
    matinst_path = matinst_root / f"{material_name}.matinst.json"
    profile = detect_profile(material_name, profile_config)

    if profile == "flat_color_normal":
        payload = resolve_flat_color_normal(material_name, mi_data, texture_index, matinst_path, copy_root, dry_run)
    else:
        payload = resolve_generic_pbr(material_name, mi_data, texture_index, matinst_path, copy_root, dry_run)

    if strip_snow:
        strip_snow_textures_from_material(material_name, payload)

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
                            matinst_root: Path, copy_root, dry_run: bool, strip_snow: bool):
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
                material_name, mi_path, mi_data, texture_index, profile_config, matinst_root, copy_root, dry_run,
                strip_snow
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


def infer_unresolved_material_base_color(material_name: str, mat_path: Path):
    lower_name = material_name.lower()
    lower_path = mat_path.as_posix().lower()

    if "window" in lower_name:
        return [0.72, 0.77, 0.83, 1.0]
    if "paper" in lower_name:
        return [0.82, 0.79, 0.72, 1.0]
    if "plasterdarkgrey" in lower_name:
        return [0.46, 0.46, 0.48, 1.0]
    if "plasterdark" in lower_name:
        return [0.55, 0.53, 0.50, 1.0]
    if "plaster" in lower_name:
        return [0.69, 0.65, 0.60, 1.0]
    if "concreteslabs_bright" in lower_name or "concreteslabs_light" in lower_name or "concreteslabs" in lower_name:
        return [0.62, 0.61, 0.58, 1.0]
    if "woodplain_black" in lower_name:
        return [0.18, 0.16, 0.15, 1.0]
    if "woodplain_red" in lower_name:
        return [0.41, 0.18, 0.15, 1.0]
    if "woodplain_yellow" in lower_name or "woodboard_natural_yellowed" in lower_name:
        return [0.56, 0.43, 0.24, 1.0]
    if "woodplain_brown" in lower_name or "woodboard_natural" in lower_name:
        return [0.44, 0.32, 0.21, 1.0]
    if "wrapping" in lower_name:
        return [0.60, 0.53, 0.40, 1.0]
    if "electric" in lower_name or "duct" in lower_name:
        return [0.46, 0.47, 0.50, 1.0]

    if lower_name.startswith("material_"):
        if "distance" in lower_path or "tower" in lower_path or "house" in lower_path or "lobby" in lower_path or "farbuilding" in lower_path:
            return [0.58, 0.59, 0.62, 1.0]
        if "building" in lower_path:
            return [0.66, 0.64, 0.60, 1.0]

    return None


def rewrite_unresolved_mesh_material(entry: dict, mat_path: Path) -> dict:
    rewritten = dict(entry)

    if "base_color_factor" in rewritten:
        return rewritten

    textures = rewritten.get("textures", [])
    if isinstance(textures, list) and len(textures) > 0:
        return rewritten

    material_name = rewritten.get("material_name", "").strip()
    if not material_name:
        return rewritten

    base_color = infer_unresolved_material_base_color(material_name, mat_path)
    if base_color is not None:
        rewritten["base_color_factor"] = base_color

    return rewritten


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
                new_materials.append(rewrite_unresolved_mesh_material(entry, mat_path))
                continue

            guid = get_or_create_material_instance_guid(matinst_path, dry_run)
            if not guid:
                missing_meta += 1
                new_materials.append(rewrite_unresolved_mesh_material(entry, mat_path))
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


def bind_mesh_materials_safe(mesh_root: Path, matinst_root: Path, dry_run: bool):
    # Safer variant for large map batches: one locked file should not abort every other material bind.
    matinst_index = build_matinst_index(matinst_root)

    processed = 0
    unresolved = 0
    missing_meta = 0
    save_failures = 0

    for mat_path in mesh_root.rglob("*.material.json"):
        try:
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
                    new_materials.append(rewrite_unresolved_mesh_material(entry, mat_path))
                    continue

                guid = get_or_create_material_instance_guid(matinst_path, dry_run)
                if not guid:
                    missing_meta += 1
                    new_materials.append(rewrite_unresolved_mesh_material(entry, mat_path))
                    continue

                new_materials.append(rewrite_mesh_material_to_guid(entry, guid))

            root["version"] = 4
            root["materials"] = new_materials

            if not dry_run:
                save_json(mat_path, root)

            processed += 1
            print(f"[BIND] {mat_path.name}")
        except Exception as e:
            save_failures += 1
            print(f"[BIND-SKIP] {mat_path.name} -> {e}")

    print("")
    print("========== BIND RESULT ==========")
    print(f"Processed mesh files : {processed}")
    print(f"Missing matinst/meta : {missing_meta}")
    print(f"Still unresolved     : {unresolved}")
    print(f"Bind save failures   : {save_failures}")


def run_convert_level(dry_run: bool, map_root: Path | None, guid_map: Path | None, out_dir: Path | None,
                      skip_snow_meshes: bool):
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
    if skip_snow_meshes:
        command.append("--skip-snow-meshes")

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
            strip_snow=args.strip_snow,
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

        bind_mesh_materials_safe(
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
            strip_snow=args.strip_snow,
        )

        normalized, updated_files = normalize_generated_matinst_paths(
            matinst_root=matinst_root,
            preferred_texture_root=copy_root if copy_root is not None else Path(),
            dry_run=args.dry_run,
        )
        if updated_files:
            print(f"[Resolver] MatInst paths normalized: {normalized} entries in {updated_files} files")

        bind_mesh_materials_safe(
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
                args.strip_snow,
            )


if __name__ == "__main__":
    main()
