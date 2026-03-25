#pragma once

#include "IPlayerState.h"

NS_BEGIN(Client)

class PlayerState_Skill : public IPlayerState
{
public:
    PlayerState_Skill();
    ~PlayerState_Skill() override = default;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return _myStateId; }

private:
    EPlayerState _myStateId = EPlayerState::END; // 스킬별로 ID 타입 세팅해주기
    int32        _mySkill_Id = 0;

    float        _channelingTimer = 0.f;        // 루프를 도는 지속시간 체크용
    bool         _isEnding = false;


public:
    static Shared<PlayerState_Skill> Create(int32 skill_Id);
};

NS_END
