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

    void Register(uint32 typeId, Creator creator, const wstring& className);
    void Register_Prototype(uint32 typeId, uint32 levelIndex,
                       ComPtr<Device> device, ComPtr<DeviceContext> context);

    shared_ptr<Component> Create(uint32 typeId, ComPtr<Device> device, ComPtr<DeviceContext> context);

    vector<pair<uint32, wstring>> Get_RegisteredComponents();

public:
    template <typename T>
        void Register(uint32 levelIndex, ComPtr<Device> device, ComPtr<DeviceContext> context)
    {
        uint32 id = T::StaticTypeID();

        if (!_creators.contains(id))
        {
            _creators[id] = [](auto device, auto context) { return T::Create(device, context); };
            _classNames[id] = T::StaticClassName();
        }

        Register_Prototype(id, levelIndex, device, context);
    }

private:
    map<uint32, Creator> _creators;
    map<uint32, wstring> _classNames;

public:
    void Free() override;

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
