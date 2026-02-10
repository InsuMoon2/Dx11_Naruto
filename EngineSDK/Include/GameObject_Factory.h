#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameObject;

class ENGINE_DLL GameObject_Factory : public Base
{
    DECLARE_SINGLETON(GameObject_Factory)

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

    void Initialize();

    void Register(Protocol::OBJECT_TYPE type, const wstring& name, Creator creator);
    shared_ptr<GameObject> Create(Protocol::OBJECT_TYPE type, ComPtr<Device> device, ComPtr<DeviceContext> context);

    // 이름기반 탐색, 에디터용
    shared_ptr<GameObject> Create(const wstring& name, ComPtr<Device> device, ComPtr<DeviceContext> context);

    vector<wstring> Get_RegisteredNames();

private:
    map<Protocol::OBJECT_TYPE, FCreatorDesc> _creators;

};

#define REGISTER_GAMEOBJECT(TYPE, ENUM) \
    static struct Helper_##TYPE { \
        Helper_##TYPE() { \
          \
            Engine::GameObject_Factory::GetInstance()->Register(ENUM, L#TYPE, [](ComPtr<Device> device, ComPtr<DeviceContext> context) { \
                return make_shared<TYPE>(device, context); \
            }); \
        } \
    } helper_##TYPE;

NS_END
