#include "pch.h"
#include "Converter.h"

#include <fstream>
#include "objbase.h"
#include <nlohmann/json.hpp>

namespace fs = filesystem;
using json = nlohmann::json;

Converter::Converter()
{
    _importer = make_shared<Assimp::Importer>();

}

bool Converter::Convert(const wstring& srcPath, const wstring& dstBasePath, EConvertModelType modelType)
{
    Clear();

    _resolvedModelType = Resolve_ModelType(srcPath, modelType);
    if (_resolvedModelType == EConvertModelType::END || _resolvedModelType == EConvertModelType::Auto)
    {
        LOG_ERROR("Failed to resolve model type: {}", fs::path(srcPath).string());
        return false;
    }

    if (!Read_AssetFile(srcPath, _resolvedModelType))
        return false;

    if (_resolvedModelType == EConvertModelType::StaticMesh)
    {
        if (!Build_StaticMeshData())
            return false;
    }
    else // Skeletal Mesh
    {
        if (!Build_BoneHierarchy())
            return false;

        if (!Build_SkeletalMeshData())
            return false;

        if (!Build_AnimationData())
            return false;
    }

    if (!Build_MaterialData(srcPath))
        return false;

    const wstring meshPath = dstBasePath + L".meshbin";
    const wstring materialPath = dstBasePath + L".material.json";

    if (!Write_MeshBin(meshPath))
        return false;

    if (!Write_MaterialJson(materialPath))
        return false;

    if (!Write_ModelMeta(meshPath, _resolvedModelType))
        return false;

    return true;
}

bool Converter::Read_AssetFile(const wstring& filePath, EConvertModelType modelType)
{
    uint32 flags = aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast;

    if (modelType == EConvertModelType::StaticMesh)
        flags |= aiProcess_PreTransformVertices;

    const string path = fs::path(filePath).string();
    _scene = _importer->ReadFile(path, flags);

    if (_scene == nullptr || _scene->mRootNode == nullptr || _scene->mNumMeshes == 0)
    {
        LOG_ERROR("Failed to import: {}", path);
        return false;
    }

    return true;
}

bool Converter::Build_StaticMeshData()
{
    if (_scene == nullptr)
        return false;

    _meshes.clear();
    _meshes.reserve(_scene->mNumMeshes);

    for (uint32 meshIndex = 0; meshIndex < _scene->mNumMeshes; ++meshIndex)
    {
        const aiMesh* srcMesh = _scene->mMeshes[meshIndex];
        if (srcMesh == nullptr)
            continue;

        FExportMeshData meshData;
        meshData.name = srcMesh->mName.length > 0 ? srcMesh->mName.C_Str() : ("Mesh_" + to_string(meshIndex));
        meshData.materialIndex = srcMesh->mMaterialIndex;
        meshData.staticVertices.reserve(srcMesh->mNumVertices);
        meshData.indices.reserve(srcMesh->mNumFaces * 3);

        for (uint32 v = 0; v < srcMesh->mNumVertices; ++v)
        {
            FMeshVertexBin vertex;

            vertex.px = srcMesh->mVertices[v].x;
            vertex.py = srcMesh->mVertices[v].y;
            vertex.pz = srcMesh->mVertices[v].z;

            if (srcMesh->HasNormals())
            {
                vertex.nx = srcMesh->mNormals[v].x;
                vertex.ny = srcMesh->mNormals[v].y;
                vertex.nz = srcMesh->mNormals[v].z;
            }

            if (srcMesh->HasTangentsAndBitangents())
            {
                vertex.tx = srcMesh->mTangents[v].x;
                vertex.ty = srcMesh->mTangents[v].y;
                vertex.tz = srcMesh->mTangents[v].z;
            }

            if (srcMesh->HasTextureCoords(0))
            {
                vertex.u = srcMesh->mTextureCoords[0][v].x;
                vertex.v = srcMesh->mTextureCoords[0][v].y;
            }

            meshData.staticVertices.push_back(vertex);
        }

        for (uint32 f = 0; f < srcMesh->mNumFaces; ++f)
        {
            const aiFace& face = srcMesh->mFaces[f];
            if (face.mNumIndices != 3)
                continue;

            meshData.indices.push_back(face.mIndices[0]);
            meshData.indices.push_back(face.mIndices[1]);
            meshData.indices.push_back(face.mIndices[2]);
        }

        _meshes.push_back(meshData);
    }

    return !_meshes.empty();
}

bool Converter::Build_BoneHierarchy()
{
    _bones.clear();
    _boneNameToIndex.clear();

    if (_scene == nullptr || _scene->mRootNode == nullptr)
        return false;

    Collect_Bones_DFS(_scene->mRootNode, -1, 0);

    // offsetMatrix를 mesh bone 정보에서 덮어쓰기
    for (uint32 meshIndex = 0; meshIndex < _scene->mNumMeshes; ++meshIndex)
    {
        const aiMesh* mesh = _scene->mMeshes[meshIndex];
        if (mesh == nullptr || !mesh->HasBones())
            continue;

        for (uint32 boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx)
        {
            const aiBone* srcBone = mesh->mBones[boneIdx];
            if (srcBone == nullptr)
                continue;

            auto it = _boneNameToIndex.find(srcBone->mName.C_Str());
            if (it == _boneNameToIndex.end())
                continue;

            FExportBoneData& bone = _bones[it->second];
            bone.offsetMatrix = To_MatrixBin(Convert_AssimpMatrix(srcBone->mOffsetMatrix));
            bone.hasOffsetMatrix = true;
        }
    }

    LOG_INFO("Build_BoneHierarchy done. boneCount={}", _bones.size());

    return !_bones.empty();
}

bool Converter::Build_SkeletalMeshData()
{
    _meshes.clear();
    _meshes.reserve(_scene->mNumMeshes);

    for (uint32 meshIndex = 0; meshIndex < _scene->mNumMeshes; ++meshIndex)
    {
        const aiMesh* srcMesh = _scene->mMeshes[meshIndex];
        if (srcMesh == nullptr)
            continue;

        FExportMeshData meshData{};

        meshData.name = srcMesh->mName.length > 0 ? srcMesh->mName.C_Str() :
            ("Mesh_" + to_string(meshIndex));

        meshData.materialIndex = srcMesh->mMaterialIndex;
        meshData.isAnimated = true;
        meshData.animVertices.resize(srcMesh->mNumVertices);
        meshData.indices.reserve(srcMesh->mNumFaces * 3);

        for (uint32 v = 0; v < srcMesh->mNumVertices; ++v)
        {
            FMeshVertexAnimBin& vertex = meshData.animVertices[v];

            vertex.px = srcMesh->mVertices[v].x;
            vertex.py = srcMesh->mVertices[v].y;
            vertex.pz = srcMesh->mVertices[v].z;

            if (srcMesh->HasNormals())
            {
                vertex.nx = srcMesh->mNormals[v].x;
                vertex.ny = srcMesh->mNormals[v].y;
                vertex.nz = srcMesh->mNormals[v].z;
            }

            if (srcMesh->HasTangentsAndBitangents())
            {
                vertex.tx = srcMesh->mTangents[v].x;
                vertex.ty = srcMesh->mTangents[v].y;
                vertex.tz = srcMesh->mTangents[v].z;
            }

            if (srcMesh->HasTextureCoords(0))
            {
                vertex.u = srcMesh->mTextureCoords[0][v].x;
                vertex.v = srcMesh->mTextureCoords[0][v].y;
            }
        }

        for (uint32 boneIdx = 0; boneIdx < srcMesh->mNumBones; ++boneIdx)
        {
            const aiBone* srcBone = srcMesh->mBones[boneIdx];
            if (srcBone == nullptr)
                continue;

            const string boneName = srcBone->mName.C_Str();
            const int32 boneIndex = Find_BoneIndex_ByName(boneName);
            if (boneIndex < 0)
                continue;

            FExportMeshBoneRef boneRef{};
            boneRef.name = boneName;
            boneRef.boneIndex = static_cast<uint32>(boneIndex);
            boneRef.offsetMatrix = To_MatrixBin(Convert_AssimpMatrix(srcBone->mOffsetMatrix));
            meshData.boneRefs.push_back(boneRef);

            for (uint32 weightIdx = 0; weightIdx < srcBone->mNumWeights; ++weightIdx)
            {
                const aiVertexWeight& vw = srcBone->mWeights[weightIdx];
                if (vw.mVertexId >= meshData.animVertices.size())
                    continue;

                Add_BoneInfluence(meshData.animVertices[vw.mVertexId], static_cast<uint32>(boneIndex), vw.mWeight);
            }
        }

        for (auto& vertex : meshData.animVertices)
            Normalize_BoneWeights(vertex);

        for (uint32 f = 0; f < srcMesh->mNumFaces; ++f)
        {
            const aiFace& face = srcMesh->mFaces[f];
            if (face.mNumIndices != 3)
                continue;

            meshData.indices.push_back(face.mIndices[0]);
            meshData.indices.push_back(face.mIndices[1]);
            meshData.indices.push_back(face.mIndices[2]);
        }

        _meshes.push_back(std::move(meshData));
    }

    LOG_INFO("Build_SkeletalMeshData done. meshCount={}", _meshes.size());
    return !_meshes.empty();
}

bool Converter::Build_AnimationData()
{
    _animations.clear();

    if (_scene == nullptr || !_scene->HasAnimations())
        return true;

    _animations.reserve(_scene->mNumAnimations);

    for (uint32 animIndex = 0; animIndex < _scene->mNumAnimations; ++animIndex)
    {
        const aiAnimation* srcAnim = _scene->mAnimations[animIndex];
        if (srcAnim == nullptr)
            continue;

        FExportAnimationClip clip{};
        clip.name = srcAnim->mName.length > 0 ? srcAnim->mName.C_Str() : ("Anim_" + to_string(animIndex));
        clip.duration = static_cast<float>(srcAnim->mDuration);
        clip.ticksPerSecond = (srcAnim->mTicksPerSecond > 0.0)
            ? static_cast<float>(srcAnim->mTicksPerSecond)
            : 25.f;

        clip.channels.reserve(srcAnim->mNumChannels);

        for (uint32 channelIndex = 0; channelIndex < srcAnim->mNumChannels; ++channelIndex)
        {
            const aiNodeAnim* srcChannel = srcAnim->mChannels[channelIndex];
            if (srcChannel == nullptr)
                continue;

            FExportAnimationChannel channel{};
            channel.nodeName = srcChannel->mNodeName.C_Str();
            channel.boneIndex = Find_BoneIndex_ByName(channel.nodeName);

            if (srcChannel->mNumRotationKeys == 0)
                continue;

            channel.keyFrames.reserve(srcChannel->mNumRotationKeys);

            uint32 posCursor = 0;
            uint32 scaleCursor = 0;

            for (uint32 rotIndex = 0; rotIndex < srcChannel->mNumRotationKeys; ++rotIndex)
            {
                const aiQuatKey& rotKey = srcChannel->mRotationKeys[rotIndex];
                const float currentTime = static_cast<float>(rotKey.mTime);

                while (posCursor + 1 < srcChannel->mNumPositionKeys &&
                    static_cast<float>(srcChannel->mPositionKeys[posCursor + 1].mTime) <= currentTime)
                {
                    ++posCursor;
                }

                while (scaleCursor + 1 < srcChannel->mNumScalingKeys &&
                    static_cast<float>(srcChannel->mScalingKeys[scaleCursor + 1].mTime) <= currentTime)
                {
                    ++scaleCursor;
                }

                FKeyFrameBin key{};
                key.time = currentTime;

                key.rotation[0] = rotKey.mValue.x;
                key.rotation[1] = rotKey.mValue.y;
                key.rotation[2] = rotKey.mValue.z;
                key.rotation[3] = rotKey.mValue.w;

                if (srcChannel->mNumPositionKeys > 0)
                {
                    const aiVectorKey& posKey = srcChannel->mPositionKeys[posCursor];
                    key.translation[0] = posKey.mValue.x;
                    key.translation[1] = posKey.mValue.y;
                    key.translation[2] = posKey.mValue.z;
                }

                if (srcChannel->mNumScalingKeys > 0)
                {
                    const aiVectorKey& scaleKey = srcChannel->mScalingKeys[scaleCursor];
                    key.scale[0] = scaleKey.mValue.x;
                    key.scale[1] = scaleKey.mValue.y;
                    key.scale[2] = scaleKey.mValue.z;
                }

                channel.keyFrames.push_back(key);
            }

            clip.channels.push_back(std::move(channel));
        }

        _animations.push_back(std::move(clip));
    }

    LOG_INFO("Build_AnimationData done. animationCount={}", _animations.size());
    return true;
}


bool Converter::Build_MaterialData(const wstring& srcPath)
{
    if (_scene == nullptr)
        return false;

    _materials.clear();
    _materials.reserve(_scene->mNumMaterials);

    for (uint32 materialIndex = 0; materialIndex < _scene->mNumMaterials; ++materialIndex)
    {
        const aiMaterial* srcMaterial = _scene->mMaterials[materialIndex];
        if (srcMaterial == nullptr)
            continue;

        FExportMaterialData materialData;

        aiString materialName;
        if (srcMaterial->Get(AI_MATKEY_NAME, materialName) == AI_SUCCESS)
            materialData.name = materialName.C_Str();
        else
            materialData.name = "Material_" + to_string(materialIndex);

        for (uint32 typeValue = 1; typeValue <= static_cast<uint32>(AI_TEXTURE_TYPE_MAX); ++typeValue)
        {
            aiTextureType assimpType = static_cast<aiTextureType>(typeValue);
            if (assimpType == aiTextureType_NONE)
                continue;

            const uint32 textureCount = srcMaterial->GetTextureCount(assimpType);
            if (textureCount == 0)
                continue;

            for (uint32 textureIndex = 0; textureIndex < textureCount; ++textureIndex)
            {
                string resolvedPath = Resolve_TexturePath(srcMaterial, assimpType, textureIndex, srcPath);
                if (resolvedPath.empty())
                    continue;

                string slot = Resolve_ExportTextureSlot(srcMaterial, assimpType, textureIndex);
                if (slot.empty())
                    continue;

                FExportTextureRef ref;
                ref.slot = slot;
                ref.index = textureIndex;
                ref.path = resolvedPath;

                materialData.textures.push_back(ref);
            }
        }

        _materials.push_back(materialData);
    }

    return true;
}

bool Converter::Write_MeshBin(const wstring& outputPath)
{
    BinaryWriter writer;
    if (!writer.Open(outputPath))
    {
        LOG_ERROR("Failed to open meshbin output");
        return false;
    }

    if (_resolvedModelType == EConvertModelType::StaticMesh)
    {
        FStaticMeshFileHeader header{};
        header.meshCount = static_cast<uint32>(_meshes.size());
        header.materialCount = static_cast<uint32>(_materials.size());
        writer.Write(header);

        for (const auto& meshData : _meshes)
        {
            writer.WriteString(meshData.name);
            writer.Write(meshData.materialIndex);

            const uint32 vertexCount = static_cast<uint32>(meshData.staticVertices.size());
            writer.Write(vertexCount);
            writer.WriteBytes(meshData.staticVertices.data(), sizeof(FMeshVertexBin) * vertexCount);

            const uint32 indexCount = static_cast<uint32>(meshData.indices.size());
            writer.Write(indexCount);
            writer.WriteBytes(meshData.indices.data(), sizeof(uint32) * indexCount);
        }

        return true;
    }

    FSkeletalMeshFileHeader header{};
    header.meshCount = static_cast<uint32>(_meshes.size());
    header.materialCount = static_cast<uint32>(_materials.size());
    header.flags = MESHBIN_FLAG_HAS_SKINNING;

    if (!_animations.empty())
        header.flags |= MESHBIN_FLAG_HAS_ANIMATION;

    header.boneCount = static_cast<uint32>(_bones.size());
    header.animationCount = static_cast<uint32>(_animations.size());

    writer.Write(header);

    for (const auto& meshData : _meshes)
    {
        writer.WriteString(meshData.name);

        FMeshSectionBin section{};
        section.materialIndex = meshData.materialIndex;
        section.vertexType = 1;
        section.vertexCount = static_cast<uint32>(meshData.animVertices.size());
        section.indexCount = static_cast<uint32>(meshData.indices.size());
        section.boneRefCount = static_cast<uint32>(meshData.boneRefs.size());
        writer.Write(section);

        writer.WriteBytes(meshData.animVertices.data(),
            sizeof(FMeshVertexAnimBin) * meshData.animVertices.size());

        writer.WriteBytes(meshData.indices.data(),
            sizeof(uint32) * meshData.indices.size());

        for (const auto& boneRef : meshData.boneRefs)
        {
            writer.WriteString(boneRef.name);

            FMeshBoneRefBin boneRefBin{};
            boneRefBin.boneIndex = boneRef.boneIndex;
            boneRefBin.offsetMatrix = boneRef.offsetMatrix;
            writer.Write(boneRefBin);
        }
    }

    for (const auto& bone : _bones)
    {
        writer.WriteString(bone.name);

        FBoneBin bin{};
        bin.parentIndex = bone.parentIndex;
        bin.depth = bone.depth;
        bin.nodeTransform = bone.nodeTransform;
        bin.offsetMatrix = bone.offsetMatrix;
        bin.hasOffsetMatrix = bone.hasOffsetMatrix ? 1u : 0u;
        writer.Write(bin);
    }

    // [추가] 3. Animation Clip Array
    for (const auto& clip : _animations)
    {
        writer.WriteString(clip.name);

        FAnimationClipBin clipBin{};
        clipBin.duration = clip.duration;
        clipBin.ticksPerSecond = clip.ticksPerSecond;
        clipBin.channelCount = static_cast<uint32>(clip.channels.size());
        writer.Write(clipBin);

        for (const auto& channel : clip.channels)
        {
            writer.WriteString(channel.nodeName);

            FAnimationChannelBin channelBin{};
            channelBin.boneIndex = channel.boneIndex;
            channelBin.keyFrameCount = static_cast<uint32>(channel.keyFrames.size());
            writer.Write(channelBin);

            writer.WriteBytes(channel.keyFrames.data(),
                sizeof(FKeyFrameBin) * channel.keyFrames.size());
        }
    }

    return true;
}

EConvertModelType Converter::Resolve_ModelType(const wstring& srcPath, EConvertModelType requestedType)
{
    if (requestedType != EConvertModelType::Auto)
    {
        LOG_INFO("Model type forced: {} -> {}",
            fs::path(srcPath).string(),
            ToString(requestedType));

        return requestedType;
    }

    // Auto 판별은 Skeletal 기준으로 먼저 읽어서 본/애니메이션 존재 여부 확인
    if (!Read_AssetFile(srcPath, EConvertModelType::SkeletalMesh))
        return EConvertModelType::END;

    const bool hasAnimations = (_scene != nullptr) && _scene->HasAnimations();
    const bool hasBones = Scene_HasBones();

    const EConvertModelType detected = Detect_ModelType_FromScene();

    LOG_INFO("Auto detect: {} | animations={} | bones={} | resolved={}",
        fs::path(srcPath).string(),
        hasAnimations,
        hasBones,
        ToString(detected));

    return detected;
}

EConvertModelType Converter::Detect_ModelType_FromScene() const
{
    if (_scene == nullptr)
        return EConvertModelType::END;

    if (_scene->HasAnimations())
        return EConvertModelType::SkeletalMesh;

    if (Scene_HasBones())
        return EConvertModelType::SkeletalMesh;

    return EConvertModelType::StaticMesh;
}

bool Converter::Scene_HasBones() const
{
    if (_scene == nullptr)
        return false;

    for (uint32 meshIndex = 0; meshIndex < _scene->mNumMeshes; ++meshIndex)
    {
        const aiMesh* mesh = _scene->mMeshes[meshIndex];
        if (mesh && mesh->HasBones())
            return true;
    }

    return false;
}

bool Converter::Write_ModelMeta(const wstring& meshPath, EConvertModelType resolvedType)
{
    fs::path metaPath(meshPath);
    metaPath += L".meta";

    string guid;
    if (!Try_ReadExistingGuid(metaPath.wstring(), guid))
        guid = Generate_Guid_String();

    json root;
    root["guid"] = guid;
    root["type"] = "model";
    root["modelType"] = ToString(resolvedType);

    // 애니메이션 순서를 보기 위해
    root["animationClipCount"] = static_cast<uint32>(_animations.size());
    root["animationClips"] = json::array();

    for (uint32 i = 0; i < _animations.size(); ++i)
    {
        json clip;
        clip["index"] = i;
        clip["name"] = _animations[i].name;
        root["animationClips"].push_back(clip);
    }

    ofstream file(metaPath, ios_base::out | ios_base::trunc);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to write model meta: {}", metaPath.string());
        return false;
    }

    file << root.dump(4);
    return true;
}

bool Converter::Try_ReadExistingGuid(const wstring& metaPath, string& outGuid) const
{
    if (!fs::exists(metaPath))
        return false;

    ifstream file(metaPath);
    if (!file.is_open())
        return false;

    try
    {
        json root;
        file >> root;

        if (!root.contains("guid"))
            return false;

        outGuid = root["guid"].get<string>();
        return !outGuid.empty();
    }
    catch (...)
    {
        return false;
    }
}

string Converter::Generate_Guid_String()
{
    GUID guid{};
    if (FAILED(::CoCreateGuid(&guid)))
        return "";

    wchar_t buffer[64] = {};
    const int len = ::StringFromGUID2(guid, buffer, static_cast<int>(std::size(buffer)));
    if (len <= 0)
        return "";

    wstring value(buffer);

    value.erase(remove(value.begin(), value.end(), L'{'), value.end());
    value.erase(remove(value.begin(), value.end(), L'}'), value.end());

    string result(value.begin(), value.end());
    transform(result.begin(), result.end(), result.begin(), ::tolower);

    return result;
}

bool Converter::Write_MaterialJson(const wstring& outputPath)
{
    fs::path path(outputPath);
    fs::create_directories(path.parent_path());

    ofstream file(path, ios_base::out | ios_base::trunc);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open material json output");
        return false;
    }

    file << "{\n";
    file << "  \"version\": 2,\n";
    file << "  \"materials\": [\n";

    for (size_t i = 0; i < _materials.size(); ++i)
    {
        const FExportMaterialData& material = _materials[i];

        file << "    {\n";
        file << "      \"material_name\": \"" << EscapeJson(material.name) << "\",\n";
        file << "      \"textures\": [\n";

        for (size_t texIdx = 0; texIdx < material.textures.size(); ++texIdx)
        {
            const FExportTextureRef& texture = material.textures[texIdx];

            file << "        {\n";
            file << "          \"slot\": \"" << EscapeJson(texture.slot) << "\",\n";
            file << "          \"index\": " << texture.index << ",\n";
            file << "          \"path\": \"" << EscapeJson(texture.path) << "\"\n";
            file << "        }";

            if (texIdx + 1 < material.textures.size())
                file << ",";

            file << "\n";
        }

        file << "      ]\n";
        file << "    }";

        if (i + 1 < _materials.size())
            file << ",";

        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    return true;
}

string Converter::Resolve_TexturePath(const aiMaterial* material, aiTextureType textureType,
    uint32 textureIndex, const wstring& modelFilePath)
{
    if (material == nullptr)
        return "";

    if (material->GetTextureCount(textureType) <= textureIndex)
        return "";

    aiString texturePath;
    if (material->GetTexture(textureType, textureIndex, &texturePath) != AI_SUCCESS)
        return "";

    string raw = texturePath.C_Str();

    LOG_WARN("Resolve_TexturePath raw = {}", raw);

    if (raw.empty())
        return "";

    if (raw[0] == '*' && _scene->GetEmbeddedTexture(raw.c_str()) != nullptr)
    {
        LOG_WARN("Embedded texture is not supported in this converter: {}", raw);
        return "";
    }

    fs::path texPath(raw);
    fs::path modelDir = fs::path(modelFilePath).parent_path();

    if (texPath.is_relative())
    {
        texPath = modelDir / texPath;
    }

    string result;

    try
    {
        fs::path relativePath = fs::relative(texPath, modelDir);

        if (!relativePath.empty())
            result = relativePath.string();
        else
            result = texPath.filename().string();
    }
    catch (...)
    {
        result = texPath.filename().string();
    }

    if (result.empty())
        result = texPath.filename().string();

    return Normalize_Path(result);
}

string Converter::Resolve_ExportTextureSlot(const aiMaterial* material, aiTextureType assimpType, uint32 textureIndex) const
{
    string fallbackSlot = Normalize_TextureSlot(assimpType);
    if (material == nullptr)
        return fallbackSlot;

    const uint32 semantic = static_cast<uint32>(assimpType);

    for (uint32 propIndex = 0; propIndex < material->mNumProperties; ++propIndex)
    {
        const aiMaterialProperty* prop = material->mProperties[propIndex];
        if (prop == nullptr)
            continue;

        if (prop->mSemantic != semantic)
            continue;

        if (prop->mIndex != textureIndex)
            continue;

        if (prop->mType != aiPTI_String || prop->mData == nullptr)
            continue;

        string propValue = Read_PropertyString(prop);
        if (propValue.empty())
            continue;

        return Infer_TextureSlot_FromString(propValue, fallbackSlot);
    }

    return fallbackSlot;
}

string Converter::Normalize_TextureSlot(aiTextureType assimpType)
{
    switch (assimpType)
    {
    case aiTextureType_BASE_COLOR:
    case aiTextureType_DIFFUSE:
        return "base_color";

    case aiTextureType_NORMAL_CAMERA:
    case aiTextureType_NORMALS:
        return "normal";

    case aiTextureType_SPECULAR:
        return "specular";

    case aiTextureType_EMISSION_COLOR:
    case aiTextureType_EMISSIVE:
        return "emissive";

    case aiTextureType_LIGHTMAP:
    case aiTextureType_AMBIENT_OCCLUSION:
        return "ambient_occlusion";

    case aiTextureType_METALNESS:
        return "metalness";

    case aiTextureType_DIFFUSE_ROUGHNESS:
    case aiTextureType_SHININESS:
        return "roughness";

    default:
        return "";
    }
}

string Converter::Read_PropertyString(const aiMaterialProperty* prop)
{
    if (prop == nullptr || prop->mData == nullptr || prop->mDataLength == 0)
        return "";

    const aiString* str = reinterpret_cast<const aiString*>(prop->mData);

    return str->C_Str();
}

string Converter::Infer_TextureSlot_FromString(const string& value, const string& fallbackSlot)
{
    string lower = ToLower_Copy(value);

    if (lower.find("base_color") != string::npos)
        return "base_color";

    if (lower.find("normal_camera") != string::npos)
        return "normal";

    if (lower.find("normal") != string::npos)
        return "normal";

    if (lower.find("specular") != string::npos)
        return "specular";

    if (lower.find("emission") != string::npos || lower.find("emissive") != string::npos)
        return "emissive";

    if (lower.find("ambient_occlusion") != string::npos || lower.find("occlusion") != string::npos)
        return "ambient_occlusion";

    if (lower.find("metalness") != string::npos || lower.find("metallic") != string::npos)
        return "metalness";

    if (lower.find("roughness") != string::npos)
        return "roughness";

    return fallbackSlot;
}

string Converter::Normalize_Path(const string& value) const
{
    string result = value;
    replace(result.begin(), result.end(), '\\', '/');

    return result;
}

void Converter::Clear()
{
    _meshes.clear();
    _materials.clear();

    if (_importer)
        _importer->FreeScene();

    _scene = nullptr;
}

void Converter::Collect_Bones_DFS(aiNode* node, int32 parentIndex, uint32 depth)
{
    if (node == nullptr)
        return;

    const uint32 currentIndex = static_cast<uint32>(_bones.size());

    FExportBoneData bone{};
    bone.name = node->mName.C_Str();
    bone.parentIndex = parentIndex;
    bone.depth = depth;
    bone.nodeTransform = To_MatrixBin(Convert_AssimpMatrix(node->mTransformation));

    _bones.push_back(bone);
    _boneNameToIndex[bone.name] = currentIndex;

    for (uint32 i = 0; i < node->mNumChildren; ++i)
    {
        Collect_Bones_DFS(node->mChildren[i], static_cast<int32>(currentIndex), depth + 1);
    }

}

int32 Converter::Find_BoneIndex_ByName(const string& name) const
{
    auto it = _boneNameToIndex.find(name);

    if (it == _boneNameToIndex.end())
        return -1;

    return it->second;
}

void Converter::Add_BoneInfluence(FMeshVertexAnimBin& vertex, uint32 boneIndex, float weight)
{
    if (weight <= 0.f)
        return;

    for (uint32 i = 0; i < 4; ++i)
    {
        if (vertex.blendWeight[i] == 0.f)
        {
            vertex.blendIndex[i] = boneIndex;
            vertex.blendWeight[i] = weight;

            return;
        }
    }

    uint32 minIndex = 0;
    for (uint32 i = 1; i < 4; ++i)
    {
        if (vertex.blendWeight[i] < vertex.blendWeight[minIndex])
        {
            minIndex = i;
        }
    }

    if (weight > vertex.blendWeight[minIndex])
    {
        vertex.blendIndex[minIndex] = boneIndex;
        vertex.blendWeight[minIndex] = weight;
    }

}

void Converter::Normalize_BoneWeights(FMeshVertexAnimBin& vertex)
{
    float sum = vertex.blendWeight[0] + vertex.blendWeight[1] +
        vertex.blendWeight[2] + vertex.blendWeight[3];

    if (sum <= FLT_EPSILON)
    {
        // 영향이 가는 뼈가 없으면 0번 본 100%로 세팅
        vertex.blendIndex[0] = 0;
        vertex.blendWeight[0] = 1.f;

        return;
    }

    for (uint32 i = 0; i < 4; ++i)
    {
        vertex.blendWeight[i] /= sum;
    }

}

Matrix Converter::Convert_AssimpMatrix(const aiMatrix4x4& m) const
{
    Matrix out(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4);

    return out;
}

FMatrixBin Converter::To_MatrixBin(const Matrix& mat) const
{
    FMatrixBin out{};

    memcpy(out.m, &mat, sizeof(float) * 16);

    return out;
}

string Converter::EscapeJson(const string& value)
{
    string out;
    out.reserve(value.size());

    for (char ch : value)
    {
        switch (ch)
        {
        case '\\': out += "\\\\"; break;
        case '\"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += ch; break;
        }
    }

    return out;
}

string Converter::ToLower_Copy(string value)
{
    transform(value.begin(), value.end(), value.begin(), ::tolower);
    return value;
}

unique_ptr<Converter> Converter::Create()
{
    return make_unique<Converter>();
}
