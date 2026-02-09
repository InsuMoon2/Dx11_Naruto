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

    void Initialize();

    static void Register(Protocol::OBJECT_TYPE type, const wstring& name, Creator creator);
    static shared_ptr<GameObject> Create(Protocol::OBJECT_TYPE type, ComPtr<Device> device, ComPtr<DeviceContext> context);

    // 이름기반 탐색, 에디터용
    static shared_ptr<GameObject> Create(const wstring& name, ComPtr<Device> device, ComPtr<DeviceContext> context);

    static vector<wstring> Get_RegisteredNames();

private:
    static map<Protocol::OBJECT_TYPE, FCreatorDesc> _creators;

};

#define REGISTER_GAMEOBJECT(TYPE, ENUM) \
    static struct Helper_##TYPE { \
        Helper_##TYPE() { \
          \
            Engine::GameObject_Factory::Register(ENUM, L#TYPE, [](ComPtr<Device> device, ComPtr<DeviceContext> context) { \
                return make_shared<TYPE>(device, context); \
            }); \
        } \
    } helper_##TYPE;

NS_END
