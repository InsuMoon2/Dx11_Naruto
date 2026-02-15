#pragma once

#include "Component.h"

NS_BEGIN(Client)

class InputComponent final : public Component
{
    GENERATED_COMPONENT(InputComponent, Protocol::COMPONENT_TYPE_INPUT)

public:
    struct FInputFrame
    {
        float moveX = 0.f;      
        float moveY = 0.f;      
        float lookYaw = 0.f;    // Mouse X
        float lookPitch = 0.f;  // Mouse Y

        bool sprintPress = false;
        bool sprintDown = false;
        bool jumpDown = false;
    };

public:
    explicit InputComponent(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit InputComponent(const InputComponent& rhs);
    virtual ~InputComponent();

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;

public:
    void Update_Input(float timeDelta);
    void Reset_FrameInput();

public:
    const FInputFrame& Get_Frame() const { return _frame; }

protected:
    json To_Json() const override;
    void From_Json(const json& data) override;

private:
    FInputFrame _frame;

public:
    static Shared<InputComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;

};

NS_END
