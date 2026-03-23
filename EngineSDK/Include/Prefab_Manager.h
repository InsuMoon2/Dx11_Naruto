#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameObject;

struct FPrefabDesc
{
    string prefab_name;
    Protocol::OBJECT_TYPE object_type;

    json components;
    json custom_properties;
};

class ENGINE_DLL Prefab_Manager : public Base
{
public:
    explicit Prefab_Manager(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Prefab_Manager();

public:
    HRESULT Initialize();

    HRESULT Load_Prefab(const string& prefabPath);
    HRESULT Save_Prefab(const string& prefabPath, shared_ptr<GameObject> gameObject);


    // Prefab에서 GameObject 생성 (인스턴싱)
    Shared<GameObject>  Instantiate_Prefab(const string& prefabName, const json& overrides = {});
    Shared<FPrefabDesc> Get_PrefabData(const string& prefabName);

    void Reapply_Prefabs_InLevel(uint32 levelIndex);

private:
    // 직렬화
    json                Serialize_GameObject(shared_ptr<GameObject> gameObject);
    // 역직렬화
    Shared<GameObject>  Deserialize_GameObject(const FPrefabDesc& desc, const json& overrides);
    string              Normalize_PrefabPath(const string& prefabPath);

    void Reapply_Prefab_ToObject(const FPrefabDesc& desc, Shared<GameObject> gameObject);
private:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;

    umap<string, shared_ptr<FPrefabDesc>> _prefabs;

public:
    static unique_ptr<Prefab_Manager> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual void Free() override;

};

NS_END
