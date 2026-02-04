#include "pch.h"
#include "Prefab_Manager.h"
#include "GameInstance.h"
#include <fstream>

#include "GameObject.h"

Prefab_Manager::Prefab_Manager(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

Prefab_Manager::~Prefab_Manager()
{
}

HRESULT Prefab_Manager::Initialize()
{

    return S_OK;
}

HRESULT Prefab_Manager::Load_Prefab(const string& prefabPath)
{
    ifstream file(prefabPath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open prefab file : {}", prefabPath);
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    // Prefab 데이터 생성
    auto prefabData = make_shared<FPrefabDesc>();
    prefabData->prefab_name = root["prefab_name"];
    prefabData->object_type = static_cast<OBJECT_TYPE>(root["object_type"].get<int>());
    prefabData->components = root["components"];

    if (root.contains("custom_properties"))
    {
        prefabData->custom_properties = root["custom_properties"];
    }

    // 데이터 캐싱
    _prefabs[prefabData->prefab_name] = prefabData;

    LOG_INFO("Loaded prefab : {}", prefabData->prefab_name);

    return S_OK;
}

HRESULT Prefab_Manager::Save_Prefab(const string& prefabPath, shared_ptr<GameObject> gameObject)
{
    json root = Serialize_GameObject(gameObject);

    ofstream file(prefabPath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to create prefab file : {}", prefabPath);
        return E_FAIL;
    }

    file << root.dump(4); // 들여쓰기 4칸
    file.close();

    LOG_INFO("Save prefab : {}", prefabPath);

    return S_OK;
}

shared_ptr<GameObject> Prefab_Manager::Instantiate_Prefab(const string& prefabName, const json& overrides)
{
    auto desc = Get_PrefabData(prefabName);

    if (!desc)
    {
        LOG_ERROR("Prefab not found: {}", prefabName);
        return nullptr;
    }

    return Deserialize_GameObject(*desc, overrides);
}

shared_ptr<FPrefabDesc> Prefab_Manager::Get_PrefabData(const string& prefabName)
{
    auto iter = _prefabs.find(prefabName);

    if (iter == _prefabs.end())
        return nullptr;

    return iter->second;
}

json Prefab_Manager::Serialize_GameObject(shared_ptr<GameObject> gameObject)
{
    json root;

    root["prefab_name"] = "NewPrefab"; // TODO : 추후 이름 변경 예정
    root["object_type"] = static_cast<int>(gameObject->Get_ObjectType());

    json components = json::array();

    auto transform = gameObject->Get_Component<Transform>();
    if (transform)
    {
        //json transformData;
        //transformData["type"] = "Transform";
        //
        //Vec3 pos = transform->Get_State(Transform::STATE_POSITION);
        //transformData["position"] = { pos.x, pos.y, pos.z };
        //
        //Vec3 rot = transform->Get_Rotation();
        //transformData["rotation"] = { rot.x, rot.y, rot.z };
        //
        //Vec3 scale = transform->Get_Scaled();
        //transformData["scale"] = { scale.x, scale.y, scale.z };

        //components.push_back(transformData);
    }

    // TODO: 다른 Component들도 직렬화

    root["components"] = components;

    return root;
}

shared_ptr<GameObject> Prefab_Manager::Deserialize_GameObject(const FPrefabDesc& desc, const json& overrides)
{

}

unique_ptr<Prefab_Manager> Prefab_Manager::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_unique<Prefab_Manager>(device, context);

    if (FAILED(instance->Initialize()))
    {
        LOG_ERROR("Failed to Create : Prefab_Manager");
        return nullptr;
    }

    return instance;
}

void Prefab_Manager::Free()
{
    _prefabs.clear();

    Base::Free();
}
