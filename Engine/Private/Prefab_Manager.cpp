#include "pch.h"
#include "Prefab_Manager.h"
#include "GameInstance.h"
#include <fstream>
#include "Component_Factory.h"
#include "GameObject.h"
#include "Transform.h"

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

    // Prefab 데이터 생성
    auto prefabData = make_shared<FPrefabDesc>();

    fs::path path(prefabPath);
    string fileName = path.filename().string();
    size_t dotPos = fileName.find('.'); // 첫번째 마침표 위치 찾기
    string prefabKey = (dotPos != string::npos) ? fileName.substr(0, dotPos) : path.stem().string();
    prefabData->prefab_name = prefabKey;

    prefabData->object_type = magic_enum::enum_cast<Protocol::OBJECT_TYPE>(
        root["object_type"].get<std::string>()).value_or(Protocol::OBJECT_TYPE_NONE);

    prefabData->components = root["components"];

    if (root.contains("custom_properties"))
    {
        prefabData->custom_properties = root["custom_properties"];
    }

    // 데이터 캐싱
    _prefabs[prefabKey] = prefabData;

    LOG_INFO("Loaded prefab : {}", prefabKey);

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

    // 저장 후 바로 등록
    return Load_Prefab(prefabPath);
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
    json root = gameObject->To_Json();

    root["prefab_name"] = Utils::ToString(gameObject->Get_Name());

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
        LOG_ERROR("Failed to Create GameObject to Factory");
        return nullptr;
    }

    // 컴포넌트 데이터 적용
    for (const auto& compData : desc.components)
    {
		uint32 typeId = 0;

		if (compData["type"].is_string())
		{
			string typeName = compData["type"].get<string>();
			auto result = magic_enum::enum_cast<Protocol::ComponentID>(typeName);

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

        // 기존 컴포넌트들 가져오기
        auto comp = gameObject->Find_Component_ByStaticType(typeId);

        if (comp == nullptr)
        {
            LOG_WARN("Prefab component not found in prototype (typeId: {}). Skipping.", typeId);
            continue;

#pragma region Legacy
            // 없으면 Factory로 생성, Id 기반
            //comp = Component_Factory::GetInstance()->Create(typeId, _device, _context);
            //if (comp)
            //{
            //    gameObject->Add_Component(typeId, comp);
            //}
#pragma endregion

            
        }

        // 데이터 로드
        if (comp)
            comp->From_Json(compData);
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

    gameObject->Set_Name(Utils::ToWString(desc.prefab_name));

    return gameObject;
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
