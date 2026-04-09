#include "pch.h"
#include "Component_Factory.h"
#include "Component.h"
#include "GameInstance.h"

void Component_Factory::Initialize()
{
    _creators.clear();
    _classNames.clear();
}

void Component_Factory::Register(uint32 typeId, Creator creator, const wstring& className)
{
    if (_creators.contains(typeId))
        return;

    _creators.emplace(typeId, creator);
    _classNames.emplace(typeId, className);
}

void Component_Factory::Register_Prototype(uint32 typeId, uint32 levelIndex, ComPtr<Device> device,
    ComPtr<DeviceContext> context)
{
    auto iter = _creators.find(typeId);
    if (iter == _creators.end())
    {
        LOG_ERROR("Component not registered in factory (typeId: {})", typeId);
        return;
    }

	const wstring className = _classNames.contains(typeId)
		? _classNames[typeId]
		: L"<unknown>";

    // Creator로 프로토타입 인스턴스 생성
    auto prototype = iter->second(device, context);
    if (!prototype)
    {
        LOG_ERROR(
            "Failed to create prototype (typeId: {}, className: '{}', levelIndex: {})",
            typeId,
            Utils::ToString(className),
            levelIndex);
    }

    // 만든 후 프로토타입 등록
    GAME->Add_Component_Prototype(levelIndex, typeId, prototype);
}

shared_ptr<Component> Component_Factory::Instantiate(uint32 typeId, ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto iter = _creators.find(typeId);
    if (iter == _creators.end())
        return nullptr;

    return iter->second(device, context);
}

vector<pair<uint32, wstring>> Component_Factory::Get_RegisteredComponents()
{
    vector<pair<uint32, wstring>> components;
    components.reserve(_creators.size());

    for (const auto& pair : _classNames)
    {
        components.push_back(pair); 
    }
    return components;
}

Unique<Component_Factory> Component_Factory::Create()
{
    auto instance = make_unique<Component_Factory>();
    instance->Initialize();

    return instance;
}

void Component_Factory::Free()
{
    _creators.clear();
    _classNames.clear();

    Base::Free();
}
