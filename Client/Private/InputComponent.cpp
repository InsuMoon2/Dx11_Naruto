#include "pch.h"
#include "InputComponent.h"

InputComponent::InputComponent(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

InputComponent::InputComponent(const InputComponent& rhs)
    : Component(rhs)
    , _frame(rhs._frame)
    , _inputGate(rhs._inputGate)
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
    _superJumpCharge = 0.f;
    _inputGate = {};

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

    float rawMoveX = 0.f;
    float rawMoveY = 0.f;

    if (INPUT->KeyPress(KEY_TYPE::W)) rawMoveY += 1.f;
    if (INPUT->KeyPress(KEY_TYPE::S)) rawMoveY -= 1.f;
    if (INPUT->KeyPress(KEY_TYPE::A)) rawMoveX -= 1.f;
    if (INPUT->KeyPress(KEY_TYPE::D)) rawMoveX += 1.f;

    const bool rawDashDown = INPUT->KeyDown(KEY_TYPE::SHIFT);

    const bool rawJumpDown = INPUT->KeyDown(KEY_TYPE::SPACE);

    const bool rawSkill1Down = INPUT->KeyDown(KEY_TYPE::KEY_1);
    const bool rawSkill2Down = INPUT->KeyDown(KEY_TYPE::KEY_2);

    const bool rawSkill1Press = INPUT->KeyPress(KEY_TYPE::KEY_1);
    const bool rawSkill2Press = INPUT->KeyPress(KEY_TYPE::KEY_2);

    const bool rawCtrlPress = INPUT->KeyPress(KEY_TYPE::LCTRL);
    const bool rawCtrlUp = INPUT->KeyUp(KEY_TYPE::LCTRL);

    const bool rawJumpDash = INPUT->KeyDown(KEY_TYPE::SHIFT);

    const bool rawAttackDown = INPUT->KeyDown(KEY_TYPE::LBUTTON);

    // 무기 교체
    const bool rawToggleWeaponDown = INPUT->KeyDown(KEY_TYPE::TAB);

    Vec2 rawMouseDelta = INPUT->GetMouseDelta();

    if (_inputGate.allowSuperJump && rawCtrlPress)
    {
        _superJumpCharge = ::clamp(_superJumpCharge + timeDelta, 0.f, MAX_JUMP_CHARGE);
    }
    else if (!_inputGate.allowSuperJump || !rawCtrlPress)
    {
        // Ctrl을 떼었거나 gate에서 super jump를 막고 있으면 charge를 초기화
        if (!rawCtrlUp)
        {
            _superJumpCharge = 0.f;
        }
    }

    // gate 적용 후 frame에 반영
    if (_inputGate.allowMove)
    {
        _frame.moveX = rawMoveX;
        _frame.moveY = rawMoveY;
    }

    if (_inputGate.allowLook)
    {
        _frame.lookYaw = rawMouseDelta.x;
        _frame.lookPitch = rawMouseDelta.y;
    }

    if (_inputGate.allowDash)
    {
        _frame.dashDown = rawDashDown;
    }

    if (_inputGate.allowJump)
    {
        _frame.jumpDown = rawJumpDown;
    }

    if (_inputGate.allowSkill)
    {
        _frame.useSkillDown[0] = rawSkill1Down;
        _frame.useSkillDown[1] = rawSkill2Down;

        _frame.useSkillPress[0] = rawSkill1Press;
        _frame.useSkillPress[1] = rawSkill2Press;
    }

    if (_inputGate.allowSuperJump)
    {
        _frame.superJumpPress = rawCtrlPress;
        _frame.superJumpUp = rawCtrlUp;
        _frame.superJumpCharge = _superJumpCharge;
    }
    else
    {
        _frame.superJumpPress = false;
        _frame.superJumpUp = false;
        _frame.superJumpCharge = 0.f;
    }

    if (_inputGate.allowJumpDash)
    {
        _frame.jumpDash = rawJumpDash;
    }

    if (_inputGate.allowAttack)
    {
        _frame.attackDown = rawAttackDown;
    }

    if (_inputGate.allowWeaponToggle)
    {
        _frame.toggleWeaponDown = rawToggleWeaponDown;
    }
}

void InputComponent::Reset_FrameInput()
{
    _frame.jumpDown = false;
    _frame.dashDown = false;
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

void InputComponent::Set_InputMode(EPlayerInputMode mode)
{
    _inputMode = mode;
    _inputGate = Get_InputGate_Preset(mode);
}

InputComponent::FInputGate InputComponent::Get_InputGate_Preset(EPlayerInputMode mode)
{
    FInputGate gate{};

    switch (mode)
    {
    case EPlayerInputMode::Normal:
        gate.allowMove = true;
        gate.allowLook = true;
        gate.allowDash = true;
        gate.allowJump = true;
        gate.allowSuperJump = true;
        gate.allowSkill = true;
        gate.allowAttack = true;
        gate.allowWeaponToggle = true;
        break;

    case EPlayerInputMode::LookOnly:
        gate.allowMove = false;
        gate.allowLook = true;
        gate.allowDash = false;
        gate.allowJump = false;
        gate.allowSuperJump = false;
        gate.allowSkill = true;
        gate.allowAttack = true;
        gate.allowWeaponToggle = false;
        break;

    case EPlayerInputMode::MoveAndLook:
        gate.allowMove = true;
        gate.allowLook = true;
        gate.allowDash = false;
        gate.allowJump = false;
        gate.allowSuperJump = false;
        gate.allowSkill = false;
        gate.allowAttack = true;
        gate.allowWeaponToggle = true;
        break;

    case EPlayerInputMode::BlockAll:
        gate.allowMove = false;
        gate.allowLook = false;
        gate.allowDash = false;
        gate.allowJump = false;
        gate.allowSuperJump = false;
        gate.allowSkill = false;
        gate.allowAttack = false;
        gate.allowWeaponToggle = false;
        break;

    default:
        break;
    }

    return gate;
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
