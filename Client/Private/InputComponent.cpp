#include "pch.h"
#include "InputComponent.h"

InputComponent::InputComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

InputComponent::InputComponent(const InputComponent& rhs)
    : Component(rhs)
    , _frame(rhs._frame)
{
}

InputComponent::~InputComponent()
{
}

HRESULT InputComponent::Initialize_Prototype()
{
    Component::Initialize_Prototype();

    return S_OK;
}

HRESULT InputComponent::Initialize(void* arg)
{
    _frame = {};

    Component::Initialize(arg);

    return S_OK;
}

void InputComponent::BeginPlay()
{
    Component::BeginPlay();


}

void InputComponent::Update_Input(float timeDelta)
{
    _frame = {};

    if (INPUT->KeyPress(KEY_TYPE::W)) _frame.moveY += 1.f;
    if (INPUT->KeyPress(KEY_TYPE::S)) _frame.moveY -= 1.f;

    if (INPUT->KeyPress(KEY_TYPE::A)) _frame.moveX -= 1.f;
    if (INPUT->KeyPress(KEY_TYPE::D)) _frame.moveX += 1.f;

    _frame.sprintPress  = INPUT->KeyPress(KEY_TYPE::SHIFT);
    _frame.sprintDown   = INPUT->KeyDown(KEY_TYPE::SHIFT);
    _frame.jumpDown     = INPUT->KeyDown(KEY_TYPE::SPACE);

    Vec2 mouseDelta = INPUT->GetMouseDelta();
    _frame.lookYaw = mouseDelta.x;
    _frame.lookPitch = mouseDelta.y;

}

void InputComponent::Reset_FrameInput()
{
    _frame.jumpDown = false;
    _frame.sprintDown = false;
}

json InputComponent::To_Json() const
{
    json j = Component::To_Json();

    return j;
}

void InputComponent::From_Json(const json& data)
{
    Component::From_Json(data);
}

Shared<InputComponent> InputComponent::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<InputComponent>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create: InputComponent");
        instance.reset();
    }

    return instance;
}

Shared<Component> InputComponent::Clone(void* arg)
{
    auto clone = make_shared<InputComponent>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone: InputComponent");
        clone.reset();
    }

    return clone;
}

void InputComponent::Free()
{
    Component::Free();
}
