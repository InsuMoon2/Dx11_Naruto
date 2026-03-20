import struct
import sys
import os

def dump_bones(path):
    if not os.path.exists(path):
        print(f"File not found: {path}")
        return
        
    try:
        with open(path, 'rb') as f:
            data = f.read(32)
            if len(data) < 32:
                return
            
            unpacked = struct.unpack('<8I', data)
            bone_count = unpacked[6]
            mesh_count = unpacked[2]
            anim_count = unpacked[7]
            
            # Skip meshes
            for _ in range(mesh_count):
                name_len = struct.unpack('<I', f.read(4))[0]
                f.read(name_len)
                
                f.read(8) # vertexCount, indexCount, boneRefCount, materialIndex
                
                # We don't have the exact struct sizes here easily without full parsing.
                # Actually, parsing binary sequentially in python is tricky without knowing exact struct sizes.
                # Let's write a C++ program or just check the first few bones using a robust python script.
                pass
    except Exception as e:
        print(f"Error reading {path}: {e}")

# Instead of python, I will write a C++ dump program if it's too hard.
