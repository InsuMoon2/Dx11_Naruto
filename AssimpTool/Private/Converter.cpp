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

    const EConvertModelType resolvedType = Resolve_ModelType(srcPath, modelType);
    if (resolvedType == EConvertModelType::END || resolvedType == EConvertModelType::Auto)
    {
        LOG_ERROR("Failed to resolve model type: {}", fs::path(srcPath).string());
        return false;
    }


    if (!Read_AssetFile(srcPath, modelType))
        return false;

    if (!Build_MeshData())
        return false;

    if (!Build_MaterialData(srcPath))
        return false;

    const wstring meshPath = dstBasePath + L".meshbin";
    const wstring materialPath = dstBasePath + L".material.json";

    if (!Write_MeshBin(meshPath))
        return false;

    if (!Write_MaterialJson(materialPath))
        return false;

    if (!Write_ModelMeta(meshPath, resolvedType))
        return false;

    LOG_INFO("Convert success: {} | requested={} | resolved={}",
        fs::path(srcPath).string(),
        ToString(modelType),
        ToString(resolvedType));

    return true;
}

bool Converter::Read_AssetFile(const wstring& filePath, EConvertModelType modelType)
{
    uint32 flags = aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_Fast;

    if (modelType == EConvertModelType::StaticMesh)
    {
        flags |= aiProcess_PreTransformVertices;
    }

    const string path = fs::path(filePath).string();
    _scene = _importer->ReadFile(path, flags);

    if (_scene == nullptr || _scene->mRootNode == nullptr || _scene->mNumMeshes == 0)
    {
        LOG_ERROR("Failed to import: {}", path);
        return false;
    }

    return true;
}

bool Converter::Build_MeshData()
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
        meshData.vertices.reserve(srcMesh->mNumVertices);
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

            meshData.vertices.push_back(vertex);
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

    FMeshFileHeader header{};
    header.meshCount = static_cast<uint32>(_meshes.size());
    header.materialCount = static_cast<uint32>(_materials.size());

    writer.Write(header);

    for (const FExportMeshData& meshData : _meshes)
    {
        writer.WriteString(meshData.name);
        writer.Write(meshData.materialIndex);

        const uint32 vertexCount = static_cast<uint32>(meshData.vertices.size());
        writer.Write(vertexCount);
        writer.WriteBytes(meshData.vertices.data(), sizeof(FMeshVertexBin) * meshData.vertices.size());

        const uint32 indexCount = static_cast<uint32>(meshData.indices.size());
        writer.Write(indexCount);
        writer.WriteBytes(meshData.indices.data(), sizeof(uint32) * meshData.indices.size());
    }

    return true;
}

EConvertModelType Converter::Resolve_ModelType(const wstring& srcPath, EConvertModelType requestedType)
{
    if (requestedType != EConvertModelType::Auto)
    {
        LOG_INFO("Model type forced: {} -> {}", fs::path(srcPath).string(), ToString(requestedType));
        return requestedType;
    }

    // Auto 판별은 PreTransformVertices 없이 먼저 읽기
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
    {
        guid = Generate_Guid_String();
        if (guid.empty())
        {
            LOG_ERROR("Failed to generate guid for meta: {}", metaPath.string());
            return false;
        }
    }

    json root;
    root["guid"] = guid;
    root["type"] = "model";
    root["modelType"] = ToString(resolvedType);

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
