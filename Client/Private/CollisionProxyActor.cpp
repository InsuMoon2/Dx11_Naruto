#include "pch.h"
#include "CollisionProxyActor.h"
#include "GameInstance.h"
#include "Model.h"
#include "Shader.h"
#include "GameObject_Factory.h"

REGISTER_GAMEOBJECT(CollisionProxyActor, Protocol::OBJECT_TYPE_COLLISION_PROXY)

CollisionProxyActor::CollisionProxyActor(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

CollisionProxyActor::CollisionProxyActor(const CollisionProxyActor& rhs)
    : GameObject(rhs)
    , _modelGuid(rhs._modelGuid)
    , _resolvedPath(rhs._resolvedPath)
    , _proxyType(rhs._proxyType)
    , _enabled(rhs._enabled)
    , _modelCom(rhs._modelCom)
    , _shaderCom(rhs._shaderCom)
{
}

HRESULT CollisionProxyActor::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT CollisionProxyActor::Initialize(void* arg)
{
    FCollisionProxyDesc* desc = static_cast<FCollisionProxyDesc*>(arg);

    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    if (desc)
    {
        _proxyType = desc->proxyType;
        _enabled = desc->enabled;

        if (!desc->modelGuid.empty())
        {
            CHECK_FAILED(Apply_ModelGuid(desc->modelGuid), E_FAIL);
            CHECK_FAILED(Ensure_ModelReady(), E_FAIL);
        }
    }

    return S_OK;
}

void CollisionProxyActor::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void CollisionProxyActor::Update(float timeDelta)
{
    GameObject::Update(timeDelta);
}

void CollisionProxyActor::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);
}

HRESULT CollisionProxyActor::Render()
{
    return GameObject::Render();
}

json CollisionProxyActor::To_Json() const
{
    json data = GameObject::To_Json();

    data["model_guid"] = _modelGuid;
    data["proxy_type"] = magic_enum::enum_name(_proxyType);
    data["enabled"] = _enabled;

    return data;
}

void CollisionProxyActor::From_Json(const json& data)
{
    GameObject::From_Json(data);

    if (data.contains("proxy_type"))
    {
        auto proxyType = magic_enum::enum_cast<ECollisionProxyType>(
            data["proxy_type"].get<string>());

        if (proxyType.has_value())
        {
            _proxyType = proxyType.value();
        }
        else
        {
            LOG_WARN(
                "CollisionProxyActor::From_Json - unknown proxy_type '{}', fallback to WorldBlock",
                data["proxy_type"].get<string>());

            _proxyType = ECollisionProxyType::WorldBlock;
        }
    }

    if (data.contains("enabled"))
    {
        _enabled = data["enabled"].get<bool>();
    }

    if (data.contains("model_guid"))
    {
        const string newGuid = data["model_guid"].get<string>();

        if (newGuid.empty())
        {
            _modelGuid.clear();
            _resolvedPath.clear();
            _modelCom = nullptr;
        }
        else if (_modelGuid.empty())
        {
            if (FAILED(Apply_ModelGuid(newGuid)))
            {
                LOG_ERROR(
                    "CollisionProxyActor::From_Json - model_guid 적용 실패: {}",
                    newGuid);
                return;
            }
        }
        else if (_modelGuid != newGuid)
        {
            LOG_WARN(
                "CollisionProxyActor '{}' already has model_guid '{}', incoming guid '{}' ignored.",
                Utils::ToString(_name),
                _modelGuid,
                newGuid);
        }
    }

    if (!_modelGuid.empty())
    {
        if (FAILED(Ensure_ModelReady()))
        {
            LOG_ERROR(
                "CollisionProxyActor::From_Json - 모델 준비 실패: {}",
                _modelGuid);
        }
    }
}

HRESULT CollisionProxyActor::Apply_ModelGuid(const string& modelGuid)
{
    if (modelGuid.empty())
    {
        _modelGuid.clear();
        _resolvedPath.clear();
        _modelCom = nullptr;
        return S_OK;
    }

    if (!_modelGuid.empty() && _modelGuid != modelGuid)
        return E_FAIL;

    _modelGuid = modelGuid;

    return Resolve_ModelAsset();
}

HRESULT CollisionProxyActor::Resolve_ModelAsset()
{
    // GUID를 asset path로 변환
    if (_modelGuid.empty())
        return E_FAIL;

    _resolvedPath = Utils::ToString(GAME->Resolve_AssetPath(_modelGuid));

    if (_resolvedPath.empty())
    {
        LOG_ERROR(
            "CollisionProxyActor: GUID {} 에 해당하는 에셋을 찾을 수 없습니다.",
            _modelGuid);

        return E_FAIL;
    }

    return S_OK;
}

HRESULT CollisionProxyActor::Ensure_ModelReady()
{
    if (_modelCom)
        return S_OK;

    if (_modelGuid.empty())
        return S_OK;

    CHECK_FAILED(Resolve_ModelAsset(), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

HRESULT CollisionProxyActor::Ready_Components()
{
    if (_modelCom)
        return S_OK;

    if (_modelGuid.empty())
    {
        LOG_ERROR("CollisionProxyActor::Ready_Components - model guid is empty");
        return E_FAIL;
    }

    if (_resolvedPath.empty())
    {
        CHECK_FAILED(Resolve_ModelAsset(), E_FAIL);
    }

    const uint32 staticLevelIndex = ETOI(ELevelType::Static);
    const uint32 modelKey = static_cast<uint32>(hash<string>{}(_modelGuid));

    // collision proxy도 동일 guid 프로토타입이 살아 있으면 재생성 대신 clone만 사용한다.
    if (GAME->Find_Component_Prototype(staticLevelIndex, modelKey) == nullptr)
    {
        // 기존에 사용하던 StaticMesh랑 동일하게 맞추기
        const Matrix scaleMatrix = Matrix::CreateScale(1.f);
        const Matrix rotationMatrix = Matrix::CreateRotationY(XMConvertToRadians(180.f));
        const Matrix preTransform = scaleMatrix * rotationMatrix;

        auto proto = Model::Create(
            _device, _context, EMeshVertexType::StaticMesh, _resolvedPath, preTransform, true);

        CHECK_NULL(proto, E_FAIL);

        CHECK_FAILED(GAME->Add_Component_Prototype(staticLevelIndex, modelKey, proto), E_FAIL);
    }

    CHECK_FAILED(Add_Component(staticLevelIndex, modelKey, _modelCom), E_FAIL);

    return S_OK;
}

Shared<GameObject> CollisionProxyActor::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<CollisionProxyActor>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CollisionProxyActor");
        return nullptr;
    }

    instance->Set_ObjectType(Protocol::OBJECT_TYPE_COLLISION_PROXY);

    return instance;
}

Shared<GameObject> CollisionProxyActor::Clone(void* arg)
{
    auto clone = make_shared<CollisionProxyActor>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : CollisionProxyActor");
        return nullptr;
    }

    return clone;
}

void CollisionProxyActor::Free()
{
    GameObject::Free();
}
