#include "pch.h"
#include "Model.h"

#include "Animation.h"
#include "Bone.h"
#include "Mesh.h"
#include "ModelMaterial.h"
#include "GameInstance.h"
#include "Model_BinaryLoader.h"
#include "Shader.h"

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
    , _bones(rhs._bones)                      
    , _animations(rhs._animations)            
    , _boneMatrices(rhs._boneMatrices)        
    , _currentAnimationIndex(rhs._currentAnimationIndex)
    , _isAnimationLoop(rhs._isAnimationLoop) 
    , _modelGuid(rhs._modelGuid)
{
}

HRESULT Model::Initialize_Prototype(EMeshVertexType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix)
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

void Model::Set_Animation(uint32 animIndex, bool isLoop)
{
    if (animIndex >= _animations.size())
        return;

    //LOG_INFO("Set_Animation index = {}, animationCount = {}", animIndex, _animations.size());

    _currentAnimationIndex = static_cast<int32>(animIndex);
    _isAnimationLoop = isLoop;
    _animations[animIndex]->Reset();

    for (auto& bone : _bones)
    {
        bone->Reset_ToNodeTransform();
    }
}

void Model::Set_Animation(const string& animName, bool isLoop)
{
    const int32 index = Find_AnimationIndex_ByName(animName);

    if (index < 0)
    {
        LOG_WARN("Animation clip not found: {}", animName);
        return;
    }

    Set_Animation(static_cast<uint32>(index), isLoop);
}

int32 Model::Find_AnimationIndex_ByName(const string& animName)
{
    for (uint32 i = 0; i < _animations.size(); ++i)
    {
        if (_animations[i] && _animations[i]->Get_Name() == animName)
        {
            return static_cast<int32>(i);
        }
    }

    return -1;
}

bool Model::Play_Animation(float timeDelta)
{
    if (_currentAnimationIndex < 0 ||
        _currentAnimationIndex >= static_cast<int32>(_animations.size()))
    {
        return false;
    }

    // 매 프레임 기본 자세로 먼저 되돌리고 채널 덮어쓰기
    for (auto& bone : _bones)
    {
        bone->Reset_ToNodeTransform();
    }

    const bool finished =
        _animations[_currentAnimationIndex]->Update_TransformationMatrices(timeDelta, _bones, _isAnimationLoop);

    for (size_t i = 0; i < _bones.size(); ++i)
    {
        const int32 parentIndex = _bones[i]->Get_ParentIndex();
        const Matrix* parentMatrix = (parentIndex >= 0) ?
            &_bones[parentIndex]->Get_CombinedTransform() : nullptr;

        _bones[i]->Update_Combined(parentMatrix, _preLocalTransformMatrix);
        _boneMatrices[i] = _bones[i]->Get_SkinningMatrix();

        if (!_boneMatrices.empty())
        {
            const Matrix& m = _boneMatrices[0];
        }
    }

    return finished;
}

HRESULT Model::Bind_BoneMatrices(Shared<Shader> shader, const char* constantName)
{
    if (_boneMatrices.empty())
        return S_OK;

    return shader->Bind_RawValue(constantName, _boneMatrices.data(),
        static_cast<uint32>(sizeof(Matrix) * _boneMatrices.size()));
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
        Shared<ModelMaterial> material = nullptr;

        string matInstGuid = item.value("material_instance_guid", "");

        if (!matInstGuid.empty())
        {
            wstring matInstPath = GAME->Resolve_AssetPath(matInstGuid);
            if (!matInstPath.empty())
            {
                material = make_shared<ModelMaterial>(_device, _context);

                CHECK_FAILED(material->Initialize_FromMaterialInstance(
                    Utils::ToString(matInstPath)),E_FAIL);

                material->From_Json(item);
            }

            else
            {
                material = ModelMaterial::Create(_device, _context, item, materialFilePath);
            }
        }

        if (material == nullptr)
        {
            material = ModelMaterial::Create(
                _device, _context, item, materialFilePath);
        }

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

    j["model_type"] = (_modelType == EMeshVertexType::SkeletalMesh) ? "SkeletalMesh" : "StaticMesh";

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

    EMeshVertexType newType = (newTypeStr == "SkeletalMesh") ? EMeshVertexType::SkeletalMesh : EMeshVertexType::StaticMesh;

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
    CHECK_FAILED(Ready_FromBinary(modelFilePath), E_FAIL);
    CHECK_FAILED(Ready_Materials_FromJson(Build_MaterialJsonPath(modelFilePath)), E_FAIL);

    return S_OK;
}

HRESULT Model::Ready_FromBinary(const string& modelFilePath)
{
    FModelBinaryData data{};
    if (!Model_BinaryLoader::Load(modelFilePath, data))
        return E_FAIL;

    LOG_INFO("Ready_FromBinary path = {}", modelFilePath);
    LOG_INFO("modelType = {}", static_cast<int>(data.modelType));
    LOG_INFO("meshes = {}", data.meshes.size());
    LOG_INFO("bones = {}", data.bones.size());
    LOG_INFO("animations = {}", data.animations.size());

    _numMaterials = data.materialCount;

    if (data.modelType == EMeshVertexType::StaticMesh)
    {
        return Ready_StaticMeshes(data);
    }

    CHECK_FAILED(Ready_SkeletalMeshes(data), E_FAIL);
    CHECK_FAILED(Ready_Bones(data), E_FAIL);
    CHECK_FAILED(Ready_Animations(data), E_FAIL);

    _boneMatrices.resize(_bones.size(), Matrix::Identity);

    return S_OK;
}

HRESULT Model::Ready_StaticMeshes(const FModelBinaryData& data)
{
    _meshes.clear();
    _numMeshes = static_cast<uint32>(data.meshes.size());

    for (const FMeshBinaryData& srcMesh : data.meshes)
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

HRESULT Model::Ready_SkeletalMeshes(const FModelBinaryData& data)
{
    _meshes.clear();
    _numMeshes = static_cast<uint32>(data.meshes.size());

    LOG_INFO("Ready_SkeletalMeshes meshCount = {}", data.meshes.size());

    for (const auto& srcMesh : data.meshes)
    {
        vector<VTXANIM> vertices;
        vertices.reserve(srcMesh.animVertices.size());

        for (const auto& raw : srcMesh.animVertices)
        {
            VTXANIM vertex{};
            vertex.position = Vec3(raw.px, raw.py, raw.pz);
            vertex.normal = Vec3(raw.nx, raw.ny, raw.nz);
            vertex.tangent = Vec3(raw.tx, raw.ty, raw.tz);
            vertex.texcoord = Vec2(raw.u, raw.v);

            vertex.blendIndex = XMUINT4(
                raw.blendIndex[0],
                raw.blendIndex[1],
                raw.blendIndex[2],
                raw.blendIndex[3]);

            vertex.blendWeight = Vec4(
                raw.blendWeight[0],
                raw.blendWeight[1],
                raw.blendWeight[2],
                raw.blendWeight[3]);

            //vertex.position = Vec3::Transform(vertex.position, _preLocalTransformMatrix);
            //vertex.normal = Vec3::TransformNormal(vertex.normal, _preLocalTransformMatrix);
            //vertex.tangent = Vec3::TransformNormal(vertex.tangent, _preLocalTransformMatrix);
            
            vertices.push_back(vertex);
        }

        Shared<Mesh> mesh = Mesh::Create(_device, _context,
            srcMesh.name, srcMesh.materialIndex, vertices, srcMesh.indices);

        CHECK_NULL(mesh, E_FAIL);
        _meshes.push_back(mesh);
    }

    return S_OK;
}

HRESULT Model::Ready_Bones(const FModelBinaryData& data)
{
    _bones.clear();
    _bones.reserve(data.bones.size());

    LOG_INFO("Ready_Bones count = {}", data.bones.size());

    for (const auto& boneRaw : data.bones)
    {
        LOG_INFO("bone = {}, parent = {}", boneRaw.name, boneRaw.parentIndex);

        Shared<Bone> bone = Bone::Create(boneRaw);
        CHECK_NULL(bone, E_FAIL);
        _bones.push_back(bone);
    }

    return S_OK;
}

HRESULT Model::Ready_Animations(const FModelBinaryData& data)
{
    _animations.clear();
    _animations.reserve(data.animations.size());

    for (const auto& animRaw : data.animations)
    {
        Shared<Animation> animation = Animation::Create(animRaw);
        CHECK_NULL(animation, E_FAIL);
        _animations.push_back(animation);
    }

    return S_OK;
}

Shared<Model> Model::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EMeshVertexType type, const string& modelFilePath, const Matrix& preLocalTransformMatrix)
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
