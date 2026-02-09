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
    static void Register(uint32 typeId, Creator creator, const wstring& className);
    static shared_ptr<Component> Create(uint32 typeId, ComPtr<Device> device, ComPtr<DeviceContext> context);

    // Prototype Clone
    static void Register_Prototype(uint32 typeId, const wstring& prototypeTag);
    static shared_ptr<Component> Clone_Prototype(uint32 typeId, uint32 levelIndex, void* arg = {});

    static vector<uint32> Get_RegisteredComponentIds();
    static vector<pair<uint32, wstring>> Get_RegisteredComponents();

private:
    static map<uint32, Creator> _creators;
    static map<uint32, wstring> _prototypeMap;
    static map<uint32, wstring> _classNames;

};

#define REGISTER_COMPONENT_FACTORY(TYPE, ENUM) \
    static struct Helper_Comp_##TYPE { \
        Helper_Comp_##TYPE() { \
            Engine::Component_Factory::Register(ENUM, \
                [](ComPtr<Device> device, ComPtr<DeviceContext> context) { \
                    return TYPE::Create(device, context); \
                }, \
                TYPE::StaticClassName()  \
            ); \
        } \
    } helper_comp_##TYPE;

NS_END
