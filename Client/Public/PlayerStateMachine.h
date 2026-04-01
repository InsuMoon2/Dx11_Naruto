#pragma once

#include "Component.h"
#include "IPlayerState.h"
#include "MovementComponent.h"

NS_BEGIN(Engine)
class Model;
NS_END

NS_BEGIN(Client)
class InputComponent;
class MovementComponent;
class AnimationStateComponent;

class PlayerStateMachine final : public Component
{
    GENERATED_COMPONENT(PlayerStateMachine, Protocol::COMPONENT_TYPE_PLAYER_STATE)

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

    void                        Set_ActiveSkillSlot(int32 slot) { _activeSkillSlot = slot; }
    int32                       Get_ActiveSkillSlot() const { return _activeSkillSlot; }

private:
    static string               To_AnimationStateName(EPlayerState stateID);


    bool                        Check_Global_Transitions();

    bool                        Check_Skill_Input();
    bool                        Check_WeaponToggle();

    // 추가 예정
    bool                        Check_Death();
    bool                        Check_Cinematic();
    bool                        Check_HitReaction(); // 슈퍼아머 아닐 때



private:
    Shared<InputComponent>                      _input;
    Shared<MovementComponent>                   _movement;
    Shared<AnimationStateComponent>             _animationState;

private:
    umap<EPlayerState, Shared<IPlayerState>>    _states;

    Shared<IPlayerState>                        _currentState = nullptr;
    EPlayerState                                _currentStateID = EPlayerState::END;
    EPlayerState                                _prevStateID = EPlayerState::END;

    float                                       _pendingSuperJumpVelocity = 0.f;
    Vec3                                        _pendingLandDirection = Vec3::Zero;

    EMoveInputDirection                         _pendingMoveInputDirection = EMoveInputDirection::Forward;

    int32                                       _activeSkillSlot = -1;

protected:
    json To_Json() const override;
    void From_Json(const json& data) override;

public:
    static Shared<PlayerStateMachine> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;

};


NS_END
