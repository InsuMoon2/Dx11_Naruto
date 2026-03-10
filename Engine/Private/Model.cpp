#include "pch.h"
#include "Model.h"

#include "Mesh.h"
#include "ModelMaterial.h"
#include "GameInstance.h"
#include "Model_BinaryLoader.h"

Model::Model(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Model::Model(const Model& rhs)
    : Component(rhs)
    , _modelType(rhs._modelType)
    , _preLocalTransformMatrix(rhs._preLocalTransformMatrix)
    , _numMeshes(rhs._numMeshes)
    , _meshes(rhs._meshes)
    , _numMaterials(rhs._numMaterials)
    , _materials(rhs._materials)
    , _modelGuid(rhs._modelGuid)
{
}

HRESULT Model::Initialize_Prototype(EModelType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix)
{
    _modelType = type;
    _preLocalTransformMatrix = preLocalTransformMatrix;

    fs::path path(modelFilePath);
    string ext = path.extension().string();
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext != ".meshbin")
    {
        LOG_ERROR("Model runtime only supports .meshbin: {}", modelFilePath);
        return E_FAIL;
    }

    return Initialize_FromMeshBin(modelFilePath);
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
    EMaterialTextureSlot slot, uint32 textureIndex)
{
    if (meshIndex >= _meshes.size())
        return E_FAIL;

    uint32 matIdx = _meshes[meshIndex]->Get_MaterialIndex();

    if (matIdx >= _materials.size())
        return E_FAIL;

    return _materials[matIdx]->Bind_Material(shader, constantName, slot, textureIndex);
}

HRESULT Model::Ready_Meshes_FromBinary(const string& modelFilePath)
{
    // binary loader가 읽은 raw 정점을 엔진의 VTXMESH로 변환한 뒤
    // 최종적으로는 기존과 동일한 Mesh 객체를 만든다.
    vector<FMeshBinaryData> meshList;
    uint32 materialCount = 0;

    if (!Model_BinaryLoader::Load(modelFilePath, meshList, materialCount))
        return E_FAIL;

    _meshes.clear();
    _numMeshes = static_cast<uint32>(meshList.size());
    _numMaterials = materialCount;

    for (const FMeshBinaryData& srcMesh : meshList)
    {
        vector<VTXMESH> vertices;
        vertices.reserve(srcMesh.vertices.size());

        for (const FMeshVertexRaw& raw : srcMesh.vertices)
        {
            VTXMESH vertex{};
            vertex.position = Vec3(raw.px, raw.py, raw.pz);
            vertex.normal = Vec3(raw.nx, raw.ny, raw.nz);
            vertex.tangent = Vec3(raw.tx, raw.ty, raw.tz);
            vertex.texcoord = Vec2(raw.u, raw.v);

            Vec3 pos = vertex.position;
            Vec3 nor = vertex.normal;
            Vec3 tan = vertex.tangent;

            vertex.position = Vec3::Transform(pos, _preLocalTransformMatrix);
            vertex.normal = Vec3::TransformNormal(nor, _preLocalTransformMatrix);
            vertex.tangent = Vec3::TransformNormal(tan, _preLocalTransformMatrix);

            vertices.push_back(vertex);
        }

        Shared<Mesh> mesh = Mesh::Create(_device, _context,
            srcMesh.name, srcMesh.materialIndex, vertices, srcMesh.indices);
        CHECK_NULL(mesh, E_FAIL);
        _meshes.push_back(mesh);
    }

    return S_OK;
}

HRESULT Model::Ready_Materials_FromJson(const string& materialFilePath)
{
    ifstream file(materialFilePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open material json: {}", materialFilePath);
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    if (!root.contains("materials") || !root["materials"].is_array())
    {
        LOG_ERROR("Invalid material json: {}", materialFilePath);
        return E_FAIL;
    }

    _materials.clear();

    for (const auto& item : root["materials"])
    {
        Shared<ModelMaterial> material = ModelMaterial::Create(
            _device, _context, item, materialFilePath);
        CHECK_NULL(material, E_FAIL);
        _materials.push_back(material);
    }

    _numMaterials = static_cast<uint32>(_materials.size());
    return S_OK;
}

void Model::Apply_MaterialOverrides(const json& data)
{
    if (!data.contains("materials"))
        return;

    auto& matArray = data["materials"];
    for (size_t i = 0; i < matArray.size() && i < _materials.size(); ++i)
    {
        if (_materials[i] && !matArray[i].empty())
            _materials[i]->From_Json(matArray[i]);
    }

}

string Model::Build_MaterialJsonPath(const string& modelFilePath) const
{
    fs::path path(modelFilePath);
    path.replace_extension(".material.json");

    return path.string();
}

json Model::To_Json() const
{
    json j = Component::To_Json();

    if (!_modelGuid.empty())
    {
        j["model_guid"] = _modelGuid;
    }

    j["model_type"] = (_modelType == EModelType::SkeletalMesh) ? "SkeletalMesh" : "StaticMesh";

    if (!_materials.empty())
    {
        json matArray = json::array();
        for (auto& mat : _materials)
        {
            if (mat)
            {
                if (mat)
                    matArray.push_back(mat->To_Json());
                else
                    matArray.push_back(json::object());
            }
        }
        j["materials"] = matArray;
    }

    return j;
}

void Model::From_Json(const json& data)
{
    Component::From_Json(data);

    if (!data.contains("model_guid"))
        return;

    string newGuid = data["model_guid"].get<string>();
    string newTypeStr = data.value("model_type", "SkeletalMesh");

    EModelType newType = (newTypeStr == "SkeletalMesh") ? EModelType::SkeletalMesh : EModelType::StaticMesh;

    // Guid, 타입 둘다 같으면 로딩 스킵 (이미 프로토타입에서 로딩)
    if (newGuid == _modelGuid && newType == _modelType)
    {
        Apply_MaterialOverrides(data);
        return;
    }

    _modelGuid = newGuid;
    _modelType = newType;

    wstring path = GAME->Resolve_AssetPath(_modelGuid);
    if (path.empty())
    {
        LOG_WARN("Model GUID not found: {}", _modelGuid);
        return;
    }

    // 기존 데이터 밀기
    _meshes.clear();
    _materials.clear();

    Matrix preTransform = Matrix::CreateScale(0.01f);
    if (_modelType == EModelType::SkeletalMesh)
    {
        preTransform = preTransform
            * Matrix::CreateRotationX(XMConvertToRadians(90.f))
            * Matrix::CreateRotationY(XMConvertToRadians(180.f));
    }

    Initialize_Prototype(_modelType, Utils::ToString(path), preTransform);

    // 모델 로드 후, 머티리얼 오버라이드 적용 guid로 세팅되게
    Apply_MaterialOverrides(data);
}

Shared<ModelMaterial> Model::Get_Material(uint32 index) const
{
    if (index >= _materials.size())
        return nullptr;

    return _materials[index];
}

uint32 Model::Get_MeshMaterialIndex(uint32 index) const
{
    if (index >= _meshes.size())
        return 0;

    return _meshes[index]->Get_MaterialIndex();
}

string Model::Get_MeshName(uint32 index)
{
    if (index >= _meshes.size())
        return "";

    return _meshes[index]->Get_MeshName();
}

HRESULT Model::Initialize_FromMeshBin(const string& modelFilePath)
{
    // 메시 본체는 meshbin, 머티리얼은 .material.json 에서 읽기
    CHECK_FAILED(Ready_Meshes_FromBinary(modelFilePath), E_FAIL);
    CHECK_FAILED(Ready_Materials_FromJson(Build_MaterialJsonPath(modelFilePath)), E_FAIL);

    return S_OK;
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
}
