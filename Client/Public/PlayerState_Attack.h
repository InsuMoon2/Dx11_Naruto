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

    EPlayerState Get_StateID() const override { return EPlayerState::Attack; }

public:
    // ANS_ComboWindow에서 세팅
    void    Open_ComboWindow();
    void    Close_ComboWindow();

    void    Buffer_AttackInput();

    int32   Get_ComboIndex() const { return _comboIndex; }

    void    Reset_Combo();      // 콤보 초기화
    void    Advance_Combo();    // 다음 콤보로

    EAttackProfileType Get_ActiveProfileType() const { return _activeProfileType; }

    // 현재 콤보 반환용
    const FComboEntry* Get_CurrentComboEntry() const;

private:
    void    Select_Profile(PlayerStateMachine* state);
    void    Play_CurrentComboClip(PlayerStateMachine* state);

private:
    int32   _comboIndex = 0;
    bool    _comboWindowOpen = false;
    bool    _hasBufferedAttack = false; // 콤보 입력 가능 구간에서 입력이 들어왔는지 판단

private: // 콤보 프로파일
    const FComboProfile* _activeProfile = nullptr;
    EAttackProfileType   _activeProfileType = EAttackProfileType::Hand_Ground;

    // Close_ComboWindow에서 상태 전이용 - Enter에서 캐싱
    PlayerStateMachine* _cachedStateMachine = nullptr;

public:
    static Shared<PlayerState_Attack> Create();
};

NS_END
