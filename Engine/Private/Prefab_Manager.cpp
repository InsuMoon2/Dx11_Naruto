#include "pch.h"
#include "Prefab_Manager.h"
#include "GameInstance.h"
#include <fstream>
#include "Component_Factory.h"
#include "GameObject.h"
#include "Transform.h"
#include <magic_enum/magic_enum.hpp>

#include "ContainerObject.h"
#include "PartObject.h"

Prefab_Manager::Prefab_Manager(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

Prefab_Manager::~Prefab_Manager()
{
}

HRESULT Prefab_Manager::Initialize()
{
    fs::path prefabDir = "../../Client/Bin/Resources/Data/json/Prefabs";

    if (!fs::exists(prefabDir))
    {
        fs::create_directories(prefabDir);
    }

    if (fs::exists(prefabDir))
    {
        for (const auto& entry : fs::directory_iterator(prefabDir))
        {
            string pathStr = entry.path().string();
            if (pathStr.ends_with(".prefab.json"))
            {
                Load_Prefab(entry.path().string());
            }
        }
    }
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

    if (!root.contains("object_type"))
    {
        LOG_ERROR("Prefab has no object_type : {}", prefabPath);
        return E_FAIL;
    }

    if (!root.contains("components"))
    {
        LOG_ERROR("Prefab has no components : {}", prefabPath);
        return E_FAIL;
    }


    // Prefab 데이터 생성
    auto prefabData = make_shared<FPrefabDesc>();

    fs::path path(prefabPath);
    string fileName = path.filename().string();

    const string suffix = ".prefab.json";
    string prefabKey;

    if (fileName.size() >= suffix.size() &&
        fileName.compare(fileName.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
        prefabKey = fileName.substr(0, fileName.size() - suffix.size());
    }
    else
    {
        prefabKey = path.stem().string();
    }

    prefabData->prefab_name = prefabKey;
    prefabData->object_type = magic_enum::enum_cast<Protocol::OBJECT_TYPE>(
        root["object_type"].get<string>()).value_or(Protocol::OBJECT_TYPE_NONE);

    if (prefabData->object_type == Protocol::OBJECT_TYPE_NONE)
    {
        LOG_ERROR("Invalid object_type in prefab : {}", prefabPath);
        return E_FAIL;
    }

    prefabData->components = root["components"];

    if (root.contains("custom_properties"))
    {
        prefabData->custom_properties = root["custom_properties"];
    }

    _prefabs[prefabKey] = prefabData;

    LOG_INFO("Loaded prefab : {}", prefabKey);

    return S_OK;
}

HRESULT Prefab_Manager::Save_Prefab(const string& prefabPath, shared_ptr<GameObject> gameObject)
{
    string normalizePath = Normalize_PrefabPath(prefabPath);

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

    // 저장 후 바로 등록
    return Load_Prefab(prefabPath);
}

shared_ptr<GameObject> Prefab_Manager::Instantiate_Prefab(const string& prefabName, const json& overrides)
{
    auto desc = Get_PrefabData(prefabName);

    if (!desc)
    {
        LOG_ERROR("Prefab not found: '{}'", prefabName);

        if (!_prefabs.empty())
        {
            string keys;
            for (const auto& [key, _] : _prefabs)
            {
                if (!keys.empty())
                    keys += ", ";
                keys += key;
            }

            LOG_WARN("Available prefab keys: {}", keys);
        }

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

void Prefab_Manager::Reapply_Prefabs_InLevel(uint32 levelIndex)
{
    const auto objects = GAME->Get_GameObjects(levelIndex);

    for (const auto& obj : objects)
    {
        if (!obj || obj->Is_Destroy())
            continue;

        const string& prefabName = obj->Get_SourcePrefabName();
        if (prefabName.empty())
            continue;

        auto prefabDesc = Get_PrefabData(prefabName);
        if (!prefabDesc)
            continue;

        Reapply_Prefab_ToObject(*prefabDesc, obj);
    }
}

HRESULT Prefab_Manager::Reapply_Prefab_ToObject(Shared<GameObject> gameObject)
{
    if (!gameObject)
        return E_FAIL;

    const string& prefabName = gameObject->Get_SourcePrefabName();
    if (prefabName.empty())
        return E_FAIL;

    auto prefabDesc = Get_PrefabData(prefabName);
    if (!prefabDesc)
        return E_FAIL;

    Reapply_Prefab_ToObject(*prefabDesc, gameObject);
    return S_OK;
}

json Prefab_Manager::Serialize_GameObject(shared_ptr<GameObject> gameObject)
{
    json root = gameObject->To_Json();

    root["prefab_name"] = Utils::ToString(gameObject->Get_Name());

    if (auto container = dynamic_pointer_cast<ContainerObject>(gameObject))
    {
        if (root.contains("part_objects"))
        {
            root["custom_properties"]["part_objects"] = root["part_objects"];
            root.erase("part_objects");
        }

        if (root.contains("part_transforms"))
        {
            root["custom_properties"]["part_transforms"] = root["part_transforms"];
            root.erase("part_transforms"); 
        }
    }

    return root;
}

shared_ptr<GameObject> Prefab_Manager::Deserialize_GameObject(const FPrefabDesc& desc, const json& overrides)
{
    // 팩토리에서 오브젝트 타입으로 생성 (리플렉션 해야함)

#pragma region Legacy : GameObject_Factory 사용
    //auto gameObject = GameObject_Factory::GetInstance()->Create(desc.object_type, _device, _context);
#pragma endregion

    auto gameObject = GAME->Clone_GameObject(0, desc.object_type, nullptr);

    if (!gameObject)
    {
        LOG_ERROR("Failed to clone prototype for prefab. prefab='{}', object_type='{}' ({})",
            desc.prefab_name, magic_enum::enum_name(desc.object_type),
            static_cast<uint32>(desc.object_type));

        return nullptr;
    }

    Apply_ComponentDataToObject(desc.components, gameObject, false);

    if (desc.custom_properties.is_object())
    {
        gameObject->From_Json(desc.custom_properties);
    }

    // Override 적용
    if (!overrides.empty())
    {
        auto transform = gameObject->Get_Component<Transform>();
        if (transform)
        {
            transform->From_Json(overrides);
        }
    }

    if (auto container = dynamic_pointer_cast<ContainerObject>(gameObject))
    {
        if (desc.custom_properties.contains("part_transforms"))
        {
            json spoof;
            spoof["part_transforms"] = desc.custom_properties["part_transforms"];
            container->From_Json(spoof);
        }

        Apply_PartObjectDataToContainer(desc.custom_properties, container);
    }

    gameObject->Set_Name(Utils::ToWString(desc.prefab_name));
    gameObject->Set_SourcePrefabName(desc.prefab_name);

    return gameObject;
}

string Prefab_Manager::Normalize_PrefabPath(const string& prefabPath)
{
    const string suffix = ".prefab.json";

    if (prefabPath.size() >= suffix.size() &&
            prefabPath.compare(prefabPath.size() - suffix.size(), suffix.size(), suffix) == 0)
    {
        return prefabPath;
    }

    return prefabPath + suffix;
}

void Prefab_Manager::Reapply_Prefab_ToObject(const FPrefabDesc& desc, Shared<GameObject> gameObject)
{
    if (!gameObject)
        return;

    Apply_ComponentDataToObject(desc.components, gameObject, true);

    if (desc.custom_properties.is_object())
    {
        gameObject->From_Json(desc.custom_properties);
    }

    if (auto container = dynamic_pointer_cast<ContainerObject>(gameObject))
    {
        if (desc.custom_properties.contains("part_transforms"))
        {
            json spoof;
            spoof["part_transforms"] = desc.custom_properties["part_transforms"];

            container->From_Json(spoof);
            for (int i = 0; i < ETOI(ContainerObject::EPartSlot::END); ++i)
            {
                auto slot = static_cast<ContainerObject::EPartSlot>(i);
                if (container->Get_PartObject(slot))
                {
                    auto transform = container->Get_PartObject(slot)->Get_Component<Transform>();
                    string slotName = ContainerObject::Get_PartSlotName(slot);
                    if (transform && spoof["part_transforms"].contains(slotName))
                    {
                        transform->From_Json(spoof["part_transforms"][slotName]);
                    }
                }
            }
        }

        Apply_PartObjectDataToContainer(desc.custom_properties, container);
    }
}

void Prefab_Manager::Apply_ComponentDataToObject(const json& components, Shared<GameObject> gameObject, bool skipTransform)
{
    if (!gameObject || !components.is_array())
        return;

    for (const auto& compData : components)
    {
        if (compData.is_null() || !compData.contains("type"))
            continue;

        uint32 typeId = 0;

        if (compData["type"].is_string())
        {
            const string typeName = compData["type"].get<string>();
            const auto result = magic_enum::enum_cast<Protocol::ComponentID>(typeName);

            if (!result.has_value())
            {
                LOG_WARN("Unknown component type: '{}'. Skipping.", typeName);
                continue;
            }

            typeId = static_cast<uint32>(result.value());
        }
        else
        {
            typeId = compData["type"].get<uint32>();
        }

        if (skipTransform && typeId == Transform::StaticTypeID())
            continue;

        auto comp = gameObject->Find_Component_ByStaticType(typeId);
        if (!comp)
        {
            LOG_WARN("Prefab component not found in prototype (typeId: {}). Skipping.", typeId);
            continue;
        }

        comp->From_Json(compData);
    }
}

void Prefab_Manager::Apply_PartObjectDataToContainer(const json& customProperties, Shared<ContainerObject> container)
{
    if (!container || !customProperties.contains("part_objects"))
        return;

    const json& partObjects = customProperties["part_objects"];
    if (!partObjects.is_object())
        return;

    for (int32 i = 0; i < ETOI(ContainerObject::EPartSlot::END); ++i)
    {
        const auto slot = static_cast<ContainerObject::EPartSlot>(i);
        const string slotName = ContainerObject::Get_PartSlotName(slot);
        if (!partObjects.contains(slotName))
            continue;

        const auto partObject = container->Get_PartObject(slot);
        if (!partObject)
            continue;

        const json& partJson = partObjects[slotName];
        partObject->From_Json(partJson);

        if (partJson.contains("components"))
        {
            Apply_ComponentDataToObject(partJson["components"], partObject, false);
        }
    }
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
