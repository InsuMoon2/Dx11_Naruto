# pskx_to_fbx_batch.py
import sys
import argparse
from pathlib import Path

import bpy


def parse_args():
    argv = sys.argv
    if "--" in argv:
        argv = argv[argv.index("--") + 1 :]
    else:
        argv = []

    parser = argparse.ArgumentParser(description="Batch convert PSKX to FBX with Blender.")
    parser.add_argument("--src-root", required=True, help="Root folder containing .pskx files")
    parser.add_argument("--dst-root", required=True, help="Root folder to export .fbx files")
    parser.add_argument("--flat", action="store_true", help="Export all FBX files into dst root without subfolders")
    parser.add_argument("--axis-forward", default="-Z", help="FBX axis forward")
    parser.add_argument("--axis-up", default="Y", help="FBX axis up")
    parser.add_argument("--global-scale", type=float, default=1.0, help="FBX export global scale")
    parser.add_argument("--use-visible", action="store_true", help="Also require visible objects on export")
    return parser.parse_args(argv)


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)

    for datablocks in (
        bpy.data.meshes,
        bpy.data.armatures,
        bpy.data.materials,
        bpy.data.images,
        bpy.data.actions,
    ):
        for block in list(datablocks):
            if block.users == 0:
                datablocks.remove(block)

    try:
        bpy.ops.outliner.orphans_purge(do_local_ids=True, do_linked_ids=True, do_recursive=True)
    except TypeError:
        bpy.ops.outliner.orphans_purge()


def ensure_object_mode():
    obj = bpy.context.active_object
    if obj and obj.mode != "OBJECT":
        bpy.ops.object.mode_set(mode="OBJECT")


def import_pskx(filepath: Path):
    if not hasattr(bpy.ops.import_scene, "psk"):
        raise RuntimeError(
            "PSK/PSKX importer not found. Enable the io_scene_psk_psa add-on first."
        )

    result = bpy.ops.import_scene.psk(filepath=str(filepath))
    if "FINISHED" not in result:
        raise RuntimeError(f"Import failed: {filepath}")


def select_export_objects():
    bpy.ops.object.select_all(action="DESELECT")

    exportables = []
    for obj in bpy.context.scene.objects:
        if obj.type in {"MESH", "ARMATURE"}:
            obj.select_set(True)
            exportables.append(obj)

    if not exportables:
        raise RuntimeError("No MESH/ARMATURE objects found after import.")

    bpy.context.view_layer.objects.active = exportables[0]
    return exportables


def export_fbx(filepath: Path, axis_forward: str, axis_up: str, global_scale: float, use_visible: bool):
    filepath.parent.mkdir(parents=True, exist_ok=True)

    result = bpy.ops.export_scene.fbx(
        filepath=str(filepath),
        check_existing=False,
        use_selection=True,
        use_visible=use_visible,
        global_scale=global_scale,
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_NONE",
        use_space_transform=True,
        bake_space_transform=False,
        object_types={"ARMATURE", "MESH"},
        use_mesh_modifiers=True,
        use_mesh_modifiers_render=True,
        mesh_smooth_type="FACE",
        use_mesh_edges=False,
        use_tspace=True,
        use_triangles=False,
        add_leaf_bones=False,
        use_armature_deform_only=False,
        armature_nodetype="NULL",
        bake_anim=False,
        path_mode="AUTO",
        embed_textures=False,
        axis_forward=axis_forward,
        axis_up=axis_up,
        use_metadata=True,
    )

    if "FINISHED" not in result:
        raise RuntimeError(f"FBX export failed: {filepath}")


def iter_pskx_files(src_root: Path):
    for path in src_root.rglob("*.pskx"):
        if path.is_file():
            yield path


def build_output_path(src_root: Path, dst_root: Path, src_file: Path, flat: bool):
    if flat:
        return dst_root / f"{src_file.stem}.fbx"
    rel = src_file.relative_to(src_root).with_suffix(".fbx")
    return dst_root / rel


def main():
    args = parse_args()

    src_root = Path(args.src_root)
    dst_root = Path(args.dst_root)

    if not src_root.exists():
        raise FileNotFoundError(f"src root not found: {src_root}")

    files = list(iter_pskx_files(src_root))
    if not files:
        raise RuntimeError(f"No .pskx files found under: {src_root}")

    print(f"[PSKX->FBX] src_root = {src_root}")
    print(f"[PSKX->FBX] dst_root = {dst_root}")
    print(f"[PSKX->FBX] count    = {len(files)}")

    success = 0
    failed = []

    for index, src_file in enumerate(files, start=1):
        dst_file = build_output_path(src_root, dst_root, src_file, args.flat)
        print(f"[{index}/{len(files)}] {src_file.name}")

        try:
            clear_scene()
            ensure_object_mode()
            import_pskx(src_file)
            exported = select_export_objects()
            export_fbx(dst_file, args.axis_forward, args.axis_up, args.global_scale, args.use_visible)
            print(f"  -> exported: {dst_file}")
            print(f"  -> objects : {len(exported)}")
            success += 1
        except Exception as exc:
            print(f"  -> FAILED: {exc}")
            failed.append((src_file, str(exc)))

    print("")
    print("========== RESULT ==========")
    print(f"Success: {success}")
    print(f"Fail   : {len(failed)}")

    if failed:
        print("")
        print("========== FAIL LIST ==========")
        for src_file, reason in failed:
            print(f"{src_file} :: {reason}")


if __name__ == "__main__":
    main()
