#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class GameObject;

class ENGINE_DLL Controller abstract : public Component
{
    GENERATED_COMPONENT(Controller, Protocol::COMPONENT_TYPE_CONTROLLER)

public:
    explicit         Controller(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit         Controller(const Controller& rhs);
    virtual         ~Controller();

public:
    HRESULT         Initialize_Prototype() override;
    HRESULT         Initialize(void* arg) override;

    virtual void    Update(float timeDelta) {};

public:
    // 네이밍 편의성 함수
    Shared<GameObject> Get_Pawn() { return Get_Owner(); }

protected:
    json            To_Json() const override;
    void            From_Json(const json& data) override;

public:
    void            Free() override;

};

NS_END
