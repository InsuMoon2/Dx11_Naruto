#pragma once

#include "Component.h"
#include "IPlayerState.h"
#include "MovementComponent.h"

NS_BEGIN(Engine)
class Model;
class MovementComponent;
NS_END

NS_BEGIN(Client)

DECLARE_DELEGATE(FOnPlayerStateChanged, EPlayerState /* Prev */, EPlayerState /* next */);

class InputComponent;
class AnimationStateComponent;

class PlayerStateMachine final : public Component
{
    GENERATED_COMPONENT(PlayerStateMachine, Protocol::COMPONENT_TYPE_PLAYER_STATE)

public:
    struct FPendingHitReaction
    {
        bool                active = false;
        EHitReactionType    type = EHitReactionType::Default;
        uint32              serial = 0;
        bool                forceRestart = false;
        string              hitAnimStateOverride = ""; // 스킬별 전용 피격 애니메이션을 재생해야 할 때 사용하는 상태명이다.
    };

public:
    explicit         PlayerStateMachine(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit         PlayerStateMachine(const PlayerStateMachine& rhs);
    virtual         ~PlayerStateMachine() = default;

public:
    HRESULT         Initialize_Prototype() override;
    HRESULT         Initialize(void* arg) override;
    void            BeginPlay() override;
    void            Update(float timeDelta);

public:
    void                        Register_State(EPlayerState stateID, Shared<IPlayerState> state);
    void                        Change_State(EPlayerState newState);
    [[nodiscard]]               MovementComponent::FMoveCommand Init_MoveCommand() const;

public:
    EPlayerState                    Get_CurrentStateID() const { return _currentStateID; }
    Shared<IPlayerState>            Get_CurrentState()   const { return _currentState; }
    EPlayerState                    Get_PrevStateID()    const { return _prevStateID; }
    Shared<InputComponent>          Get_Input()          const { return _input; }
    Shared<MovementComponent>       Get_Movement()       const { return _movement; }
    Shared<AnimationStateComponent> Get_AnimationState() const { return _animationState; }

public:
    bool            Play_AnimState(EPlayerState stateID);
    bool            Play_DirectionalAnimState(EPlayerState stateID, EMoveInputDirection dir);
    bool            Play_AnimStateLoopOnly(EPlayerState stateID);

    void            Request_AnimStateEnd();
    bool            Is_AnimStateFinished() const;
    bool            Is_AnimSequenceFinished() const;

    const FStateAnimationDesc* Find_AnimStateDesc(EPlayerState stateID);

    EAnimPhase      Get_AnimPhase() const;
    float           Get_AnimTrackPositionTicks() const;
    float           Get_AnimDurationTicks() const;
    float           Get_AnimTrackPositionSec() const;
    float           Get_AnimDurationSec() const;

    template<typename T>
    Shared<T> Get_State(EPlayerState stateID) const
    {
        auto it = _states.find(stateID);

        if (it == _states.end())
            return nullptr;
        return dynamic_pointer_cast<T>(it->second);
    }

public:
    // 플레이어 강제 상태 변경
    void                        Force_Enter_State(EPlayerState stateID);

    void                        Set_PendingSuperJumpVelocity(float velocity) { _pendingSuperJumpVelocity = velocity; }
    void                        Set_PendingLandingDir(Vec3 dir) { _pendingLandDirection= dir; }

    float                       Get_PendingSuperJumpVeloicty() { return _pendingSuperJumpVelocity; }
    Vec3                        Get_PendingLandingDir() { return _pendingLandDirection; }

    void                        Set_PendingMoveInputDirection(EMoveInputDirection dir) { _pendingMoveInputDirection = dir; }
    EMoveInputDirection         Get_PendingMoveInputDirection() const { return _pendingMoveInputDirection; }

    void                        Set_PendingDashWorldDirection(const Vec3& dir) { _pendingDashWorldDirection = dir; }
    Vec3                        Get_PendingDashWorldDirection() const { return _pendingDashWorldDirection; }

    void                        Set_ActiveSkillSlot(int32 slot) { _activeSkillSlot = slot; }
    int32                       Get_ActiveSkillSlot() const { return _activeSkillSlot; }

    bool                        Set_CameraRelativeMoveDirection(
                                    const Vec2& moveAxis,
                                    EMoveInputDirection& outDir,
                                    Vec3& outWorldDir) const;

    static string               To_AnimationStateName(EPlayerState stateID);

    void                        Trigger_HitReaction(EHitReactionType type, uint32 serial, bool forceRestart, const string& hitAnimStateOverride = "");

    const FPendingHitReaction&  Get_PendingHitReaction() const { return _pendingHitReaction; }
    void                        Consume_PendingHitReaction();


    // 상태 바뀔 때 이벤트 전파
    FOnPlayerStateChanged       OnStateChanged;

    // 근접해서 때릴지 판단
    bool Try_MeleeApproach(
        EPlayerState nextState,
        float approachRange = 7.f,
        float meleeRange = 1.45f);


public:
    void Set_ForceGroundAttack(bool val) { _forceGroundAttack = val; }
    bool Is_ForceGroundAttack() const    { return _forceGroundAttack; }


private:
    void                        Register_DefaultStates();

    bool                        Check_Global_Transitions();

    bool                        Check_Skill_Input();
    bool                        Check_WeaponToggle();

    // 추가 예정
    bool                        Check_Death();
    bool                        Check_Cinematic();
    bool                        Check_HitReaction(); // 슈퍼아머 아닐 때

    bool Check_Airbone_Input();

public:
    void Begin_WireLockOn();
    void End_WireLockOn();
    bool Is_WireLockOn() const { return _isWireLockOn;}

private:
    Shared<InputComponent>                      _input;
    Shared<MovementComponent>                   _movement;
    Shared<AnimationStateComponent>             _animationState;

    FPendingHitReaction                         _pendingHitReaction{};
    uint32                                      _lastConsumedReactionSerial = 0; // 소모된 히트리액션

private:
    umap<EPlayerState, Shared<IPlayerState>>    _states;

    Shared<IPlayerState>                        _currentState = nullptr;
    EPlayerState                                _currentStateID = EPlayerState::END;
    EPlayerState                                _prevStateID = EPlayerState::END;

    float                                       _pendingSuperJumpVelocity = 0.f;
    Vec3                                        _pendingLandDirection = Vec3::Zero;

    //bool                                        _pendingHitReaction = false;

    EMoveInputDirection                         _pendingMoveInputDirection = EMoveInputDirection::Forward;
    Vec3                                        _pendingDashWorldDirection = Vec3::Forward;

    int32                                       _activeSkillSlot = -1;

    bool _forceGroundAttack = false;
    bool _isWireLockOn = false;

protected:
    json To_Json() const override;
    void From_Json(const json& data) override;

public:
    static Shared<PlayerStateMachine> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;

};


NS_END
