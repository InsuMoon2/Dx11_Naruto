#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_Attack : public IPlayerState
{
public:
    PlayerState_Attack();
    ~PlayerState_Attack() override = default;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return EPlayerState::Attack_1; }

public:
    // ANS_ComboWindow에서 세팅
    void Open_ComboWindow();
    void Close_ComboWindow();

    void Buffer_AttackInput();

    void Reset_Combo();

    int32 Get_ComboIndex() const { return _comboIndex; }

private:
    static constexpr int32 MAX_COMBO = 4;

    int32   _comboIndex = 0;
    bool    _comboWindowOpen = false;

    // 콤보 입력 가능 구간에서 입력이 들어왔는지 판단
    bool    _hasBufferedAttack = false;

    vector<EPlayerState> _comboAnimStates;

    // Close_ComboWindow에서 상태 전이용 - Enter에서 캐싱
    PlayerStateMachine* _cachedStateMachine = nullptr;

public:
    static Shared<PlayerState_Attack> Create();
};

NS_END
