#include "pch.h"
#include "Model.h"

#include "Mesh.h"
#include "ModelMaterial.h"
#include "GameInstance.h"

Model::Model(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Model::Model(const Model& rhs)
    : Component(rhs)
    , _aiScene(rhs._aiScene)
    , _numMeshes(rhs._numMeshes) , _meshes(rhs._meshes)
    , _preLocalTransformMatrix(rhs._preLocalTransformMatrix)
    , _numMaterials(rhs._numMaterials) , _materials(rhs._materials)
{
}

HRESULT Model::Initialize_Prototype(EModelType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix)
{
    _modelType = type;
    _preLocalTransformMatrix = preLocalTransformMatrix;

    uint32 flag = aiProcess_ConvertToLeftHanded| aiProcessPreset_TargetRealtime_Fast;

    if (_modelType == EModelType::StaticMesh)
        flag |= aiProcess_PreTransformVertices;

    _aiScene = _importer.ReadFile(modelFilePath, flag);
    CHECK_NULL(_aiScene, E_FAIL);

    CHECK_FAILED(Ready_Meshes(), E_FAIL);
    CHECK_FAILED(Ready_Materials(modelFilePath), E_FAIL);

    return S_OK;
}

HRESULT Model::Initialize(void* arg)
{
    return Component::Initialize(arg);
}

HRESULT Model::Render(uint32 meshIndex)
{
    if (meshIndex >= _meshes.size())
        return E_FAIL;

    _meshes[meshIndex]->Bind_Resources();
    _meshes[meshIndex]->Render();

    return S_OK;
}

HRESULT Model::Bind_Material(Shared<Shader> shader, const char* constantName, uint32 meshIndex,
    aiTextureType materialType, uint32 textureIndex)
{
    if (meshIndex >= _meshes.size())
        return E_FAIL;

    uint32 matIdx = _meshes[meshIndex]->Get_MaterialIndex();

    if (matIdx >= _materials.size())
        return E_FAIL;

    return _materials[matIdx]->Bind_Material(shader, constantName, materialType, textureIndex);
}

HRESULT Model::Ready_Meshes()
{
    _numMeshes = _aiScene->mNumMeshes;

    for (size_t i = 0; i < _numMeshes; i++)
    {
        Shared<Mesh> mesh = Mesh::Create(_device, _context,
                                            _aiScene->mMeshes[i], _preLocalTransformMatrix);

        CHECK_NULL(mesh, E_FAIL);
        _meshes.push_back(mesh);
    }

    return S_OK;
}

HRESULT Model::Ready_Materials(const string& modelFilePath)
{
    _numMaterials = _aiScene->mNumMaterials;

    for (size_t i = 0; i < _numMaterials; ++i)
    {
        Shared<ModelMaterial> material = ModelMaterial::Create(_device, _context,
                                                            _aiScene->mMaterials[i], modelFilePath);

        CHECK_NULL(material, E_FAIL);
        _materials.push_back(material);

    }

    return S_OK;
}

json Model::To_Json() const
{
    json j = Component::To_Json();

    if (!_modelGuid.empty())
    {
        j["model_guid"] = _modelGuid;
    }

    j["model_type"] = (_modelType == EModelType::SkeletalMesh) ? "SkeletalMesh" : "StaticMesh";

    return j;
}

void Model::From_Json(const json& data)
{
    Component::From_Json(data);

    if (!data.contains("model_guid"))
        return;

    string newGuid = data["model_guid"].get<string>();
    if (newGuid == _modelGuid)
        return;

    _modelGuid = newGuid;

    wstring path = GAME->Resolve_AssetPath(_modelGuid);
    if (path.empty())
    {
        LOG_WARN("Model GUID not found: {}", _modelGuid);
        return;
    }

    string modelTypeStr = data.value("model_type", "SkeletalMesh");
    EModelType modelType = (modelTypeStr == "SkeletalMesh") ? EModelType::SkeletalMesh : EModelType::StaticMesh;
    
    Matrix preTransform = Matrix::CreateScale(0.01f);
    if (modelType == EModelType::SkeletalMesh)
    {
        preTransform = preTransform
            * Matrix::CreateRotationX(XMConvertToRadians(90.f))
            * Matrix::CreateRotationY(XMConvertToRadians(180.f));
    }

    Initialize_Prototype(modelType, Utils::ToString(path), preTransform);
}

Shared<Model> Model::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EModelType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix)
{
    auto instance = make_shared<Model>(device, context);

    if (FAILED(instance->Initialize_Prototype(type, modelFilePath, preLocalTransformMatrix)))
    {
        MSG_BOX("Failed to Create : Model");
        instance->Free();

        return nullptr;
    }

    return instance;
}

Shared<Component> Model::Clone(void* arg)
{
    auto clone = make_shared<Model>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Model");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void Model::Free()
{
    Component::Free();

    _meshes.clear();
    _materials.clear();

    _importer.FreeScene();
}
