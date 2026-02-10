#include "pch.h"
#include "ResourceLoader.h"
#include "GameInstance.h"
#include "Texture.h"
#include "Component_Factory.h"
#include "VIBuffer_Rect.h"
#include <fstream>

ResourceLoader::ResourceLoader(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

HRESULT ResourceLoader::Initialize()
{

    return S_OK;
}

HRESULT ResourceLoader::Load_Table(const wstring& tablePath, uint32 levelIndex)
{
    // Json 열기
    ifstream file(tablePath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open table: {}", Utils::ToString(tablePath));
        return E_FAIL;
    }

    json root;
    file >> root;
    file.close();

    for (auto& [typeName, items] : root.items())
    {
        CHECK_FAILED(Load_Components(items, levelIndex, typeName), E_FAIL);
    }

    LOG_INFO("Loaded table: {}", Utils::ToString(tablePath));
    
    return S_OK;
}

HRESULT ResourceLoader::Load_Components(const json& data, uint32 levelIndex, const string& typeName)
{
    for (const auto& item : data)
    {
        string idStr = item["id"];
        uint32 protoID = Get_ComponentID_From_String(idStr);

        if (protoID == 0)
        {
            LOG_WARN("Unknown component ID: {}", idStr);
            continue;
        }

        // Component_Factory에서 생성
        auto comp = Component_Factory::GetInstance()->Create(protoID, _device, _context);
        if (!comp)
        {
            LOG_ERROR("Failed to create component: {} (ID: {})", typeName, idStr);
            continue;
        }

        // 프로토타입 등록
        CHECK_FAILED(GAME->Add_Component_Prototype(levelIndex, protoID, comp), E_FAIL);

        LOG_INFO("Registered prototype: {} -> {}", typeName, idStr);
    }
    return S_OK;
}

uint32 ResourceLoader::Get_ComponentID_From_String(const string& idStr)
{
    const google::protobuf::EnumDescriptor* descriptor = Protocol::ComponentID_descriptor();
    const google::protobuf::EnumValueDescriptor* valueDesc =
        descriptor->FindValueByName(idStr);

    if (valueDesc == nullptr)
        return 0;

    return static_cast<uint32>(valueDesc->number());
}

shared_ptr<ResourceLoader> ResourceLoader::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<ResourceLoader>(device, context);

    if (FAILED(instance->Initialize()))
    {
        MSG_BOX("Failed to Create : Resource Loader");

        return nullptr;
    }

    return instance;
}

void ResourceLoader::Free()
{
    Base::Free();
}
