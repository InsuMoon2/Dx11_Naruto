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
    float _strafeChance = 0.25f;    
    float _decisionRefreshInterval = 0.35f; 

    float _targetAirborneTime = 0.f;
    float _airApproachCooldownRemain = 0.f;
    float _decisionRefreshElapsed = 0.f; 
    float _shinraDecisionRoll = 1.f; 
    float _banshoDecisionRoll = 1.f;
    float _strafeDecisionRoll = 1.f;

    float _chibakuMinRange = 3.0f;
    float _chibakuMaxRange = 8.0f;
    float _chibakuChance = 0.20f; 

    float _chibakuDecisionRoll = 1.f;

public:
    static Shared<BTTask_UpdateBossContext> Create();
    Shared<BTNode> Clone() override;
};

NS_END
