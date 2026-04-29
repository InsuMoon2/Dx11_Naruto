#pragma once

#include "BTTask.h"

NS_BEGIN(Client)

class BTTask_UpdateBossContext : public BTTask
{
    GENERATED_BT_REFLECTION(BTTask_UpdateBossContext)

public:
    explicit BTTask_UpdateBossContext();
    explicit BTTask_UpdateBossContext(const BTTask_UpdateBossContext& rhs);
    virtual ~BTTask_UpdateBossContext() = default;

public:
    void Initialize() override;
    EBTNodeResult Update(float timeDelta) override;

private:
    string _targetObjectKey = "TargetObjectKey";

    string _targetIsAirborneKey = "TargetIsAirborne";
    string _targetDistanceKey = "TargetDistance";

    string _targetHeightDeltaKey = "TargetHeightDelta";
    string _targetAirborneTimeKey = "TargetAirborneTime";

    string _shouldAirApproachKey = "ShouldAirApproach";
    string _shouldWaitLandingKey = "ShouldWaitLanding";

    string _shouldGroundChaseKey = "ShouldGroundChase";
    string _canUseAerialAttackKey = "CanUseAerialAttack";

    string _shouldUseShinraKey = "ShouldUseShinra"; 
    string _shouldUseBanshoKey = "ShouldUseBansho";

    string _shouldMeleeComboKey = "ShouldMeleeCombo"; 
    string _shouldRetreatKey = "ShouldRetreat";
    string _shouldStrafeKey = "ShouldStrafe"; 
    string _globalSkillCooldownKey = "BossSkillGlobalCooldownRemain"; 
    string _painForceSkillCycleCooldownKey = "PainForceSkillCycleCooldownRemain"; // 신라천정/만상천인이 서로 번갈아 연속 발동되지 않도록 막는 공용 템포 쿨 키다.
    string _shinraCooldownRemainKey = "ShinraTenseiCooldownRemain"; 
    string _banshoCooldownRemainKey = "BanshoTeninCooldownRemain";

    string _shouldUseChibakuTenseiKey = "ShouldUseChibakuTensei"; 
    string _chibakuCooldownRemainKey = "ChibakuTenseiCooldownRemain"; 


    float _airborneHeightThreshold = 1.2f;
    float _airApproachMinAirborneTime = 0.28f;

    float _airApproachMinDistance = 3.0f;
    float _airApproachMaxDistance = 10.0f;

    float _airApproachMinHeightDelta = 1.1f;
    float _airApproachCooldown = 2.0f;

    float _waitLandingMaxAirborneTime = 0.45f;

    float _retreatRange = 2.0f;     
    float _meleeComboRange = 2.8f;  
    float _shinraRange = 4.5f;      
    float _banshoMinRange = 6.0f;   
    float _banshoMaxRange = 14.0f;  
    float _strafeMinRange = 2.5f;   
    float _strafeMaxRange = 6.0f;   
    float _shinraChance = 0.18f;    
    float _banshoChance = 0.45f;    
    float _retreatChance = 0.22f; // 너무 붙었을 때 바로 맞딜만 하지 않고 한 템포 뒤로 빠질 확률이다.
    float _strafeChance = 0.25f;    
    float _decisionRefreshInterval = 0.35f; 

    float _targetAirborneTime = 0.f;
    float _airApproachCooldownRemain = 0.f;
    float _decisionRefreshElapsed = 0.f; 
    float _shinraDecisionRoll = 1.f; 
    float _banshoDecisionRoll = 1.f;
    float _retreatDecisionRoll = 1.f; // 근거리 백스탭 여부를 결정할 때 사용하는 랜덤 롤 값이다.
    float _strafeDecisionRoll = 1.f;

    float _chibakuMinRange = 3.0f;
    float _chibakuMaxRange = 8.0f;
    float _chibakuChance = 0.20f; 

    float _chibakuDecisionRoll = 1.f;
    int32 _painForceSkillCycleIndex = 0; // 페인 강제 스킬을 신라천정 -> 만상천인 -> 지폭천성 순으로 반복시키기 위한 다음 선택 인덱스다.
    int32 _latchedPainForceSkillIndex = -1; // 현재 프레임들에서 이미 선택해 둔 스킬 인덱스를 보관해 BT가 실제 스킬 노드로 진입할 때까지 선택이 흔들리지 않게 한다.
    bool _painForceSkillSelectionLatched = false; // 스킬 선택이 확정된 뒤 실제 글로벌 쿨다운이 걸릴 때까지 같은 스킬 결정을 유지하기 위한 래치 상태다.

public:
    static Shared<BTTask_UpdateBossContext> Create();
    Shared<BTNode> Clone() override;
};

NS_END
