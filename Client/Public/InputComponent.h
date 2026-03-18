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

        bool dashDown = false;

        bool jumpDown = false;
        bool superJumpPress = false;
        bool superJumpUp = false;
        float superJumpCharge = 0.f;

        bool useSkillDown[2] = { false, false };
    };

    // 입력 제어용
    struct FInputGate
    {
        bool allowMove = true;
        bool allowLook = true;
        bool allowDash = true;
        bool allowJump = true;
        bool allowSuperJump = true;
        bool allowSkill = true;

        void Disable_AllInput()
        {
            allowMove = false;
            allowLook = false;
            allowDash = false;
            allowJump = false;
            allowSuperJump = false;
            allowSkill = false;
        }

        void Enable_AllInput()
        {
            allowMove = true;
            allowLook = true;
            allowDash = true;
            allowJump = true;
            allowSuperJump = true;
            allowSkill = true;
        }
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
    const FInputFrame&  Get_Frame() const { return _frame; }
    bool                Has_MoveInput() const;
    Vec2                Get_MoveAxis() const;

    void                Set_InputGate(const FInputGate& gate) { _inputGate = gate; }
    void                Reset_InputGate() { _inputGate = {}; }

    void                Set_MoveInputEnabled(bool enabled) { _inputGate.allowMove = enabled; }
    void                Set_LookInputEnabled(bool enabled) { _inputGate.allowLook = enabled; }
    void                Set_DashInputEnabled(bool enabled) { _inputGate.allowDash = enabled; }
    void                Set_JumpInputEnabled(bool enabled) { _inputGate.allowJump = enabled; }
    void                Set_SuperJumpInputEnabled(bool enabled) { _inputGate.allowSuperJump = enabled; }
    void                Set_SkillInputEnabled(bool enabled) { _inputGate.allowSkill = enabled; }

    const FInputGate&   Get_InputGate() const { return _inputGate; }

public:
    void                Set_InputMode(EPlayerInputMode mode);
    EPlayerInputMode    Get_InputMode() const { return _inputMode; }

private:
    FInputGate          Get_InputGate_Preset(EPlayerInputMode mode);

protected:
    json To_Json() const override;
    void From_Json(const json& data) override;

private:
    static constexpr float MAX_JUMP_CHARGE = 3.f;

    FInputFrame      _frame;
    FInputGate       _inputGate;
    EPlayerInputMode _inputMode = EPlayerInputMode::Normal;

    float   _superJumpCharge = 0.f;


public:
    static Shared<InputComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;

};

NS_END
