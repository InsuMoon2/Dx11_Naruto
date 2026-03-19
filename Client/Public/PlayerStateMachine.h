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
    EPlayerState                    Get_PrevStateID()    const { return _prevStateID; }
    Shared<InputComponent>          Get_Input()          const { return _input; }
    Shared<MovementComponent>       Get_Movement()       const { return _movement; }
    Shared<AnimationStateComponent> Get_AnimationState() const { return _animationState; }

public:
    bool    Play_AnimState(EPlayerState stateID);
    bool    Play_DirectionalAnimState(EPlayerState stateID, EMoveInputDirection dir);
    bool    Play_AnimStateLoopOnly(EPlayerState stateID);

    void    Request_AnimStateEnd();
    bool    Is_AnimStateFinished() const;
    bool    Is_AnimSequenceFinished() const;

    const FStateAnimationDesc* Find_AnimStateDesc(EPlayerState stateID) const;

    EAnimPhase      Get_AnimPhase() const;
    float           Get_AnimTrackPosition() const;
    float           Get_AnimDuration() const;

public:
    // 플레이어 강제 상태 변경
    void                        Force_Enter_State(EPlayerState stateID);

    void                        Set_PendingSuperJumpVelocity(float velocity) { _pendingSuperJumpVelocity = velocity; }
    void                        Set_PendingLandingDir(Vec3 dir) { _pendingLandDirection= dir; }

    float                       Get_PendingSuperJumpVeloicty() { return _pendingSuperJumpVelocity; }
    Vec3                        Get_PendingLandingDir() { return _pendingLandDirection; }

    void                        Set_PendingMoveInputDirection(EMoveInputDirection dir) { _pendingMoveInputDirection = dir; }
    EMoveInputDirection         Get_PendingMoveInputDirection() const { return _pendingMoveInputDirection; }

private:
    static string               To_AnimationStateName(EPlayerState stateID);

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

protected:
    json To_Json() const override;
    void From_Json(const json& data) override;

public:
    static Shared<PlayerStateMachine> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;

};


NS_END
