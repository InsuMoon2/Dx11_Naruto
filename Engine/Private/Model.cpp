#include "pch.h"
#include "Model.h"

#include "Mesh.h"

Model::Model(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Model::Model(const Model& rhs)
    : Component(rhs)
    , _aiScene(rhs._aiScene)
    , _numMeshes(rhs._numMeshes)
    , _meshes(rhs._meshes)
{
}

HRESULT Model::Initialize_Prototype(const string& modelFilePath)
{
    uint32 flag = aiProcess_GlobalScale
        | aiProcess_PreTransformVertices
        | aiProcess_ConvertToLeftHanded
        | aiProcessPreset_TargetRealtime_Fast;

    _aiScene = _importer.ReadFile(modelFilePath, flag);
    CHECK_NULL(_aiScene, E_FAIL);

    CHECK_FAILED(Ready_Meshes(), E_FAIL);

    return S_OK;
}

HRESULT Model::Initialize(void* arg)
{
    return Component::Initialize(arg);
}

HRESULT Model::Render()
{
    for (auto& mesh : _meshes)
    {
        mesh->Bind_Resources();
        mesh->Render();
    }

    return S_OK;
}

HRESULT Model::Ready_Meshes()
{
    _numMeshes = _aiScene->mNumMeshes;

    for (size_t i = 0; i < _numMeshes; i++)
    {
        Shared<Mesh> mesh = Mesh::Create(_device, _context, _aiScene->mMeshes[i]);
        CHECK_NULL(mesh, E_FAIL);

        _meshes.push_back(mesh);
    }

    return S_OK;
}

Shared<Model> Model::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, const string& modelFilePath)
{
    auto instance = make_shared<Model>(device, context);

    if (FAILED(instance->Initialize_Prototype(modelFilePath)))
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

    _importer.FreeScene();
}
