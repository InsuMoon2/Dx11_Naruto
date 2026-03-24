#!/usr/bin/env python3
import argparse
import json
import re
import sys
import uuid
from pathlib import Path


ACTOR_NAME_PATTERN = re.compile(r"\.([^\.']+)'$")
# [변경] 스크립트가 MapTools 폴더로 이동했으므로 실제 Data 루트는 부모 폴더다.
SCRIPT_DIR = Path(__file__).resolve().parent
RESOURCE_DATA_DIR = SCRIPT_DIR.parent
RESOURCE_JSON_DIR = RESOURCE_DATA_DIR / "json"
RESOURCE_LEVELS_DIR = RESOURCE_JSON_DIR / "Levels"
DEFAULT_FMODEL_PATH = Path(
    r"C:\Users\moon\Desktop\FModel\Output\Exports\NARUTO\Content\Maps\LobbyMaps\KonohaVillage_BORUTO\LM_KonohaVillage_BORUTO_Environments_BackdropBuildings.json"
)
DEFAULT_GUID_MAP_PATH = RESOURCE_JSON_DIR / "KonohaVilliage_mesh_guid_map.json"
DEFAULT_OUT_PATH = RESOURCE_LEVELS_DIR / "LM_KonohaVillage_BORUTO_Environments_BackdropBuildings.level.json"
DEFAULT_LEVEL_NAME = "LM_KonohaVillage_BORUTO_Environments_BackdropBuildings"
PREFERRED_MAP_PATTERNS = (
    "*_Environments_BackdropBuildings.json",
    "*_Environments_Props.json",
    "*_Environments_Terrain.json",
    "*_Props.json",
    "*_Terrain.json",
    "*_Floor.json",
    "*_p.json",
)


def load_json(path: Path):
    with path.open("r", encoding="utf-8-sig") as file:
        return json.load(file)


def get_prop(obj, key, default=None):
    if not isinstance(obj, dict):
        return default
    return obj.get(key, default)


def get_short_name_from_object_name(object_name: str):
    if not object_name:
        return None

    match = ACTOR_NAME_PATTERN.search(object_name)
    if match:
        return match.group(1)

    return None


def get_mesh_basename_from_object_path(object_path: str):
    if not object_path:
        return None

    normalized = object_path.replace("\\", "/")
    leaf = normalized.split("/")[-1]

    if "." in leaf:
        return leaf.split(".")[0]

    return Path(leaf).stem


# [추가] collision / 보조 메시 판별
def is_collision_mesh_name(mesh_base_name: str):
    if not mesh_base_name:
        return False

    upper_name = mesh_base_name.upper()

    # [추가] 현재 맵 export 기준으로 *_COL 메시를 렌더 제외 대상으로 본다.
    if upper_name.endswith("_COL"):
        return True

    # [추가] 혹시 중간 이름에 _COL_ 이 끼는 경우도 같이 차단
    if "_COL_" in upper_name:
        return True

    return False


def read_ue_vec3(props, name: str, default):
    value = get_prop(props, name)
    if not isinstance(value, dict):
        return [float(default[0]), float(default[1]), float(default[2])]

    if any(axis in value for axis in ("X", "Y", "Z")):
        return [
            float(value.get("X", default[0])),
            float(value.get("Y", default[1])),
            float(value.get("Z", default[2])),
        ]

    if any(axis in value for axis in ("Pitch", "Yaw", "Roll")):
        return [
            float(value.get("Pitch", default[0])),
            float(value.get("Yaw", default[1])),
            float(value.get("Roll", default[2])),
        ]

    return [float(default[0]), float(default[1]), float(default[2])]


def convert_position(vec, unit_scale=0.01, swap_yz=True):
    x = float(vec[0]) * unit_scale
    y = float(vec[1]) * unit_scale
    z = float(vec[2]) * unit_scale

    if swap_yz:
        return [x, z, y]

    return [x, y, z]


def convert_rotation(rot, negate_yaw=False):
    pitch = float(rot[0])
    yaw = float(rot[1])
    roll = float(rot[2])

    if negate_yaw:
        yaw = -yaw

    return [pitch, yaw, roll]


def convert_scale(scale, swap_yz=True):
    sx = 1.0 if float(scale[0]) == 0.0 else float(scale[0])
    sy = 1.0 if float(scale[1]) == 0.0 else float(scale[1])
    sz = 1.0 if float(scale[2]) == 0.0 else float(scale[2])

    if swap_yz:
        return [sx, sz, sy]

    return [sx, sy, sz]


def build_component_index(entries):
    component_by_actor_name = {}

    for entry in entries:
        if get_prop(entry, "Type", "") != "StaticMeshComponent":
            continue

        outer = get_prop(entry, "Outer", {})
        outer_object_name = get_prop(outer, "ObjectName", "")
        owner_actor_name = get_short_name_from_object_name(outer_object_name)

        if owner_actor_name and owner_actor_name not in component_by_actor_name:
            component_by_actor_name[owner_actor_name] = entry

    return component_by_actor_name


def convert_level(
    entries,
    guid_map,
    level_name,
    level_index,
    layer_tag,
    unit_scale=0.01,
    swap_yz=True,
    negate_yaw=False,
):
    component_by_actor_name = build_component_index(entries)
    game_objects = []
    skipped = []
    warnings = []

    for actor in entries:
        if get_prop(actor, "Type", "") != "StaticMeshActor":
            continue

        actor_name = get_prop(actor, "Name", "StaticMeshActor")
        component = component_by_actor_name.get(actor_name)

        if component is None:
            skipped.append({
                "actor": actor_name,
                "reason": "StaticMeshComponent not found",
                "mesh": "",
            })
            continue

        component_props = get_prop(component, "Properties", {})
        mesh_ref = get_prop(component_props, "StaticMesh", {})
        mesh_object_path = get_prop(mesh_ref, "ObjectPath", "")
        mesh_base_name = get_mesh_basename_from_object_path(mesh_object_path)

        if not mesh_base_name:
            skipped.append({
                "actor": actor_name,
                "reason": "StaticMesh.ObjectPath is empty",
                "mesh": "",
            })
            continue

        # [추가] 충돌/보조 메시(_COL)는 엔진 level spawn 대상에서 제외
        if is_collision_mesh_name(mesh_base_name):
            skipped.append({
                "actor": actor_name,
                "reason": "Collision mesh skipped",
                "mesh": mesh_base_name,
            })
            continue

        model_guid = guid_map.get(mesh_base_name)
        if not model_guid:
            skipped.append({
                "actor": actor_name,
                "reason": "GUID mapping missing",
                "mesh": mesh_base_name,
            })
            continue

        if "RelativeLocation" not in component_props:
            warnings.append({
                "actor": actor_name,
                "reason": "RelativeLocation missing. Using (0,0,0)",
                "mesh": mesh_base_name,
            })

        ue_location = read_ue_vec3(component_props, "RelativeLocation", [0.0, 0.0, 0.0])
        ue_rotation = read_ue_vec3(component_props, "RelativeRotation", [0.0, 0.0, 0.0])
        ue_scale = read_ue_vec3(component_props, "RelativeScale3D", [1.0, 1.0, 1.0])

        game_objects.append({
            "static_class": actor_name,
            "object_type": "OBJECT_TYPE_STATIC_MESH",
            "guid": str(uuid.uuid4()),
            "layerTag": layer_tag,
            "model_guid": model_guid,
            "components": [
                {
                    "type": "COMPONENT_TYPE_TRANSFORM",
                    "position": convert_position(ue_location, unit_scale=unit_scale, swap_yz=swap_yz),
                    "rotation": convert_rotation(ue_rotation, negate_yaw=negate_yaw),
                    "scale": convert_scale(ue_scale, swap_yz=swap_yz),
                }
            ],
        })

    level_json = {
        "levelName": level_name,
        "levelIndex": level_index,
        "gameObjects": game_objects,
    }

    return level_json, skipped, warnings


def print_entries(title, entries):
    if not entries:
        return

    print(f"\n[{title}]")
    for item in entries[:20]:
        print(f"- actor={item['actor']} | reason={item['reason']} | mesh={item['mesh']}")

    if len(entries) > 20:
        print(f"... {len(entries) - 20} more")


def collect_default_map_jsons(map_root: Path):
    selected = []
    seen = set()

    for pattern in PREFERRED_MAP_PATTERNS:
        for path in sorted(map_root.glob(pattern)):
            key = str(path).lower()
            if key in seen:
                continue
            seen.add(key)
            selected.append(path)

    return selected


def infer_layer_tag(fmodel_path: Path, default_layer_tag: str):
    name = fmodel_path.stem.lower()

    if "backdrop" in name:
        return "Layer_Backdrop"
    if "terrain" in name or "floor" in name:
        return "Layer_Terrain"
    if "props" in name:
        return "Layer_Props"

    return default_layer_tag


def build_out_path(out_dir: Path, fmodel_path: Path):
    return out_dir / f"{fmodel_path.stem}.level.json"


def has_non_collision_skips(skipped_entries):
    # [추가] _COL 제외는 의도된 동작이므로 실패 코드로 올리지 않는다.
    for item in skipped_entries:
        if item.get("reason", "") != "Collision mesh skipped":
            return True

    return False


def process_fmodel_file(fmodel_path: Path, guid_map: dict, args):
    if not fmodel_path.exists():
        print(f"ERROR: FModel JSON not found: {fmodel_path}")
        return 2

    fmodel_entries = load_json(fmodel_path)
    level_name = fmodel_path.stem if args.level_name_auto else args.level_name
    layer_tag = infer_layer_tag(fmodel_path, args.layer_tag)
    out_path = build_out_path(args.out_dir, fmodel_path) if args.out_auto else args.out

    level_json, skipped, warnings = convert_level(
        entries=fmodel_entries,
        guid_map=guid_map,
        level_name=level_name,
        level_index=args.level_index,
        layer_tag=layer_tag,
        unit_scale=args.unit_scale,
        swap_yz=args.swap_yz,
        negate_yaw=args.negate_yaw,
    )

    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8") as file:
        json.dump(level_json, file, indent=4, ensure_ascii=False)

    print(f"\n[LEVEL] {fmodel_path.name}")
    print(f"Saved : {out_path}")
    print(f"Spawn : {len(level_json['gameObjects'])}")
    print(f"Skip  : {len(skipped)}")
    print(f"Warn  : {len(warnings)}")

    print_entries("Warnings Top 20", warnings)
    print_entries("Skipped Top 20", skipped)

    # [변경] collision mesh skipped는 정상 스킵으로 보고 종료 코드를 성공으로 처리
    return 1 if has_non_collision_skips(skipped) else 0


def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert FModel StaticMeshActor JSON into engine .level.json."
    )
    parser.add_argument("--fmodel", type=Path, default=DEFAULT_FMODEL_PATH, help="Path to a single FModel JSON.")
    parser.add_argument("--map-root", type=Path, help="Folder containing multiple FModel JSON files to convert.")
    parser.add_argument("--guid-map", type=Path, default=DEFAULT_GUID_MAP_PATH, help="Path to mesh basename -> guid JSON.")
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT_PATH, help="Output .level.json path for single-file mode.")
    parser.add_argument("--out-dir", type=Path, default=RESOURCE_LEVELS_DIR, help="Output folder for map-root mode.")
    parser.add_argument("--level-name", default=DEFAULT_LEVEL_NAME, help="Output levelName.")
    parser.add_argument("--level-name-auto", action="store_true", default=True, help="Use FModel filename stem as levelName. Default: on")
    parser.add_argument("--no-level-name-auto", dest="level_name_auto", action="store_false", help="Use --level-name literally.")
    parser.add_argument("--out-auto", action="store_true", default=True, help="Use <out-dir>/<fmodel stem>.level.json. Default: on")
    parser.add_argument("--no-out-auto", dest="out_auto", action="store_false", help="Use --out literally.")
    parser.add_argument("--level-index", type=int, default=3, help="Output levelIndex. Default: 3")
    parser.add_argument("--layer-tag", default="Layer_Backdrop", help="Output layerTag. Default: Layer_Backdrop")
    parser.add_argument("--unit-scale", type=float, default=0.01, help="Position unit scale. Default: 0.01")
    parser.add_argument("--swap-yz", dest="swap_yz", action="store_true", default=True, help="Swap Y/Z axes. Default: on")
    parser.add_argument("--no-swap-yz", dest="swap_yz", action="store_false", help="Disable Y/Z axis swap.")
    parser.add_argument("--negate-yaw", action="store_true", default=False, help="Negate yaw during rotation conversion.")
    args = parser.parse_args()

    if len(sys.argv) == 1:
        print("[ConvertFModelLevel] No arguments provided. Using default paths.")
        print(f"  fmodel   : {args.fmodel}")
        print(f"  guid_map : {args.guid_map}")
        print(f"  out      : {args.out}")

    return args


def main():
    args = parse_args()

    if not args.guid_map.exists():
        print(f"ERROR: GUID map JSON not found: {args.guid_map}")
        return 2

    guid_map = load_json(args.guid_map)
    if args.map_root:
        if not args.map_root.exists():
            print(f"ERROR: map root not found: {args.map_root}")
            return 2

        fmodel_paths = collect_default_map_jsons(args.map_root)
        if not fmodel_paths:
            print(f"ERROR: no supported FModel JSON files found under: {args.map_root}")
            return 2

        print(f"[ConvertFModelLevel] map-root mode")
        print(f"  map_root  : {args.map_root}")
        print(f"  guid_map  : {args.guid_map}")
        print(f"  out_dir   : {args.out_dir}")
        print(f"  files     : {len(fmodel_paths)}")

        worst_code = 0
        for fmodel_path in fmodel_paths:
            result = process_fmodel_file(fmodel_path, guid_map, args)
            worst_code = max(worst_code, result)

        return worst_code

    return process_fmodel_file(args.fmodel, guid_map, args)


if __name__ == "__main__":
    sys.exit(main())
