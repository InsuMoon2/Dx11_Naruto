#pragma once

#include "IPlayerState.h"

NS_BEGIN(Engine)
class Character;
NS_END

NS_BEGIN(Client)

class PlayerState_Skill : public IPlayerState
{
public:
    // 스킬 내부 진행 단계
    enum class ESkillSubPhase
    {
        Charging,  // Start -> Loop 차징 중
        Dashing,    // End (DashLoop) : 돌진 중
        Attacking,  // AttackEnd : 공격 마무리 애니메이션 따로 재생

        END
    };

public:
    PlayerState_Skill();
    ~PlayerState_Skill() override = default;

public:
    void Enter(PlayerStateMachine* state) override;
    void Update(PlayerStateMachine* state, float timeDelta) override;
    void Exit(PlayerStateMachine* state) override;

    EPlayerState Get_StateID() const override { return _myStateId; }

private:
    void Update_Charging(PlayerStateMachine* state, float timeDelta);
    void Update_Dashing(PlayerStateMachine* state, float timeDelta);
    void Update_Attacking(PlayerStateMachine* state, float timeDelta);

    // 돌진 시작
    void Begin_DashPhase(PlayerStateMachine* state);

    // 돌진 종료 시
    void Begin_AttackPhase(PlayerStateMachine* state);

    // 대쉬 중 타겟 세팅
    void Find_DashTarget(PlayerStateMachine* state);

private:
    EPlayerState _myStateId = EPlayerState::END; // 스킬별로 ID 타입 세팅해주기
    int32        _mySkill_Id = 0;

    float        _channelingTimer = 0.f;         // 루프를 도는 지속시간 체크용
    bool         _isEnding = false;

private:
    ESkillSubPhase _subPhase = ESkillSubPhase::Charging;

    Vec3    _dashStartPos = Vec3::Zero;
    Vec3    _dashDirection = Vec3::Forward;

    Weak<Character> _dashTarget;

public:
    static Shared<PlayerState_Skill> Create(int32 skill_Id, EPlayerState stateId);
};

NS_END
