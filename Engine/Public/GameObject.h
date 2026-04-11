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
    virtual bool        Should_ExcludeFromEditorSnapshot() const { return false; }

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
    Shared<T> Get_Component()
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

    const map<uint32, Shared<Component>>& Get_Components() const { return _components; }

    Shared<Component> Find_Component_ByStaticType(uint32 componentID);

    Shared<Transform> Get_Transform() const { return _transformCom; }

public:
    template<typename T = GameObject>
    shared_ptr<T> GetSharedPtr()
    {
        return static_pointer_cast<T>(shared_from_this());
    }

public: /* Collision Events */
    virtual void OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) {}
    virtual void OnStayOverlap(Shared<Collider> self, Shared<Collider> other)  {}
    virtual void OnEndOverlap(Shared<Collider> self, Shared<Collider> other)   {}

    virtual void OnBlockBegin(Shared<Collider> self, Shared<Collider> other) {}
    virtual void OnBlockStay(Shared<Collider> self, Shared<Collider> other) {}

public:
    void Set_Owner(Shared<GameObject> owner) { _owner = owner; }
    Shared<GameObject> Get_Owner() { return _owner.lock(); }

    void Set_LevelIndex(uint32 index) { _levelIndex = index; }
    uint32 Get_LevelIndex() const { return _levelIndex; }

protected: /* Device */
    ComPtr<Device>          _device = { nullptr };
    ComPtr<DeviceContext>   _context = { nullptr };

protected: /* Component */
    map<uint32, shared_ptr<Component>> _components;

    Shared<Transform>   _transformCom;

protected: /* Values */
    Protocol::OBJECT_TYPE _objectType = Protocol::OBJECT_TYPE::OBJECT_TYPE_NONE;

    bool _isDestroyed = false;
    bool _hasBegunPlay = false;

    // GUID
    string _guid;

    string _sourcePrefabName = "";

    Weak<GameObject> _owner;

    uint32 _levelIndex = 0;

public:
    virtual Shared<GameObject> Clone(void* arg) abstract;
    virtual void Free() override;
};

NS_END
