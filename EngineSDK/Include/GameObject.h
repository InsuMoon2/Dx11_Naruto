#pragma once

#include <type_traits>

#include "Transform.h"
#include "GameInstance.h"

NS_BEGIN(Engine)

class ENGINE_DLL GameObject abstract : public Base
{
    GENERATED_BODY(GameObject)

public:
    struct FGameObjectDesc : public Transform::FTransformDesc
    {
        wstring name = TEXT("");
    };

public:
    explicit GameObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit GameObject(const GameObject& rhs);
    virtual ~GameObject();

public:
    virtual HRESULT     Initialize_Prototype();
    virtual HRESULT     Initialize(void* arg);
    virtual void        BeginPlay();
    virtual void        Priority_Update(float timeDelta);
    virtual void        Update(float timeDelta);
    virtual void        Late_Update(float timeDelta);
    virtual HRESULT     Render();

    virtual json        To_Json() const;
    virtual void        From_Json(const json& data);

    virtual HRESULT     Bind_ShaderResources();

public: /* PendingKill */
    void Set_Destroy(bool flag) { _isDestroyed = flag; }
    bool Is_Destroy() const { return _isDestroyed; }

    Protocol::OBJECT_TYPE Get_ObjectType() const { return _objectType; }
    void Set_ObjectType(Protocol::OBJECT_TYPE type) { _objectType = type; }

    const string& Get_GUID() const { return _guid; }

    void Set_SourcePrefabName(const string& prefabName) { _sourcePrefabName = prefabName; }
    const string& Get_SourcePrefabName() const { return _sourcePrefabName; }

    void Set_Local(bool isLocal);

public:
    template<typename T>
    shared_ptr<T> Get_Component()
    {
        static_assert(std::is_base_of_v<Component, T>,
            "GameObject::Get_Component<T>() requires T to derive from Component.");

        uint32 id = T::StaticTypeID();

        for (auto& [key, comp] : _components)
        {
            if (comp && comp->Get_ComponentID() == id)
                return static_pointer_cast<T>(comp);
        }

        return nullptr;
    }

    shared_ptr<Component> Get_Component(uint32 id);

    template<typename T>
    HRESULT Add_Component(uint32 levelIndex, uint32 componentID, shared_ptr<T>& outCom, void* arg = {})
    {
        if (_components.contains(componentID))
            return E_FAIL;

        shared_ptr<Component> component = GAME->Clone_Component(levelIndex, componentID, arg);
        CHECK_NULL(component, E_FAIL);

        component->Set_Owner(this->GetSharedPtr());

        _components.emplace(componentID, component);
        outCom = static_pointer_cast<T>(component);

        return S_OK;
    }

    template<typename T>
    HRESULT Add_Component(uint32 componentID, shared_ptr<T>& outCom, void* arg = {})
    {
        if (_components.contains(componentID))
            return E_FAIL;

        shared_ptr<Component> component = GAME->Clone_Component(componentID, arg);
        CHECK_NULL(component, E_FAIL);

        component->Set_Owner(this->GetSharedPtr());

        _components.emplace(componentID, component);
        outCom = static_pointer_cast<T>(component);

        return S_OK;
    };

    HRESULT Add_Component(uint32 id, Shared<Component> component);

    void    Remove_Component(uint32 id);

    const map<uint32, shared_ptr<class Component>>& Get_Components() const { return _components; }

    Shared<Component> Find_Component_ByStaticType(uint32 componentID);

    Shared<Transform> Get_Transform() const { return _transformCom; }

protected:
    template<typename T = GameObject>
    shared_ptr<T> GetSharedPtr()
    {
        return static_pointer_cast<T>(shared_from_this());
    }

protected: /* Device */
    ComPtr<Device>          _device = { nullptr };
    ComPtr<DeviceContext>   _context = { nullptr };

protected: /* Component */
    map<uint32, shared_ptr<Component>> _components;

    shared_ptr<Transform>   _transformCom;

protected: /* Values */
    Protocol::OBJECT_TYPE _objectType = Protocol::OBJECT_TYPE::OBJECT_TYPE_NONE;

    bool _isDestroyed = false;
    bool _hasBegunPlay = false;

    // GUID
    string _guid;

    string _sourcePrefabName = "";

public:
    virtual shared_ptr<GameObject> Clone(void* arg) abstract;
    virtual void Free() override;
};

NS_END
