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
        string  category; // 에디터 정렬용
    };

    struct FRegisteredObjectDesc
    {
        Protocol::OBJECT_TYPE type = Protocol::OBJECT_TYPE_NONE;

        wstring name;
        string  category;
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

    // 특정 카테고리로 등록된 오브젝트 타입 목록을 에디터가 조회할 때 호출
    static vector<FRegisteredObjectDesc> Get_RegisteredObjectsByCategory(const string& category);

    // 에디터에서 오브젝트 타입이 특정 카테고리에 속하는지 방어적으로 확인용
    static bool Is_ObjectInCategory(Protocol::OBJECT_TYPE type, const string& category);

    virtual void Free() override;

public:
    static void Register(Protocol::OBJECT_TYPE type, const wstring& name, Creator creator, const string& category = "");

private:
    static map<Protocol::OBJECT_TYPE, FCreatorDesc>& Get_Registry();

};

#define REGISTER_GAMEOBJECT(TYPE, ENUM) \
    REGISTER_GAMEOBJECT_CATEGORY(TYPE, ENUM, "")

#define REGISTER_GAMEOBJECT_CATEGORY(TYPE, ENUM, CATEGORY) \
    static struct Helper_##TYPE { \
        Helper_##TYPE() { \
            Engine::GameObject_Factory::Register(ENUM, L#TYPE, [](ComPtr<Device> device, ComPtr<DeviceContext> context) -> Shared<GameObject> { \
                auto pInstance = TYPE::Create(device, context); \
                if (pInstance) \
                    pInstance->Set_ObjectType(ENUM); \
                return pInstance; \
            }, CATEGORY); \
        } \
    } helper_##TYPE;

NS_END
