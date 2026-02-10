#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Component;

class ENGINE_DLL Component_Factory : public Base
{
    DECLARE_SINGLETON(Component_Factory)

public:
    using Creator = function<shared_ptr<Component>(ComPtr<Device>, ComPtr<DeviceContext>)>;

public:
    explicit Component_Factory() = default;
    virtual ~Component_Factory() = default;

public:
    void Initialize();

    // Direct Creator
    void Register(uint32 typeId, Creator creator, const wstring& className);
    shared_ptr<Component> Create(uint32 typeId, ComPtr<Device> device, ComPtr<DeviceContext> context);

    // Prototype Clone
    void Register_Prototype(uint32 typeId, const wstring& prototypeTag);
    shared_ptr<Component> Clone_Prototype(uint32 typeId, uint32 levelIndex, void* arg = {});

    vector<uint32> Get_RegisteredComponentIds();
    vector<pair<uint32, wstring>> Get_RegisteredComponents();

private:
    map<uint32, Creator> _creators;
    map<uint32, wstring> _prototypeMap;
    map<uint32, wstring> _classNames;

};

#define REGISTER_COMPONENT_FACTORY(TYPE, ENUM) \
    static struct Helper_Comp_##TYPE { \
        Helper_Comp_##TYPE() { \
            Engine::Component_Factory::GetInstance()->Register(ENUM, \
                [](ComPtr<Device> device, ComPtr<DeviceContext> context) { \
                    return TYPE::Create(device, context); \
                }, \
                TYPE::StaticClassName()  \
            ); \
        } \
    } helper_comp_##TYPE;

NS_END
