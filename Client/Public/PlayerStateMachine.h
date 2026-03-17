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
    EPlayerState                Get_CurrentStateID() const { return _currentStateID; }
    EPlayerState                Get_PrevStateID()    const { return _prevStateID; }
    Shared<InputComponent>      Get_Input()          const { return _input; }
    Shared<MovementComponent>   Get_Movement()       const { return _movement; }
    Shared<Model>               Get_Model();

    vector<string>              Get_AvaiableAnimationNames();
    const FStateAnimationDesc*  Find_StateAnimation(EPlayerState stateID) const;
    FStateAnimationDesc&        Edit_StateAnimation(EPlayerState stateID);

    bool                        Apply_StateAnimation(EPlayerState stateID);

    bool                        Preview_StateAnimation(EPlayerState stateID, int32 sequenceSlot);

    // 플레이어 강제 상태 변경
    void                        Force_Enter_State(EPlayerState stateID);

private:
    Shared<InputComponent>                      _input;
    Shared<MovementComponent>                   _movement;
    Shared<Model>                               _model;

private:
    umap<EPlayerState, Shared<IPlayerState>>    _states;

    Shared<IPlayerState>                        _currentState = nullptr;
    EPlayerState                                _currentStateID = EPlayerState::END;
    EPlayerState                                _prevStateID = EPlayerState::END;

protected:
    umap<EPlayerState, FStateAnimationDesc>     _stateAnimations;

protected:
    json To_Json() const override;
    void From_Json(const json& data) override;

public:
    static Shared<PlayerStateMachine> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;

};


NS_END
