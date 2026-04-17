#include "pch.h"
#include "Model_BinaryLoader.h"
#include "Animation.h"

#include <fstream>

namespace
{
    constexpr uint32 MESHBIN_MAGIC = 0x4853454D; // 'MESH'
    // [추가] 첫 번째 정적 meshbin 포맷은 UV0만 저장한다.
    constexpr uint32 LEGACY_STATIC_MESHBIN_VERSION = 1;
    // [변경] 최신 정적 meshbin 포맷은 UV1까지 저장한다.
    constexpr uint32 STATIC_MESHBIN_VERSION     = 3;
    constexpr uint32 SKELETAL_MESHBIN_VERSION   = 2;

    // [추가] 과거 v1 정적 meshbin을 읽기 위한 legacy vertex 레이아웃이다.
    struct FLegacyMeshVertexRawV1
    {
        float px = 0.f, py = 0.f, pz = 0.f;
        float nx = 0.f, ny = 0.f, nz = 0.f;
        float tx = 0.f, ty = 0.f, tz = 0.f;
        float u = 0.f, v = 0.f;
    };
}

bool Model_BinaryLoader::Read_Bytes(ifstream& file, void* dst, size_t size)
{
    if (size == 0)
        return true;

    file.read(reinterpret_cast<char*>(dst), static_cast<streamsize>(size));
    return file.good();
}

bool Model_BinaryLoader::Read_String(ifstream& file, string& outValue)
{
    uint32 length = 0;
    if (!Read_Value(file, length))
        return false;

    outValue.clear();
    outValue.resize(length);

    if (length == 0)
        return true;

    return Read_Bytes(file, outValue.data(), length);
}

bool Model_BinaryLoader::Read_V1_Static(ifstream& file, const string& filePath, const FStaticMeshFileHeader& header,
    FModelBinaryData& outData)
{
    outData.materialCount = header.materialCount;
    outData.modelType = EMeshVertexType::StaticMesh;
    outData.meshes.reserve(header.meshCount);

    for (uint32 meshIndex = 0; meshIndex < header.meshCount; ++meshIndex)
    {
        FMeshBinaryData meshData{};
        meshData.isAnimated = false;

        if (!Read_String(file, meshData.name))
        {
            LOG_ERROR("Failed to read static mesh name: {} ({})", filePath, meshIndex);
            return false;
        }

        if (!Read_Value(file, meshData.materialIndex))
            return false;

        uint32 vertexCount = 0;
        if (!Read_Value(file, vertexCount))
            return false;

        vector<FLegacyMeshVertexRawV1> legacyVertices;
        legacyVertices.resize(vertexCount);

        if (!Read_Bytes(file, legacyVertices.data(), sizeof(FLegacyMeshVertexRawV1) * vertexCount))
            return false;

        meshData.vertices.resize(vertexCount);
        for (uint32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
        {
            const FLegacyMeshVertexRawV1& legacyRaw = legacyVertices[vertexIndex];
            FMeshVertexRaw& raw = meshData.vertices[vertexIndex];

            raw.px = legacyRaw.px;
            raw.py = legacyRaw.py;
            raw.pz = legacyRaw.pz;
            raw.nx = legacyRaw.nx;
            raw.ny = legacyRaw.ny;
            raw.nz = legacyRaw.nz;
            raw.tx = legacyRaw.tx;
            raw.ty = legacyRaw.ty;
            raw.tz = legacyRaw.tz;
            raw.u = legacyRaw.u;
            raw.v = legacyRaw.v;
        }

        uint32 indexCount = 0;
        if (!Read_Value(file, indexCount))
            return false;

        meshData.indices.resize(indexCount);
        if (!Read_Bytes(file, meshData.indices.data(), sizeof(uint32) * indexCount))
            return false;

        outData.meshes.push_back(std::move(meshData));
    }

    return true;
}

bool Model_BinaryLoader::Read_V3_Static(ifstream& file, const string& filePath, const FStaticMeshFileHeader& header,
    FModelBinaryData& outData)
{
    outData.materialCount = header.materialCount;
    outData.modelType = EMeshVertexType::StaticMesh;
    outData.meshes.reserve(header.meshCount);

    for (uint32 meshIndex = 0; meshIndex < header.meshCount; ++meshIndex)
    {
        FMeshBinaryData meshData{};
        meshData.isAnimated = false;

        if (!Read_String(file, meshData.name))
        {
            LOG_ERROR("Failed to read static mesh name: {} ({})", filePath, meshIndex);
            return false;
        }

        if (!Read_Value(file, meshData.materialIndex))
            return false;

        uint32 vertexCount = 0;
        if (!Read_Value(file, vertexCount))
            return false;

        meshData.vertices.resize(vertexCount);
        if (!Read_Bytes(file, meshData.vertices.data(), sizeof(FMeshVertexRaw) * vertexCount))
            return false;

        uint32 indexCount = 0;
        if (!Read_Value(file, indexCount))
            return false;

        meshData.indices.resize(indexCount);
        if (!Read_Bytes(file, meshData.indices.data(), sizeof(uint32) * indexCount))
            return false;

        outData.meshes.push_back(std::move(meshData));
    }

    return true;
}

bool Model_BinaryLoader::Read_V2_Skeletal(ifstream& file, const string& filePath, const FSkeletalMeshFileHeader& header,
    FModelBinaryData& outData)
{
    outData.materialCount = header.materialCount;
    outData.modelType = EMeshVertexType::SkeletalMesh;
    outData.meshes.reserve(header.meshCount);

    for (uint32 meshIndex = 0; meshIndex < header.meshCount; ++meshIndex)
    {
        FMeshBinaryData meshData{};
        meshData.isAnimated = true;

        if (!Read_String(file, meshData.name))
            return false;

        FMeshSectionBin section{};
        if (!Read_Value(file, section))
            return false;

        meshData.materialIndex = section.materialIndex;
        meshData.animVertices.resize(section.vertexCount);
        meshData.indices.resize(section.indexCount);
        meshData.boneRefs.resize(section.boneRefCount);

        if (!Read_Bytes(file, meshData.animVertices.data(),
                sizeof(FMeshVertexAnimRaw) * meshData.animVertices.size()))
        {
            return false;
        }

        if (!Read_Bytes(file, meshData.indices.data(),
            sizeof(uint32) * meshData.indices.size()))
        {
            return false;
        }

        for (uint32 i = 0; i < section.boneRefCount; ++i)
        {
            if (!Read_String(file, meshData.boneRefs[i].name))
                return false;

            FMeshBoneRefBin refBin{};
            if (!Read_Value(file, refBin))
                return false;

            meshData.boneRefs[i].boneIndex = refBin.boneIndex;
            meshData.boneRefs[i].offsetMatrix = refBin.offsetMatrix;
        }

        outData.meshes.push_back(std::move(meshData));
    }

    for (uint32 boneIndex = 0; boneIndex < header.boneCount; ++boneIndex)
    {
        FBoneRaw bone{};

        if (!Read_String(file, bone.name))
            return false;

        FBoneBin bin{};
        if (!Read_Value(file, bin))
            return false;

        bone.parentIndex = bin.parentIndex;
        bone.depth = bin.depth;
        bone.nodeTransform = bin.nodeTransform;
        bone.offsetMatrix = bin.offsetMatrix;
        bone.hasOffsetMatrix = (bin.hasOffsetMatrix != 0);

        outData.bones.push_back(std::move(bone));
    }

    for (uint32 animIndex = 0; animIndex < header.animationCount; ++animIndex)
    {
        FAnimationClipRaw clip{};

        if (!Read_String(file, clip.name))
            return false;

        FAnimationClipBin clipBin{};
        if (!Read_Value(file, clipBin))
            return false;

        clip.duration = clipBin.duration;
        clip.ticksPerSecond = clipBin.ticksPerSecond;
        clip.channels.reserve(clipBin.channelCount);

        for (uint32 channelIndex = 0; channelIndex < clipBin.channelCount; ++channelIndex)
        {
            FAnimationChannelRaw channel{};

            if (!Read_String(file, channel.nodeName))
                return false;

            FAnimationChannelBin channelBin{};
            if (!Read_Value(file, channelBin))
                return false;

            channel.boneIndex = channelBin.boneIndex;
            channel.keyFrames.resize(channelBin.keyFrameCount);

            for (uint32 keyIndex = 0; keyIndex < channelBin.keyFrameCount; ++keyIndex)
            {
                FKeyFrameBin keyBin{};
                if (!Read_Value(file, keyBin))
                    return false;

                FKeyFrameRaw key{};
                key.time = keyBin.time;
                key.scale = Vec3(keyBin.scale[0], keyBin.scale[1], keyBin.scale[2]);
                key.rotation = Quat(keyBin.rotation[0], keyBin.rotation[1], keyBin.rotation[2], keyBin.rotation[3]);
                key.translation = Vec3(keyBin.translation[0], keyBin.translation[1], keyBin.translation[2]);

                channel.keyFrames[keyIndex] = key;
            }

            clip.channels.push_back(std::move(channel));
        }

        outData.animations.push_back(std::move(clip));
    }

    return true;
}

bool Model_BinaryLoader::Load(const string& filePath, FModelBinaryData& outData)
{
    outData = {};

    ifstream file(filePath, ios_base::binary);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open meshbin: {}", filePath);
        return false;
    }

    uint32 magic = 0;
    uint32 version = 0;

    if (!Read_Value(file, magic) || !Read_Value(file, version))
    {
        LOG_ERROR("Failed to read meshbin magic/version: {}", filePath);
        return false;
    }

    if (magic != MESHBIN_MAGIC)
    {
        LOG_ERROR("Invalid meshbin magic: {}", filePath);
        return false;
    }

    file.seekg(0, ios_base::beg);

    if (version == LEGACY_STATIC_MESHBIN_VERSION)
    {
        FStaticMeshFileHeader header{};
        if (!Read_Value(file, header))
            return false;

        return Read_V1_Static(file, filePath, header, outData);
    }

    if (version == STATIC_MESHBIN_VERSION)
    {
        FStaticMeshFileHeader header{};
        if (!Read_Value(file, header))
            return false;

        return Read_V3_Static(file, filePath, header, outData);
    }

    if (version == SKELETAL_MESHBIN_VERSION)
    {
        FSkeletalMeshFileHeader header{};
        if (!Read_Value(file, header))
            return false;

        return Read_V2_Skeletal(file, filePath, header, outData);
    }

    LOG_ERROR("Unsupported meshbin version: {}", filePath);
    return false;
}

bool Model_BinaryLoader::Load_AnimationOnly(const string& filePath, vector<Shared<class Animation>>& outAnimations)
{
    outAnimations.clear();

    ifstream file(filePath, ios_base::binary);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open animbin: {}", filePath);
        return false;
    }

    FAnimationFileHeader header{};
    if (!Read_Value(file, header))
        return false;

    if (header.magic != ANIMBIN_MAGIC || header.version != ANIMBIN_VERSION)
    {
        LOG_ERROR("Invalid animbin magic/version: {}", filePath);
        return false;
    }

    for (uint32 animIndex = 0; animIndex < header.animationCount; ++animIndex)
    {
        FAnimationClipRaw clip{};

        if (!Read_String(file, clip.name))
            return false;

        FAnimationClipBin clipBin{};
        if (!Read_Value(file, clipBin))
            return false;

        clip.duration = clipBin.duration;
        clip.ticksPerSecond = clipBin.ticksPerSecond;
        clip.channels.reserve(clipBin.channelCount);

        for (uint32 channelIndex = 0; channelIndex < clipBin.channelCount; ++channelIndex)
        {
            FAnimationChannelRaw channel{};

            if (!Read_String(file, channel.nodeName))
                return false;

            FAnimationChannelBin channelBin{};
            if (!Read_Value(file, channelBin))
                return false;

            channel.boneIndex = channelBin.boneIndex;
            channel.keyFrames.resize(channelBin.keyFrameCount);

            for (uint32 keyIndex = 0; keyIndex < channelBin.keyFrameCount; ++keyIndex)
            {
                FKeyFrameBin keyBin{};
                if (!Read_Value(file, keyBin))
                    return false;

                FKeyFrameRaw key{};
                key.time = keyBin.time;
                key.scale = Vec3(keyBin.scale[0], keyBin.scale[1], keyBin.scale[2]);
                key.rotation = Quat(keyBin.rotation[0], keyBin.rotation[1], keyBin.rotation[2], keyBin.rotation[3]);
                key.translation = Vec3(keyBin.translation[0], keyBin.translation[1], keyBin.translation[2]);

                channel.keyFrames[keyIndex] = key;
            }

            clip.channels.push_back(std::move(channel));
        }

        Shared<Animation> animation = Animation::Create(clip);
        if (animation)
        {
            outAnimations.push_back(std::move(animation));
        }
    }

    return true;
}
