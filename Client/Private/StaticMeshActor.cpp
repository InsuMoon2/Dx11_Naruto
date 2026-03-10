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

    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    // desc가 있으면 기존처럼 즉시 모델 준비
    if (desc && !desc->modelGuid.empty())
    {
        CHECK_FAILED(Apply_ModelGuid(desc->modelGuid), E_FAIL);
        CHECK_FAILED(Ensure_ModelReady(), E_FAIL);
    }

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

    if (!_modelCom || !_shaderCom)
        return S_OK;

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    size_t numMeshes = _modelCom->Get_NumMeshes();

    for (size_t i = 0; i < numMeshes; ++i)
    {
        _modelCom->Bind_Material(_shaderCom, "g_DiffuseTexture", i, EMaterialTextureSlot::BaseColor, 0);

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
    {
        string newGuid = data["model_guid"].get<string>();

        if (_modelGuid.empty())
        {
            if (FAILED(Apply_ModelGuid(newGuid)))
            {
                LOG_ERROR("StaticMeshActor::From_Json - model_guid 적용 실패: {}", newGuid);
                return;
            }
        }
        else if (_modelGuid != newGuid)
        {
            LOG_WARN("StaticMeshActor '{}' already has model_guid '{}', incoming guid '{}' ignored.",
                Utils::ToString(_name), _modelGuid, newGuid);
        }
    }

    if (!_modelGuid.empty())
    {
        if (FAILED(Ensure_ModelReady()))
        {
            LOG_ERROR("StaticMeshActor::From_Json - 모델 준비 실패: {}", _modelGuid);
        }
    }
        
}

HRESULT StaticMeshActor::Apply_ModelGuid(const string& modelGuid)
{
    if (modelGuid.empty())
    {
        LOG_ERROR("StaticMeshActor::Apply_ModelGuid - empty model guid");
        return E_FAIL;
    }

    if (!_modelGuid.empty() && _modelGuid != modelGuid)
    {
        LOG_WARN("StaticMeshActor '{}' already initialized with another guid. current='{}', incoming='{}'",
            Utils::ToString(_name), _modelGuid, modelGuid);
        return E_FAIL;
    }

    _modelGuid = modelGuid;

    return Resolve_ModelAsset();
}

HRESULT StaticMeshActor::Resolve_ModelAsset()
{
    if (_modelGuid.empty())
        return E_FAIL;

    _resolvedPath = Utils::ToString(GAME->Resolve_AssetPath(_modelGuid));

    if (_resolvedPath.empty())
    {
        LOG_ERROR("StaticMeshActor: GUID {} 에 해당하는 에셋을 찾을 수 없습니다.", _modelGuid);
        return E_FAIL;
    }

    return S_OK;
}

HRESULT StaticMeshActor::Ensure_ModelReady()
{
    if (_modelCom && _shaderCom)
        return S_OK;

    CHECK_FAILED(Resolve_ModelAsset(), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

HRESULT StaticMeshActor::Ready_Components()
{
    if (_modelCom && _shaderCom)
        return S_OK;

    if (_modelGuid.empty())
    {
        LOG_ERROR("StaticMeshActor::Ready_Components - model guid is empty");
        return E_FAIL;
    }

    if (_resolvedPath.empty())
    {
        CHECK_FAILED(Resolve_ModelAsset(), E_FAIL);
    }

    uint32 modelKey = static_cast<uint32>(hash<string>{}(_modelGuid));

    Matrix scaleMatrix = Matrix::CreateScale(1.f);
    Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(180.f));
    Matrix preTransform = scaleMatrix * rotationMatrix;

    auto proto = Model::Create(_device, _context, EModelType::StaticMesh, _resolvedPath, preTransform);
    CHECK_NULL(proto, E_FAIL);

    GAME->Add_Component_Prototype(ETOI(ELevelType::Static), modelKey, proto);
    CHECK_FAILED(Add_Component(ETOI(ELevelType::Static), modelKey, _modelCom), E_FAIL);
    CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_SHADER_VTXMESH, _shaderCom), E_FAIL);

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
