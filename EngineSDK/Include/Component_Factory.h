#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Component;

class ENGINE_DLL Component_Factory : public Base
{
public:
    using Creator = function<shared_ptr<Component>(ComPtr<Device>, ComPtr<DeviceContext>)>;

public:
    static void Initialize();

    // Direct Creator
    static void Register(uint32 typeId, Creator creator);
    static shared_ptr<Component> Create(uint32 typeId, ComPtr<Device> device, ComPtr<DeviceContext> context);

    // Prototype Clone
    static void Register_Prototype(uint32 typeId, const wstring& prototypeTag);
    static shared_ptr<Component> Clone_Prototype(uint32 typeId, uint32 levelIndex, void* arg = {});

private:
    static map<uint32, Creator> _creators;
    static map<uint32, wstring> _prototypeMap;

};

#define REGISTER_COMPONENT_FACTORY(TYPE) \
    static struct Helper_Comp_Factory_##TYPE { \
        Helper_Comp_Factory_##TYPE() { \
            Engine::Component_Factory::Register(TYPE::StaticTypeID(), \
                [](ComPtr<Device> device, ComPtr<DeviceContext> context) { return TYPE::Create(device, context); }); \
        } \
    } helper_comp_factory_##TYPE;

NS_END
