#!/usr/bin/env python3
import argparse
import json
import subprocess
import sys
from pathlib import Path


# Base directory for relative MapTools paths.
SCRIPT_DIR = Path(__file__).resolve().parent


def load_json(path: Path):
    with path.open("r", encoding="utf-8-sig") as file:
        return json.load(file)


def save_json(path: Path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8") as file:
        json.dump(data, file, indent=4, ensure_ascii=False)


def normalize_path(path: Path) -> Path:
    # Resolve relative inputs against the MapTools folder.
    if path.is_absolute():
        return path
    return (SCRIPT_DIR / path).resolve()


def validate_non_empty_path(path: Path, option_name: str):
    # Empty quoted batch variables can collapse to '.' after Path parsing.
    if str(path) == ".":
        raise ValueError(
            f"{option_name} resolved to current directory '.'. "
            "Check the batch file variable expansion."
        )


def validate_unexpected_maptools_path(path: Path, option_name: str):
    # Reject inputs that accidentally collapse to the MapTools directory.
    if path == SCRIPT_DIR:
        raise ValueError(
            f"{option_name} resolved to MapTools directory '{SCRIPT_DIR}'. "
            "Check for an empty batch variable or broken quoted path."
        )


def run_command(command: list[str], title: str):
    print("")
    print(f"[{title}]")
    print(" ".join(f"\"{item}\"" if " " in item else item for item in command))

    result = subprocess.run(command, check=False)
    if result.returncode != 0:
        raise RuntimeError(f"{title} failed with exit code {result.returncode}")


def build_mesh_guid_map(mesh_root: Path, out_path: Path):
    # Read generated *.meshbin.meta files and emit basename -> guid mapping.
    guid_map = {}

    for meta_path in sorted(mesh_root.rglob("*.meshbin.meta")):
        root = load_json(meta_path)
        guid = root.get("guid", "")
        if not guid:
            continue

        mesh_name = meta_path.name.replace(".meshbin.meta", "")
        guid_map[mesh_name] = guid

    save_json(out_path, guid_map)

    print("")
    print("[GUID MAP]")
    print(f"saved : {out_path}")
    print(f"count : {len(guid_map)}")


def ensure_mesh_output_dirs(mesh_src: Path, mesh_dst: Path):
    # AssimpTool does not always create nested output folders on its own.
    # Mirror the input directory tree so every meshbin/material/meta file can be opened.
    mesh_dst.mkdir(parents=True, exist_ok=True)

    for src_dir in sorted(path for path in mesh_src.rglob("*") if path.is_dir()):
        rel_dir = src_dir.relative_to(mesh_src)
        (mesh_dst / rel_dir).mkdir(parents=True, exist_ok=True)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Build the KonohaVillage02 mesh/material/level pipeline in one shot."
    )
    parser.add_argument("--assimp-tool", type=Path, required=True)
    parser.add_argument("--mesh-src", type=Path, required=True)
    parser.add_argument("--mesh-dst", type=Path, required=True)
    parser.add_argument("--mi-root", type=Path, required=True)
    parser.add_argument("--texture-root", type=Path, required=True)
    parser.add_argument("--extra-texture-root", type=Path, action="append", default=[])
    parser.add_argument("--copy-textures-to", type=Path, required=True)
    parser.add_argument("--matinst-root", type=Path, required=True)
    parser.add_argument("--level-map-root", type=Path, required=True)
    parser.add_argument("--guid-map-out", type=Path, required=True)
    parser.add_argument("--level-out-dir", type=Path, required=True)
    # Strip snow-related texture bindings from generated material instances.
    parser.add_argument("--strip-snow", action="store_true")
    parser.add_argument("--dry-run", action="store_true")
    return parser.parse_args()


def main():
    args = parse_args()

    args.assimp_tool = normalize_path(args.assimp_tool)
    args.mesh_src = normalize_path(args.mesh_src)
    args.mesh_dst = normalize_path(args.mesh_dst)
    args.mi_root = normalize_path(args.mi_root)
    args.texture_root = normalize_path(args.texture_root)
    args.extra_texture_root = [normalize_path(path) for path in args.extra_texture_root]
    args.copy_textures_to = normalize_path(args.copy_textures_to)
    args.matinst_root = normalize_path(args.matinst_root)
    args.level_map_root = normalize_path(args.level_map_root)
    args.guid_map_out = normalize_path(args.guid_map_out)
    args.level_out_dir = normalize_path(args.level_out_dir)

    validate_non_empty_path(args.mesh_src, "--mesh-src")
    validate_non_empty_path(args.mesh_dst, "--mesh-dst")
    validate_non_empty_path(args.mi_root, "--mi-root")
    validate_non_empty_path(args.texture_root, "--texture-root")
    for extra_texture_root in args.extra_texture_root:
        validate_non_empty_path(extra_texture_root, "--extra-texture-root")
    validate_non_empty_path(args.level_map_root, "--level-map-root")
    validate_non_empty_path(args.guid_map_out, "--guid-map-out")
    validate_non_empty_path(args.level_out_dir, "--level-out-dir")

    validate_unexpected_maptools_path(args.mi_root, "--mi-root")
    validate_unexpected_maptools_path(args.texture_root, "--texture-root")
    for extra_texture_root in args.extra_texture_root:
        validate_unexpected_maptools_path(extra_texture_root, "--extra-texture-root")

    resolve_script = SCRIPT_DIR / "ResolveFModelMaterials.py"

    if not args.assimp_tool.exists():
        raise FileNotFoundError(f"AssimpTool.exe not found: {args.assimp_tool}")

    if not args.mesh_src.exists():
        raise FileNotFoundError(f"mesh source not found: {args.mesh_src}")

    if not args.mi_root.exists():
        raise FileNotFoundError(f"material root not found: {args.mi_root}")

    if not args.texture_root.exists():
        raise FileNotFoundError(f"texture root not found: {args.texture_root}")

    for extra_texture_root in args.extra_texture_root:
        if not extra_texture_root.exists():
            raise FileNotFoundError(f"extra texture root not found: {extra_texture_root}")

    if not args.level_map_root.exists():
        raise FileNotFoundError(f"level map root not found: {args.level_map_root}")

    if not resolve_script.exists():
        raise FileNotFoundError(f"ResolveFModelMaterials.py not found: {resolve_script}")

    args.copy_textures_to.mkdir(parents=True, exist_ok=True)
    args.matinst_root.mkdir(parents=True, exist_ok=True)
    args.level_out_dir.mkdir(parents=True, exist_ok=True)
    args.guid_map_out.parent.mkdir(parents=True, exist_ok=True)
    ensure_mesh_output_dirs(args.mesh_src, args.mesh_dst)

    assimp_command = [
        str(args.assimp_tool),
        str(args.mesh_src),
        str(args.mesh_dst),
        "static",
    ]

    if args.dry_run:
        print("[ASSIMP CONVERT] dry-run skipped")
        print(" ".join(f"\"{item}\"" if " " in item else item for item in assimp_command))
    else:
        run_command(assimp_command, "ASSIMP CONVERT")

    if args.dry_run:
        print("[GUID MAP] dry-run skipped")
    else:
        build_mesh_guid_map(args.mesh_dst, args.guid_map_out)

    resolve_command = [
        sys.executable,
        str(resolve_script),
        "--mode", "all",
        "--run-level-convert",
        "--mesh-root", str(args.mesh_dst),
        "--mi-root", str(args.mi_root),
        "--texture-root", str(args.texture_root),
        "--copy-textures-to", str(args.copy_textures_to),
        "--matinst-root", str(args.matinst_root),
        "--level-map-root", str(args.level_map_root),
        "--level-guid-map", str(args.guid_map_out),
        "--level-out-dir", str(args.level_out_dir),
    ]

    for extra_texture_root in args.extra_texture_root:
        resolve_command.extend(["--extra-texture-root", str(extra_texture_root)])

    if args.strip_snow:
        resolve_command.append("--strip-snow")

    if args.dry_run:
        resolve_command.append("--dry-run")
        print("[RESOLVE MATERIALS + CONVERT LEVEL] dry-run command")
        print(" ".join(f"\"{item}\"" if " " in item else item for item in resolve_command))
    else:
        run_command(resolve_command, "RESOLVE MATERIALS + CONVERT LEVEL")

    print("")
    print("[BuildKonohaVillage02Pipeline] Finished.")


if __name__ == "__main__":
    main()
