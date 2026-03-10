#pragma once

NS_BEGIN(Assimp)

constexpr uint32 MESHBIN_MAGIC = 0x4853454D; // 'MESH'
constexpr uint32 MESHBIN_VERSION = 1;

enum class EConvertModelType : uint32
{
    StaticMesh = 0,
    SkeletalMesh = 1,

    END
};

struct FMeshFileHeader
{
    uint32 magic    = MESHBIN_MAGIC;
    uint32 version  = MESHBIN_VERSION;
    uint32 meshCount        = 0;
    uint32 materialCount    = 0;
    uint32 reserved         = 0;
};

struct FMeshVertexBin
{
    float px = 0.f, py = 0.f, pz = 0.f;

    float nx = 0.f ,ny = 1.f, nz = 0.f;

    float tx = 1.f, ty = 0.f, tz = 0.f;

    float u = 0.f, v = 0.f;
};

struct FExportMeshData
{
    string                  name;
    uint32                  materialIndex = 0;
    vector<FMeshVertexBin>  vertices;
    vector<uint32>          indices;
};

struct FExportTextureRef
{
    string slot;
    uint32 index = 0;
    string path;
};

struct FExportMaterialData
{
    string name;
    vector<FExportTextureRef> textures;
};

NS_END

