#include "pch.h"
#include "StaticMeshActor.h"
#include "GameInstance.h"
#include "Shader.h"
#include "Model.h"

StaticMeshActor::StaticMeshActor(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_STATIC_MESH);
}

StaticMeshActor::StaticMeshActor(const StaticMeshActor& rhs)
    : GameObject(rhs)
    , _modelGuid(rhs._modelGuid)
    , _resolvedPath(rhs._resolvedPath)
    , _shaderCom(rhs._shaderCom)
    , _modelCom(rhs._modelCom)
{
}

StaticMeshActor::~StaticMeshActor()
{
}

HRESULT StaticMeshActor::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT StaticMeshActor::Initialize(void* arg)
{
    FStaticMeshDesc* desc = static_cast<FStaticMeshDesc*>(arg);
    CHECK_NULL(desc, E_FAIL);

    _modelGuid = desc->modelGuid;

    // GUID -> 실제 경로
    _resolvedPath = Utils::ToString(GAME->Resolve_AssetPath(_modelGuid));

    if (_resolvedPath.empty())
    {
        LOG_ERROR("StaticMeshActor: GUID {} 에 해당하는 에셋을 찾을 수 없습니다.", _modelGuid);
        return E_FAIL;
    }

    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

void StaticMeshActor::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void StaticMeshActor::Update(float timeDelta)
{
    GameObject::Update(timeDelta);
}

void StaticMeshActor::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    GAME->Add_RenderGroup(ERenderGroup::NonBlend, GetSharedPtr());
}

HRESULT StaticMeshActor::Render()
{
    GameObject::Render();

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    size_t numMeshes = _modelCom->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; ++i)
    {
        _modelCom->Bind_Material(_shaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE, 0);

        CHECK_FAILED(_shaderCom->Begin_Pass(0), E_FAIL);
        CHECK_FAILED(_modelCom->Render(i), E_FAIL);
    }

    return S_OK;
}

HRESULT StaticMeshActor::Bind_ShaderResources()
{
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_WorldMatrix", &_transformCom->Get_WorldMatrix()), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ViewMatrix", GAME->Get_Transform(ETransformState::View)), E_FAIL);
    CHECK_FAILED(_shaderCom->Bind_Matrix("g_ProjMatrix", GAME->Get_Transform(ETransformState::Proj)), E_FAIL);

    GAME->Bind_CamPosition(_shaderCom, "g_CamPosition");

    // 라이트 : Temp
    const FLightDesc* lightDesc = GAME->Get_LightDesc(0);
    if (lightDesc)
    {
        _shaderCom->Bind_RawValue("g_LightDir", &lightDesc->direction, sizeof(Vec4));
        _shaderCom->Bind_RawValue("g_LightDiffuse", &lightDesc->diffuse, sizeof(Color));
        _shaderCom->Bind_RawValue("g_LightAmbient", &lightDesc->ambient, sizeof(Color));
        _shaderCom->Bind_RawValue("g_LightSpecular", &lightDesc->specular, sizeof(Color));
    }

    return S_OK;
}

json StaticMeshActor::To_Json() const
{
    json j = GameObject::To_Json();
    j["model_guid"] = _modelGuid;

    return j;
}

void StaticMeshActor::From_Json(const json& data)
{
    GameObject::From_Json(data);

    if (data.contains("model_guid"))
        _modelGuid = data["model_guid"].get<string>();
}

HRESULT StaticMeshActor::Ready_Components()
{
    uint32 modelKey = static_cast<uint32>(hash<string>{}(_modelGuid));

    Matrix scaleMatrix = Matrix::CreateScale(0.01f);
    Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(180.f));

    Matrix preTransform = scaleMatrix * rotationMatrix;

    auto proto = Model::Create(_device, _context, EModelType::StaticMesh, _resolvedPath, preTransform);
    CHECK_NULL(proto, E_FAIL);

    GAME->Add_Component_Prototype(ETOI(ELevelType::Static), modelKey, proto);

    CHECK_FAILED(Add_Component(ETOI(ELevelType::Static), modelKey, _modelCom), E_FAIL);

    CHECK_FAILED(Add_Component(
        Protocol::COMPONENT_TYPE_SHADER_VTXMESH, _shaderCom), E_FAIL);

    return S_OK;
}

Shared<GameObject> StaticMeshActor::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<StaticMeshActor>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : StaticMeshActor");
        return nullptr;

    }
    return instance;
}

Shared<GameObject> StaticMeshActor::Clone(void* arg)
{
    auto instance = make_shared<StaticMeshActor>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : StaticMeshActor");
        return nullptr;
    }

    return instance;
}

void StaticMeshActor::Free()
{
    GameObject::Free();
}
