#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class PartObject;

class ENGINE_DLL ContainerObject : public GameObject
{
    GENERATED_BODY(ContainerObject)

public:
    struct FContainerObjectDesc : public FGameObjectDesc
    {
        //
    };

    enum class EPartSlot : uint8
    {
        Headegear,
        Accessory,
        Onepiece,
        BodyUpper,
        BodyLower,
        Face,


        Weapon,

        END
    };

public:
    explicit ContainerObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit ContainerObject(const ContainerObject& rhs);
    virtual ~ContainerObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

public:
    HRESULT             Change_PartObject(EPartSlot slot, uint32 objID, void* arg);
    static string       Get_PartSlotName(EPartSlot slot);

    Shared<PartObject>  Get_PartObject(EPartSlot slot) const;

    json To_Json() const override;
    void From_Json(const json& data) override;

protected:
    HRESULT             Add_PartObject(EPartSlot slot, uint32 objID, void* arg);
    Shared<PartObject>  Find_PartObject(EPartSlot slot);

protected:
    array<Shared<PartObject>, ETOI(EPartSlot::END)> _partObjects;

    json _cachedPartTransforms = json::object();

public:
    virtual Shared<GameObject> Clone(void* arg) = 0;
    void Free() override;

};

NS_END
