#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Component;

class ENGINE_DLL Component_Factory : public Base
{
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

    Shared<Component> Instantiate(uint32 typeId, ComPtr<Device> device, ComPtr<DeviceContext> context);

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
    static Unique<Component_Factory> Create();
    void Free() override;

};

NS_END
