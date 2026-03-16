#pragma once

NS_BEGIN(Engine)

struct FStaticMeshFileHeader
{
    uint32 magic = 0;
    uint32 version = 0;
    uint32 meshCount = 0;
    uint32 materialCount = 0;
    uint32 reserved = 0;
};

struct FSkeletalMeshFileHeader
{
    uint32 magic = 0;
    uint32 version = 0;
    uint32 meshCount = 0;
    uint32 materialCount = 0;

    uint32 modelType = 0;
    uint32 flags = 0;
    uint32 boneCount = 0;
    uint32 animationCount = 0;
};

struct FMatrixBin
{
    float m[16] = {};
};

struct FMeshVertexRaw
{
    float px = 0.f, py = 0.f, pz = 0.f;
    float nx = 0.f, ny = 0.f, nz = 0.f;
    float tx = 0.f, ty = 0.f, tz = 0.f;
    float u = 0.f, v = 0.f;
};

struct FMeshVertexAnimRaw
{
    float px = 0.f, py = 0.f, pz = 0.f;
    float nx = 0.f, ny = 0.f, nz = 0.f;
    float tx = 0.f, ty = 0.f, tz = 0.f;
    float u = 0.f, v = 0.f;

    uint32  blendIndex[4] = {};
    float   blendWeight[4] = {};
};

struct FMeshBoneRefRaw
{
    string      name;
    uint32      boneIndex = 0;
    FMatrixBin  offsetMatrix;
};

struct FBoneRaw
{
    string      name;
    int32       parentIndex = -1;
    uint32      depth = 0;
    FMatrixBin  nodeTransform;
    FMatrixBin  offsetMatrix;
    bool        hasOffsetMatrix = false;
};

struct FKeyFrameRaw
{
    float time = 0.f;
    Vec3 scale = Vec3(1.f, 1.f, 1.f);
    Quat rotation = Quat::Identity;
    Vec3 translation = Vec3::Zero;
};

struct FAnimationChannelRaw
{
    string  nodeName;
    int32   boneIndex = -1;
    vector<FKeyFrameRaw> keyFrames;
};

struct FAnimationClipRaw
{
    string  name;
    float   duration = 0.f;
    float   ticksPerSecond = 25.f;
    vector<FAnimationChannelRaw> channels;
};

struct FMeshSectionBin
{
    uint32 materialIndex = 0;
    uint32 vertexType = 0;     // 0 = static, 1 = animated
    uint32 vertexCount = 0;
    uint32 indexCount = 0;
    uint32 boneRefCount = 0;
};

struct FMeshBoneRefBin
{
    uint32 boneIndex = 0;
    FMatrixBin offsetMatrix;
};

struct FBoneBin
{
    int32 parentIndex = -1;
    uint32 depth = 0;
    FMatrixBin nodeTransform;
    FMatrixBin offsetMatrix;
    uint32 hasOffsetMatrix = 0;
};

struct FKeyFrameBin
{
    float time = 0.f;
    float scale[3] = { 1.f, 1.f, 1.f };
    float rotation[4] = { 0.f, 0.f, 0.f, 1.f };
    float translation[3] = { 0.f, 0.f, 0.f };
};

struct FAnimationChannelBin
{
    int32 boneIndex = -1;
    uint32 keyFrameCount = 0;
};

struct FAnimationClipBin
{
    float duration = 0.f;
    float ticksPerSecond = 25.f;
    uint32 channelCount = 0;
};

struct FMeshBinaryData
{
    string                  name;
    uint32                  materialIndex = 0;
    bool                    isAnimated = false;

    vector<FMeshVertexRaw>  vertices;
    vector<FMeshVertexAnimRaw> animVertices;
    vector<uint32>          indices;
    vector<FMeshBoneRefRaw> boneRefs;
};

struct FModelBinaryData
{
    uint32 materialCount = 0;
    EMeshVertexType modelType = EMeshVertexType::StaticMesh;

    vector<FMeshBinaryData> meshes;
    vector<FBoneRaw> bones;
    vector<FAnimationClipRaw> animations;
};

NS_END


