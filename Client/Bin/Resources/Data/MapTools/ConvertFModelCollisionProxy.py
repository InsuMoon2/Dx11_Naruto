#!/usr/bin/env python3
import argparse
import json
import sys
import uuid
from pathlib import Path


# MapTools 기준 기본 출력 루트다.
SCRIPT_DIR = Path(__file__).resolve().parent
RESOURCE_DATA_DIR = SCRIPT_DIR.parent
RESOURCE_JSON_DIR = RESOURCE_DATA_DIR / "json"
RESOURCE_LEVELS_DIR = RESOURCE_JSON_DIR / "Levels"


def load_json(path: Path):
    with path.open("r", encoding="utf-8-sig") as file:
        return json.load(file)


def save_json(path: Path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as file:
        json.dump(data, file, indent=4, ensure_ascii=False)


# FModel object path 마지막 leaf에서 메시 basename을 추출한다.
def get_mesh_basename_from_object_path(object_path: str):
    if not object_path:
        return None

    normalized = object_path.replace("\\", "/")
    leaf = normalized.split("/")[-1]

    if "." in leaf:
        return leaf.split(".")[0]

    return Path(leaf).stem


# UE vector/rotator dict를 공통 float[3] 배열로 바꾼다.
def read_ue_vec3(props, name: str, default):
    value = props.get(name)
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


# UE 센티미터/축을 우리 런타임 좌표로 맞춘다.
def convert_position(vec, unit_scale=0.01, swap_yz=True):
    x = float(vec[0]) * unit_scale
    y = float(vec[1]) * unit_scale
    z = float(vec[2]) * unit_scale

    if swap_yz:
        return [x, z, y]

    return [x, y, z]


# 기존 레벨 변환기와 같은 회전 규칙을 사용한다.
def convert_rotation(rot, negate_yaw=False):
    pitch = float(rot[0])
    yaw = float(rot[1])
    roll = float(rot[2])

    if negate_yaw:
        yaw = -yaw

    return [pitch, yaw, roll]


# 0 스케일은 런타임 기본값 1로 보정한다.
def convert_scale(scale, swap_yz=True):
    sx = 1.0 if float(scale[0]) == 0.0 else float(scale[0])
    sy = 1.0 if float(scale[1]) == 0.0 else float(scale[1])
    sz = 1.0 if float(scale[2]) == 0.0 else float(scale[2])

    if swap_yz:
        return [sx, sz, sy]

    return [sx, sy, sz]


# StaticMeshComponent를 actor 이름으로 빠르게 조회하기 위한 인덱스다.
def build_static_mesh_component_index(entries):
    component_by_actor_name = {}

    for entry in entries:
        if entry.get("Type", "") != "StaticMeshComponent":
            continue

        outer = entry.get("Outer", {})
        outer_name = outer.get("ObjectName", "")
        actor_name = get_actor_name_from_object_name(outer_name)
        if not actor_name:
            continue

        mesh_ref = entry.get("Properties", {}).get("StaticMesh", {})
        mesh_base_name = get_mesh_basename_from_object_path(mesh_ref.get("ObjectPath", ""))
        if not mesh_base_name:
            continue

        if actor_name not in component_by_actor_name:
            component_by_actor_name[actor_name] = entry

    return component_by_actor_name


# ObjectName 문자열에서 실제 소유 actor 이름을 안정적으로 추출한다.
# FModel export는
# StaticMeshActor'BM_xxx:PersistentLevel.ActorName'
# StaticMeshComponent'BM_xxx:PersistentLevel.ActorName.StaticMeshComponent0'
# 두 형태가 모두 나오므로 component suffix는 제거하고 actor 이름만 남긴다.
def get_actor_name_from_object_name(object_name: str):
    if not object_name:
        return None

    normalized_name = object_name.strip()

    if "'" in normalized_name:
        quote_parts = normalized_name.split("'")
        if len(quote_parts) >= 2:
            normalized_name = quote_parts[1]

    if ":" in normalized_name:
        normalized_name = normalized_name.split(":", 1)[1]

    if normalized_name.startswith("PersistentLevel."):
        normalized_name = normalized_name[len("PersistentLevel."):]

    name_parts = [part for part in normalized_name.split(".") if part]
    if not name_parts:
        return None

    last_part = name_parts[-1]
    if last_part.lower().startswith("staticmeshcomponent") and len(name_parts) >= 2:
        return name_parts[-2].strip()

    return last_part.strip()


# 구형 Konoha export에서 충돌용 메시를 COL_* 이름으로 식별한다.
def is_col_mesh_name(mesh_base_name: str):
    if not mesh_base_name:
        return False

    return mesh_base_name.upper().startswith("COL_")


# Ground 계열은 기존 Ground_Collision 로더가 담당하므로 proxy 출력에서 뺀다.
def is_ground_collision_mesh_name(mesh_base_name: str):
    if not mesh_base_name:
        return False

    upper_name = mesh_base_name.upper()
    return "GROUND" in upper_name


def is_render_fallback_mesh_name(mesh_base_name: str):
    if not mesh_base_name:
        return False

    upper_name = mesh_base_name.upper()

    if "GROUND" in upper_name:
        return False

    excluded_keywords = (
        "FLAG",
        "SIGN",
        "LANTERN",
        "BARREL",
        "BENCH",
        "BOX",
        "CRATE",
        "CARDBOARD",
        "PAPER",
        "MANHOLE",
        "VIDEO",
        "STICKER",
    )
    if any(keyword in upper_name for keyword in excluded_keywords):
        return False

    included_keywords = (
        "MERGED_",
        "WALL",
        "FENCE",
        "GATE",
        "STALL",
        "BUILDING",
        "ROCKWALL",
        "ROOF",
        "TOWER",
    )
    return any(keyword in upper_name for keyword in included_keywords)


def get_render_fallback_proxy_types(mesh_base_name: str):
    upper_name = mesh_base_name.upper()

    wall_run_keywords = (
        "MERGED_",
        "WALL",
        "BUILDING",
        "ROCKWALL",
        "ROOF",
        "TOWER",
        "GATE",
    )
    if any(keyword in upper_name for keyword in wall_run_keywords):
        return ("WallRun", "WorldBlock")

    return ("WorldBlock",)


# WallRun/WorldBlock 공용 CollisionProxyActor JSON 한 개를 만든다.
def build_proxy_game_object(actor_name: str, model_guid: str, proxy_type: str,
                            position, rotation, scale):
    # CollisionProxyActor가 model_guid만으로 런타임 모델을 복구하므로 Model component는 넣지 않는다.
    return {
        "static_class": f"{actor_name}_{proxy_type}",
        "object_type": "OBJECT_TYPE_COLLISION_PROXY",
        "guid": str(uuid.uuid4()),
        "layerTag": "Layer_CollisionProxy",
        "model_guid": model_guid,
        "proxy_type": proxy_type,
        "enabled": True,
        "components": [
            {
                "type": "COMPONENT_TYPE_TRANSFORM",
                "position": position,
                "rotation": rotation,
                "scale": scale,
            }
        ],
    }


# COL StaticMeshActor를 읽어서 proxy.level.json용 CollisionProxyActor 배열로 바꾼다.
def convert_collision_proxy_level(entries, guid_map, level_name, level_index,
                                  unit_scale=0.01, swap_yz=True, negate_yaw=False):
    component_by_actor_name = build_static_mesh_component_index(entries)
    game_objects = []
    skipped = []
    spawned_proxy_keys = set()

    for actor in entries:
        if actor.get("Type", "") != "StaticMeshActor":
            continue

        actor_name = actor.get("Name", "CollisionActor")
        component = component_by_actor_name.get(actor_name)
        if component is None:
            continue

        component_props = component.get("Properties", {})
        mesh_ref = component_props.get("StaticMesh", {})
        mesh_base_name = get_mesh_basename_from_object_path(mesh_ref.get("ObjectPath", ""))
        if not is_col_mesh_name(mesh_base_name):
            continue

        if is_ground_collision_mesh_name(mesh_base_name):
            skipped.append({
                "actor": actor_name,
                "reason": "Ground collision handled by Ground_Collision runtime path",
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

        ue_location = read_ue_vec3(component_props, "RelativeLocation", [0.0, 0.0, 0.0])
        ue_rotation = read_ue_vec3(component_props, "RelativeRotation", [0.0, 0.0, 0.0])
        ue_scale = read_ue_vec3(component_props, "RelativeScale3D", [1.0, 1.0, 1.0])

        converted_position = convert_position(ue_location, unit_scale=unit_scale, swap_yz=swap_yz)
        converted_rotation = convert_rotation(ue_rotation, negate_yaw=negate_yaw)
        converted_scale = convert_scale(ue_scale, swap_yz=swap_yz)

        for proxy_type in ("WallRun", "WorldBlock"):
            spawned_proxy_keys.add((actor_name, proxy_type))
            game_objects.append(
                build_proxy_game_object(
                    actor_name=actor_name,
                    model_guid=model_guid,
                    proxy_type=proxy_type,
                    position=converted_position,
                    rotation=converted_rotation,
                    scale=converted_scale,
                )
            )

    fallback_spawned = 0

    for actor in entries:
        if actor.get("Type", "") != "StaticMeshActor":
            continue

        actor_name = actor.get("Name", "CollisionActor")
        component = component_by_actor_name.get(actor_name)
        if component is None:
            continue

        component_props = component.get("Properties", {})
        mesh_ref = component_props.get("StaticMesh", {})
        mesh_base_name = get_mesh_basename_from_object_path(mesh_ref.get("ObjectPath", ""))

        if not is_render_fallback_mesh_name(mesh_base_name):
            continue

        model_guid = guid_map.get(mesh_base_name)
        if not model_guid:
            skipped.append({
                "actor": actor_name,
                "reason": "Fallback GUID mapping missing",
                "mesh": mesh_base_name,
            })
            continue

        ue_location = read_ue_vec3(component_props, "RelativeLocation", [0.0, 0.0, 0.0])
        ue_rotation = read_ue_vec3(component_props, "RelativeRotation", [0.0, 0.0, 0.0])
        ue_scale = read_ue_vec3(component_props, "RelativeScale3D", [1.0, 1.0, 1.0])

        converted_position = convert_position(ue_location, unit_scale=unit_scale, swap_yz=swap_yz)
        converted_rotation = convert_rotation(ue_rotation, negate_yaw=negate_yaw)
        converted_scale = convert_scale(ue_scale, swap_yz=swap_yz)

        for proxy_type in get_render_fallback_proxy_types(mesh_base_name):
            proxy_key = (actor_name, proxy_type)
            if proxy_key in spawned_proxy_keys:
                continue

            spawned_proxy_keys.add(proxy_key)
            game_objects.append(
                build_proxy_game_object(
                    actor_name=actor_name,
                    model_guid=model_guid,
                    proxy_type=proxy_type,
                    position=converted_position,
                    rotation=converted_rotation,
                    scale=converted_scale,
                )
            )
            fallback_spawned += 1

    return {
        "levelName": level_name,
        "levelIndex": level_index,
        "gameObjects": game_objects,
    }, skipped, fallback_spawned


# map-root 아래 JSON을 전부 훑어 proxy.level.json을 만든다.
def collect_map_jsons(map_root: Path):
    return sorted(path for path in map_root.glob("*.json") if path.is_file())


# level chunk 이름과 맞는 proxy.level.json 파일명을 만든다.
def build_proxy_out_path(out_dir: Path, source_path: Path):
    return out_dir / f"{source_path.stem}.proxy.level.json"


def print_entries(title, entries):
    if not entries:
        return

    print(f"\n[{title}]")
    for item in entries[:20]:
        print(f"- actor={item['actor']} | reason={item['reason']} | mesh={item['mesh']}")

    if len(entries) > 20:
        print(f"... {len(entries) - 20} more")


def process_map_file(fmodel_path: Path, guid_map: dict, args):
    entries = load_json(fmodel_path)
    proxy_json, skipped, fallback_spawned = convert_collision_proxy_level(
        entries=entries,
        guid_map=guid_map,
        level_name=fmodel_path.stem,
        level_index=args.level_index,
        unit_scale=args.unit_scale,
        swap_yz=args.swap_yz,
        negate_yaw=args.negate_yaw,
    )

    out_path = build_proxy_out_path(args.out_dir, fmodel_path)
    save_json(out_path, proxy_json)

    print(f"\n[COLLISION PROXY] {fmodel_path.name}")
    print(f"Saved : {out_path}")
    print(f"Spawn : {len(proxy_json['gameObjects'])}")
    print(f"Fallback Spawn : {fallback_spawned}")
    print(f"Skip  : {len(skipped)}")
    print_entries("Skipped Top 20", skipped)

    return 0


def parse_args():
    parser = argparse.ArgumentParser(
        description="Convert FModel COL_SM StaticMeshActor JSON into engine .proxy.level.json files."
    )
    parser.add_argument("--map-root", type=Path, required=True, help="Folder containing exported map JSON files.")
    parser.add_argument("--guid-map", type=Path, required=True, help="Path to mesh basename -> guid JSON.")
    parser.add_argument("--out-dir", type=Path, default=RESOURCE_LEVELS_DIR, help="Output folder for .proxy.level.json files.")
    parser.add_argument("--level-index", type=int, default=3, help="Output levelIndex. Default: 3")
    parser.add_argument("--unit-scale", type=float, default=0.01, help="Position unit scale. Default: 0.01")
    parser.add_argument("--swap-yz", dest="swap_yz", action="store_true", default=True, help="Swap Y/Z axes. Default: on")
    parser.add_argument("--no-swap-yz", dest="swap_yz", action="store_false", help="Disable Y/Z axis swap.")
    parser.add_argument("--negate-yaw", action="store_true", default=False, help="Negate yaw during rotation conversion.")
    return parser.parse_args()


def main():
    args = parse_args()

    if not args.map_root.exists():
        print(f"ERROR: map root not found: {args.map_root}")
        return 2

    if not args.guid_map.exists():
        print(f"ERROR: guid map not found: {args.guid_map}")
        return 2

    guid_map = load_json(args.guid_map)
    map_paths = collect_map_jsons(args.map_root)
    if not map_paths:
        print(f"ERROR: no map json files found under: {args.map_root}")
        return 2

    print("[ConvertFModelCollisionProxy] map-root mode")
    print(f"  map_root  : {args.map_root}")
    print(f"  guid_map  : {args.guid_map}")
    print(f"  out_dir   : {args.out_dir}")
    print(f"  files     : {len(map_paths)}")

    worst_code = 0
    for map_path in map_paths:
        result = process_map_file(map_path, guid_map, args)
        worst_code = max(worst_code, result)

    return worst_code


if __name__ == "__main__":
    sys.exit(main())
