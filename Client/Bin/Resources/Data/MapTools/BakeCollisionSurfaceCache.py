import argparse
import glob
import json
import struct
from pathlib import Path


MAGIC = b"CSC1"
VERSION = 1

TYPE_MAP = {
    "Walkable": 0,
    "WallRun": 1,
    "WorldBlock": 2,
}

HEADER_STRUCT = struct.Struct("<4sIII")
RECORD_STRUCT = struct.Struct("<I64s37s9f")


def fixed_bytes(text: str, size: int) -> bytes:
    raw = text.encode("utf-8")
    if len(raw) >= size:
        raw = raw[: size - 1]
    return raw + (b"\0" * (size - len(raw)))


def read_transform(game_object: dict) -> tuple[list[float], list[float], list[float]]:
    for component in game_object.get("components", []):
        if component.get("type") != "COMPONENT_TYPE_TRANSFORM":
            continue

        position = component.get("position", [0.0, 0.0, 0.0])
        rotation = component.get("rotation", [0.0, 0.0, 0.0])
        scale = component.get("scale", [1.0, 1.0, 1.0])
        return position, rotation, scale

    return [0.0, 0.0, 0.0], [0.0, 0.0, 0.0], [1.0, 1.0, 1.0]


def build_record(game_object: dict) -> bytes | None:
    proxy_type_name = game_object.get("proxy_type", "")
    proxy_type = TYPE_MAP.get(proxy_type_name)
    if proxy_type is None:
        return None

    if not game_object.get("enabled", True):
        return None

    static_class = game_object.get("static_class", "")
    model_guid = game_object.get("model_guid", "")
    position, rotation, scale = read_transform(game_object)

    return RECORD_STRUCT.pack(
        proxy_type,
        fixed_bytes(static_class, 64),
        fixed_bytes(model_guid, 37),
        float(position[0]), float(position[1]), float(position[2]),
        float(rotation[0]), float(rotation[1]), float(rotation[2]),
        float(scale[0]), float(scale[1]), float(scale[2]),
    )


def collect_records(input_dir: Path, pattern: str) -> list[bytes]:
    records: list[bytes] = []

    file_pattern = str(input_dir / pattern)
    json_files = sorted(glob.glob(file_pattern))

    if not json_files:
        raise FileNotFoundError(f"No files matched pattern: {file_pattern}")

    for file_path in json_files:
        with open(file_path, "r", encoding="utf-8") as file:
            root = json.load(file)

        for game_object in root.get("gameObjects", []):
            record = build_record(game_object)
            if record is None:
                continue
            records.append(record)

    return records


def write_cache(output_path: Path, records: list[bytes]) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)

    header = HEADER_STRUCT.pack(
        MAGIC,
        VERSION,
        len(records),
        0,
    )

    with open(output_path, "wb") as file:
        file.write(header)
        for record in records:
            file.write(record)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--input-dir", required=True)
    parser.add_argument("--pattern", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()

    input_dir = Path(args.input_dir)
    output_path = Path(args.output)

    records = collect_records(input_dir, args.pattern)
    write_cache(output_path, records)

    print(f"[SurfaceCache] records={len(records)}")
    print(f"[SurfaceCache] saved={output_path}")


if __name__ == "__main__":
    main()
