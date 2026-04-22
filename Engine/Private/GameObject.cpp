#include "pch.h"
#include "GameObject.h"

GameObject::GameObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
    , _guid(Utils::Generate_GUID())
    , _isDestroyed(false)
    , _hasBegunPlay(false)
{
}

GameObject::GameObject(const GameObject& rhs)
    : _device(rhs._device), _context(rhs._context)
    , _guid(Utils::Generate_GUID())
    , _isDestroyed(false)
    , _hasBegunPlay(false)
{
    _objectType = rhs._objectType;
    _name = rhs._name;
}

GameObject::~GameObject()
{
}

HRESULT GameObject::Initialize(void* arg)
{
    // Transform
    {
        _transformCom = Transform::Create(_device, _context);
        CHECK_NULL(_transformCom, E_FAIL);

        CHECK_FAILED(_transformCom->Initialize(arg), E_FAIL);

        _transformCom->Set_IsLocal(_isLocal);
        _transformCom->Set_Owner(GetSharedPtr());

        _components.emplace(Transform::StaticTypeID(), _transformCom);
    }

    if (arg != nullptr)
    {
        FGameObjectDesc* desc = static_cast<FGameObjectDesc*>(arg);

        if (!desc->name.empty())
        {
            _name = desc->name;
        }

    }
    // 초기 스케일 보장
    else
    {
        _transformCom->Set_LocalScale(1.f, 1.f, 1.f);
    }

    // Test
    LOG_INFO(Get_GUID());

    return S_OK;
}

void GameObject::BeginPlay()
{
    if (_hasBegunPlay) return;
    _hasBegunPlay = true;

    for (auto& [id, component] : _components)
    {
        if (component)
        {
            component->BeginPlay();
        }
    }
}

HRESULT GameObject::Initialize_Prototype()
{

    return S_OK;
}

void GameObject::Priority_Update(float timeDelta)
{
    if (_isDestroyed)
        return;


}

void GameObject::Update(float timeDelta)
{
    if (_isDestroyed)
        return;

    
}

void GameObject::Late_Update(float timeDelta)
{
    if (_isDestroyed)
        return;


}

HRESULT GameObject::Render()
{
    if (_isDestroyed)
        return E_FAIL;

    CHECK_FAILED(Bind_ShaderResources(), E_FAIL);

    return S_OK;
}

json GameObject::To_Json() const
{
    json j;
    j["static_class"] = Utils::ToString(_name);
    j["object_type"] = magic_enum::enum_name(_objectType);
    j["guid"] = _guid;

    if (!_sourcePrefabName.empty())
        j["prefab_name"] = _sourcePrefabName;

    json components = json::array();
    for (auto& [id, comp] : _components)
    {
        components.emplace_back(comp->To_Json());
    }
    j["components"] = components;

    return j;
}

void GameObject::From_Json(const json& data)
{
    if (data.contains("static_class"))
    {
        _name = Utils::ToWString(data["static_class"].get<string>());
    }

    if (data.contains("object_type"))
    {
        _objectType = magic_enum::enum_cast<Protocol::OBJECT_TYPE>(
            data["object_type"].get<string>()).value_or(Protocol::OBJECT_TYPE_NONE);
    }

    if (data.contains("guid"))
    {
        _guid = data["guid"].get<string>();
    }

    if (data.contains("prefab_name"))
    {
        _sourcePrefabName = data["prefab_name"].get<string>();
    }

    // 컴포넌트 로드는 Prefab_Manager에서 세팅하기
}

HRESULT GameObject::Bind_ShaderResources()
{
    return S_OK;
}

void GameObject::Set_Local(bool isLocal)
{
    _isLocal = isLocal;

    // 자신이 리모트 객체면, 내가 가진 모든 컴포넌트들한테도 설정
    for (auto& pair : _components)
    {
        pair.second->Set_IsLocal(isLocal);
    }
}

shared_ptr<Component> GameObject::Get_Component(uint32 id)
{
    auto iter = _components.find(id);
    if (iter == _components.end())
        return nullptr;

    return iter->second;
}

HRESULT GameObject::Add_Component(uint32 id, shared_ptr<Component> component)
{
    if (_components.contains(id))
        return E_FAIL;

    component->Set_IsLocal(_isLocal);
    component->Set_Owner(GetSharedPtr());

    _components.emplace(id, component);

    return S_OK;
}

void GameObject::Remove_Component(uint32 id)
{
    auto iter = _components.find(id);

    if (iter != _components.end())
    {
        _components.erase(iter);
    }
}

Shared<Component> GameObject::Find_Component_ByStaticType(uint32 componentID)
{
    for (auto& [key, comp] : _components)
    {
        if (comp->Get_ComponentID() == componentID)
        {
            return comp;
        }
    }

    return nullptr;
}

void GameObject::Free()
{
    Base::Free();

    _components.clear();
}
