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

    if (!GAME->Is_GameInputEnabled())
    {
        _superJumpCharge = 0.f;
        return;
    }

    if (INPUT->KeyPress(KEY_TYPE::W)) _frame.moveY += 1.f;
    if (INPUT->KeyPress(KEY_TYPE::S)) _frame.moveY -= 1.f;

    if (INPUT->KeyPress(KEY_TYPE::A)) _frame.moveX -= 1.f;
    if (INPUT->KeyPress(KEY_TYPE::D)) _frame.moveX += 1.f;

    _frame.sprintPress  = INPUT->KeyPress(KEY_TYPE::SHIFT);
    _frame.sprintDown   = INPUT->KeyDown(KEY_TYPE::SHIFT);
    _frame.jumpDown     = INPUT->KeyDown(KEY_TYPE::SPACE);

    _frame.useSkillDown[0] = INPUT->KeyDown(KEY_TYPE::KEY_1);
    _frame.useSkillDown[1] = INPUT->KeyDown(KEY_TYPE::KEY_2);

    // TODO : Ctrl : 슈퍼점프, Left : 약공, Right : 강공, 우클릭 -> 벽타기 입체기동
    // TODO : 2단점프까지 가능하도록
    bool ctrlPress  = INPUT->KeyPress(KEY_TYPE::LCTRL);
    bool ctrlUp     = INPUT->KeyUp(KEY_TYPE::LCTRL);

    if (ctrlPress)
    {
        _superJumpCharge = ::clamp(_superJumpCharge + timeDelta, 0.f, MAX_JUMP_CHARGE);
    }
    else if (!ctrlUp)
    {
        _superJumpCharge = 0.f;
    }

    Vec2 mouseDelta = INPUT->GetMouseDelta();
    _frame.lookYaw = mouseDelta.x;
    _frame.lookPitch = mouseDelta.y;

    _frame.superJumpCharge = _superJumpCharge;
    _frame.superJumpPress = ctrlPress;
    _frame.superJumpUp = ctrlUp;
}

void InputComponent::Reset_FrameInput()
{
    _frame.jumpDown = false;
    _frame.sprintDown = false;

    _frame.superJumpUp = false;

    _frame.useSkillDown[0] = false;
    _frame.useSkillDown[1] = false;
}

bool InputComponent::Has_MoveInput() const
{
    return Vec2(_frame.moveX, _frame.moveY).LengthSquared() > FLT_EPSILON;
}

Vec2 InputComponent::Get_MoveAxis() const
{
    return Vec2(_frame.moveX, _frame.moveY);
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
