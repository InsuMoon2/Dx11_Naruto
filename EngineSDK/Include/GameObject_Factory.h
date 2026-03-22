#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameObject;

class ENGINE_DLL GameObject_Factory : public Base
{
    using Creator = function<shared_ptr<GameObject>(ComPtr<Device>, ComPtr<DeviceContext>)>;

public:
    struct FCreatorDesc
    {
        wstring name;
        Creator creator;
    };

public:
    explicit GameObject_Factory() = default;
    virtual ~GameObject_Factory() = default;

public:
    HRESULT Initialize();

    Shared<GameObject> Create_Object(ComPtr<Device> device, ComPtr<DeviceContext> context, Protocol::OBJECT_TYPE type);

public:
    static Unique<GameObject_Factory> Create();
    shared_ptr<GameObject> Create(const wstring& name, ComPtr<Device> device, ComPtr<DeviceContext> context);
    vector<wstring> Get_RegisteredNames();

    virtual void Free() override;

public:
    static void Register(Protocol::OBJECT_TYPE type, const wstring& name, Creator creator)
    {
        Get_Registry()[type] = { name, creator };
    }

private:
    static map<Protocol::OBJECT_TYPE, FCreatorDesc>& Get_Registry()
    {
        static map<Protocol::OBJECT_TYPE, FCreatorDesc> _creators;
        return _creators;
    }

};

#define REGISTER_GAMEOBJECT(TYPE, ENUM) \
    static struct Helper_##TYPE { \
        Helper_##TYPE() { \
            Engine::GameObject_Factory::Register(ENUM, L#TYPE, [](ComPtr<Device> device, ComPtr<DeviceContext> context) { \
                auto pInstance = TYPE::Create(device, context); \
                if (pInstance) \
                    pInstance->Set_ObjectType(ENUM);  \
                return pInstance; \
            }); \
        } \
    } helper_##TYPE;


NS_END
