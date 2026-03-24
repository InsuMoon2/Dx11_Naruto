#include "pch.h"
#include "ContainerObject.h"
#include "GameInstance.h"
#include "PartObject.h"

ContainerObject::ContainerObject(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

ContainerObject::ContainerObject(const ContainerObject& rhs)
    : GameObject(rhs)
{
}

HRESULT ContainerObject::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT ContainerObject::Initialize(void* arg)
{
    return GameObject::Initialize(arg);
}

void ContainerObject::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);

    for (auto& part : _partObjects)
    {
        if (part)
            part->Priority_Update(timeDelta);
    }

}

void ContainerObject::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    for (auto& part : _partObjects)
    {
        if (part)
            part->Update(timeDelta);
    }
}

void ContainerObject::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    for (auto& part : _partObjects)
    {
        if (part)
            part->Late_Update(timeDelta);
    }
}

HRESULT ContainerObject::Render()
{
    CHECK_FAILED(GameObject::Render(), E_FAIL);


    return S_OK;
}

HRESULT ContainerObject::Change_PartObject(EPartSlot slot, uint32 objID, void* arg)
{
    const int32 index = ETOI(slot);
    if (index < 0 || index >= ETOI(EPartSlot::END))
        return E_FAIL;

    if (_partObjects[index])
    {
        _partObjects[index]->Set_Destroy(true);
        _partObjects[index] = nullptr;
    }

    if (arg == nullptr)
        return S_OK;

    Shared<PartObject> partObject = static_pointer_cast<PartObject>(
        GAME->Clone_GameObject(0, objID, arg));

    CHECK_NULL(partObject, E_FAIL);

    _partObjects[index] = partObject;

    return S_OK;
}

string ContainerObject::Get_PartSlotName(EPartSlot slot)
{
    return string(magic_enum::enum_name(slot));
}

Shared<PartObject> ContainerObject::Get_PartObject(EPartSlot slot) const
{
    const uint32 index = ETOI(slot);

    if (index >= ETOI(EPartSlot::END))
        return nullptr;

    return _partObjects[index];
}

json ContainerObject::To_Json() const
{
    json j = GameObject::To_Json();

    json partsJson = json::object();
    for (int i = 0; i  < ETOI(EPartSlot::END); ++i)
    {
        if (_partObjects[i])
        {
            auto transform = _partObjects[i]->Get_Component<Transform>();
            if (transform)
            {
                string slotName = Get_PartSlotName(static_cast<EPartSlot>(i));
                partsJson[slotName] = transform->To_Json();
            }
        }
    }

    if (!partsJson.empty())
    {
        j["part_transforms"] = partsJson;
    }

    return j;
}

void ContainerObject::From_Json(const json& data)
{
    GameObject::From_Json(data);

    if (data.contains("part_transforms"))
        _cachedPartTransforms = data["part_transforms"];

}

HRESULT ContainerObject::Add_PartObject(EPartSlot slot, uint32 objID, void* arg)
{
    // 이미 파츠 장착
    if (Find_PartObject(slot) != nullptr)
        return E_FAIL;

    Shared<PartObject> partObject = static_pointer_cast<PartObject>(
        GAME->Clone_GameObject(0, objID, arg));

    if (partObject == nullptr)
        return E_FAIL;

    _partObjects[ETOI(slot)] = partObject;

    string slotName = Get_PartSlotName(slot);
    if (_cachedPartTransforms.contains(slotName) && _partObjects[ETOI(slot)])
    {
        auto transform = _partObjects[ETOI(slot)]->Get_Component<Transform>();
        if (transform)
            transform->From_Json(_cachedPartTransforms[slotName]);
    }


    return S_OK;
}

Shared<PartObject> ContainerObject::Find_PartObject(EPartSlot slot)
{
    int32 index = ETOI(slot);

    if (index >= ETOI(EPartSlot::END))
        return nullptr;

    return _partObjects[index];
}

void ContainerObject::Free()
{
    for (auto& part : _partObjects)
    {
        part = nullptr;
    }

    GameObject::Free();
}
