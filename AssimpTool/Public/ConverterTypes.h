#pragma once

NS_BEGIN(Assimp)

constexpr uint32 MESHBIN_MAGIC = 0x4853454D; // 'MESH'
constexpr uint32 STATIC_MESHBIN_VERSION = 1;
constexpr uint32 SKELETAL_MESHBIN_VERSION = 2;

enum class EConvertModelType : uint32
{
    Auto            = 0,
    StaticMesh      = 1,
    SkeletalMesh    = 2,

    END
};

enum EMeshBinFlags : uint32
{
    MESHBIN_FLAG_HAS_SKINNING   = 1 << 0,
    MESHBIN_FLAG_HAS_ANIMATION  = 1 << 1,
};

inline const char* ToString(EConvertModelType type)
{
    switch (type)
    {
    case EConvertModelType::Auto:         return "Auto";
    case EConvertModelType::StaticMesh:   return "StaticMesh";
    case EConvertModelType::SkeletalMesh: return "SkeletalMesh";
    default:                              return "Unknown";
    }
}

struct FStaticMeshFileHeader
{
    uint32 magic            = MESHBIN_MAGIC;
    uint32 version          = STATIC_MESHBIN_VERSION;
    uint32 meshCount        = 0;
    uint32 materialCount    = 0;
    uint32 reserved         = 0;
};

struct FSkeletalMeshFileHeader
{
    uint32 magic            = MESHBIN_MAGIC;
    uint32 version          = SKELETAL_MESHBIN_VERSION;
    uint32 meshCount        = 0;
    uint32 materialCount    = 0;

    uint32 modelType        = static_cast<uint32>(EConvertModelType::SkeletalMesh);
    uint32 flags            = 0;
    uint32 boneCount        = 0;
    uint32 animationCount   = 0;
};

struct FMatrixBin
{
    float m[16] = {};
};

struct FMeshVertexBin
{
    float px = 0.f,  py = 0.f, pz = 0.f;
    float nx = 0.f,  ny = 1.f, nz = 0.f;
    float tx = 1.f,  ty = 0.f, tz = 0.f;
    float u = 0.f,   v = 0.f;
};

struct FMeshVertexAnimBin
{
    float px = 0.f, py = 0.f, pz = 0.f;
    float nx = 0.f, ny = 1.f, nz = 0.f;
    float tx = 1.f, ty = 0.f, tz = 0.f;
    float u = 0.f, v = 0.f;

    uint32  blendIndex[4]   = {};
    float   blendWeight[4]  = {};
};

struct FMeshSectionBin
{
    uint32 materialIndex    = 0;
    uint32 vertexType       = 0; // 0 = Static, 1 = Animation
    uint32 vertexCount      = 0;
    uint32 indexCount       = 0;
    uint32 boneRefCount     = 0;
};

struct FMeshBoneRefBin
{
    uint32 boneIndex = 0;
    FMatrixBin offsetMatrix;
};

struct FBoneBin
{
    int32       parentIndex     = -1;
    uint32      depth           = 0;
    FMatrixBin  nodeTransform;
    FMatrixBin  offsetMatrix;
    uint32      hasOffsetMatrix = 0;
};

struct FKeyFrameBin
{
    float time              = 0.f;
    float scale[3]          = { 1.f, 1.f, 1.f };
    float rotation[4]       = { 0.f,0.f,0.f,1.f }; // 쿼터니온
    float translation[3]    = { 0.f, 0.f, 0.f };
};

struct FAnimationChannelBin
{
    int32   boneIndex         = -1;
    uint32  keyFrameCount    = 0;
};

struct FAnimationClipBin
{
    float   duration          = 0.f;
    float   ticksPerSecond    = 25.f;
    uint32  channelCount     = 0;
};

struct FExportMeshBoneRef
{
    string name;
    uint32 boneIndex = 0;
    FMatrixBin offsetMatrix;
};

struct FExportMeshData
{
    string                  name;
    uint32                  materialIndex = 0;
    bool                    isAnimated = false;

    vector<FMeshVertexBin> staticVertices;
    vector<FMeshVertexAnimBin> animVertices;
    vector<uint32> indices;
    vector<FExportMeshBoneRef> boneRefs;
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

struct FExportBoneData
{
    string          name;
    int32           parentIndex = -1;
    uint32          depth = 0;
    FMatrixBin      nodeTransform;
    FMatrixBin      offsetMatrix;
    bool            hasOffsetMatrix = false;
};

struct FExportAnimationChannel
{
    string          nodeName;
    int32           boneIndex = -1;
    vector<FKeyFrameBin> keyFrames;
};

struct FExportAnimationClip
{
    string          name;
    float           duration = 0.f;
    float           ticksPerSecond = 25.f;
    vector<FExportAnimationChannel> channels;
};


NS_END

