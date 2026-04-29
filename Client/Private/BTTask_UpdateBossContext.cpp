#include "pch.h"
#include "BTTask_UpdateBossContext.h"

#include "Blackboard.h"
#include "GameObject.h"
#include "Transform.h"
#include "MovementComponent.h"
#include "Utils.h"

IMPLEMENT_REFLECTION(BTTask_UpdateBossContext)

bool BTTask_UpdateBossContext::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_UpdateBossContext";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Target Is Airborne Key", "target_is_airborne_key", _targetIsAirborneKey);
    PROPERTY_STRING_JSON("Target Distance Key", "target_distance_key", _targetDistanceKey);
    PROPERTY_STRING_JSON("Target Height Delta Key", "target_height_delta_key", _targetHeightDeltaKey);
    PROPERTY_STRING_JSON("Target Airborne Time Key", "target_airborne_time_key", _targetAirborneTimeKey);
    PROPERTY_STRING_JSON("Should Air Approach Key", "should_air_approach_key", _shouldAirApproachKey);
    PROPERTY_STRING_JSON("Should Wait Landing Key", "should_wait_landing_key", _shouldWaitLandingKey);
    PROPERTY_STRING_JSON("Should Ground Chase Key", "should_ground_chase_key", _shouldGroundChaseKey);
    PROPERTY_STRING_JSON("Can Use Aerial Attack Key", "can_use_aerial_attack_key", _canUseAerialAttackKey);
    PROPERTY_STRING_JSON("Should Use Shinra Key", "should_use_shinra_key", _shouldUseShinraKey);
    PROPERTY_STRING_JSON("Should Use Bansho Key", "should_use_bansho_key", _shouldUseBanshoKey);
    PROPERTY_STRING_JSON("Should Melee Combo Key", "should_melee_combo_key", _shouldMeleeComboKey);
    PROPERTY_STRING_JSON("Should Retreat Key", "should_retreat_key", _shouldRetreatKey);
    PROPERTY_STRING_JSON("Should Strafe Key", "should_strafe_key", _shouldStrafeKey);
    PROPERTY_STRING_JSON("Global Skill Cooldown Key", "global_skill_cooldown_key", _globalSkillCooldownKey);
    PROPERTY_STRING_JSON("Pain Force Skill Cycle Cooldown Key", "pain_force_skill_cycle_cooldown_key", _painForceSkillCycleCooldownKey);
    PROPERTY_STRING_JSON("Shinra Cooldown Remain Key", "shinra_cooldown_remain_key", _shinraCooldownRemainKey);
    PROPERTY_STRING_JSON("Bansho Cooldown Remain Key", "bansho_cooldown_remain_key", _banshoCooldownRemainKey);

    PROPERTY_FLOAT_JSON("Airborne Height Threshold", "airborne_height_threshold", _airborneHeightThreshold, 0.1f, 10.f);
    PROPERTY_FLOAT_JSON("Air Approach Min Airborne Time", "air_approach_min_airborne_time", _airApproachMinAirborneTime, 0.0f, 5.f);
    PROPERTY_FLOAT_JSON("Air Approach Min Distance", "air_approach_min_distance", _airApproachMinDistance, 0.1f, 30.f);
    PROPERTY_FLOAT_JSON("Air Approach Max Distance", "air_approach_max_distance", _airApproachMaxDistance, 0.1f, 50.f);
    PROPERTY_FLOAT_JSON("Air Approach Min Height Delta", "air_approach_min_height_delta", _airApproachMinHeightDelta, 0.1f, 10.f);
    PROPERTY_FLOAT_JSON("Air Approach Cooldown", "air_approach_cooldown", _airApproachCooldown, 0.0f, 10.f);
    PROPERTY_FLOAT_JSON("Wait Landing Max Airborne Time", "wait_landing_max_airborne_time", _waitLandingMaxAirborneTime, 0.0f, 5.f);
    PROPERTY_FLOAT_JSON("Retreat Range", "retreat_range", _retreatRange, 0.1f, 10.f);
    PROPERTY_FLOAT_JSON("Melee Combo Range", "melee_combo_range", _meleeComboRange, 0.1f, 10.f);
    PROPERTY_FLOAT_JSON("Shinra Range", "shinra_range", _shinraRange, 0.1f, 20.f);
    PROPERTY_FLOAT_JSON("Bansho Min Range", "bansho_min_range", _banshoMinRange, 0.1f, 30.f);
    PROPERTY_FLOAT_JSON("Bansho Max Range", "bansho_max_range", _banshoMaxRange, 0.1f, 50.f);
    PROPERTY_FLOAT_JSON("Strafe Min Range", "strafe_min_range", _strafeMinRange, 0.1f, 20.f);
    PROPERTY_FLOAT_JSON("Strafe Max Range", "strafe_max_range", _strafeMaxRange, 0.1f, 30.f);
    PROPERTY_FLOAT_JSON("Shinra Chance", "shinra_chance", _shinraChance, 0.f, 1.f);
    PROPERTY_FLOAT_JSON("Bansho Chance", "bansho_chance", _banshoChance, 0.f, 1.f);
    PROPERTY_FLOAT_JSON("Retreat Chance", "retreat_chance", _retreatChance, 0.f, 1.f);
    PROPERTY_FLOAT_JSON("Strafe Chance", "strafe_chance", _strafeChance, 0.f, 1.f);
    PROPERTY_FLOAT_JSON("Decision Refresh Interval", "decision_refresh_interval", _decisionRefreshInterval, 0.05f, 5.f);

    PROPERTY_STRING_JSON("Should Use Chibaku Tensei Key", "should_use_chibaku_tensei_key", _shouldUseChibakuTenseiKey);
    PROPERTY_STRING_JSON("Chibaku Cooldown Remain Key", "chibaku_cooldown_remain_key", _chibakuCooldownRemainKey);

    PROPERTY_FLOAT_JSON("Chibaku Min Range", "chibaku_min_range", _chibakuMinRange, 0.1f, 30.f);
    PROPERTY_FLOAT_JSON("Chibaku Max Range", "chibaku_max_range", _chibakuMaxRange, 0.1f, 50.f);
    PROPERTY_FLOAT_JSON("Chibaku Chance", "chibaku_chance", _chibakuChance, 0.f, 1.f);

    return true;
}

BTTask_UpdateBossContext::BTTask_UpdateBossContext()
{
}

BTTask_UpdateBossContext::BTTask_UpdateBossContext(const BTTask_UpdateBossContext& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _targetIsAirborneKey(rhs._targetIsAirborneKey)
    , _targetDistanceKey(rhs._targetDistanceKey)
    , _targetHeightDeltaKey(rhs._targetHeightDeltaKey)
    , _targetAirborneTimeKey(rhs._targetAirborneTimeKey)
    , _shouldAirApproachKey(rhs._shouldAirApproachKey)
    , _shouldWaitLandingKey(rhs._shouldWaitLandingKey)
    , _shouldGroundChaseKey(rhs._shouldGroundChaseKey)
    , _canUseAerialAttackKey(rhs._canUseAerialAttackKey)
    , _shouldUseShinraKey(rhs._shouldUseShinraKey)
    , _shouldUseBanshoKey(rhs._shouldUseBanshoKey)
    , _shouldMeleeComboKey(rhs._shouldMeleeComboKey)
    , _shouldRetreatKey(rhs._shouldRetreatKey)
    , _shouldStrafeKey(rhs._shouldStrafeKey)
    , _globalSkillCooldownKey(rhs._globalSkillCooldownKey)
    , _painForceSkillCycleCooldownKey(rhs._painForceSkillCycleCooldownKey)
    , _shinraCooldownRemainKey(rhs._shinraCooldownRemainKey)
    , _banshoCooldownRemainKey(rhs._banshoCooldownRemainKey)
    , _airborneHeightThreshold(rhs._airborneHeightThreshold)
    , _airApproachMinAirborneTime(rhs._airApproachMinAirborneTime)
    , _airApproachMinDistance(rhs._airApproachMinDistance)
    , _airApproachMaxDistance(rhs._airApproachMaxDistance)
    , _airApproachMinHeightDelta(rhs._airApproachMinHeightDelta)
    , _airApproachCooldown(rhs._airApproachCooldown)
    , _waitLandingMaxAirborneTime(rhs._waitLandingMaxAirborneTime)
    , _retreatRange(rhs._retreatRange)
    , _meleeComboRange(rhs._meleeComboRange)
    , _shinraRange(rhs._shinraRange)
    , _banshoMinRange(rhs._banshoMinRange)
    , _banshoMaxRange(rhs._banshoMaxRange)
    , _strafeMinRange(rhs._strafeMinRange)
    , _strafeMaxRange(rhs._strafeMaxRange)
    , _shinraChance(rhs._shinraChance)
    , _banshoChance(rhs._banshoChance)
    , _retreatChance(rhs._retreatChance)
    , _strafeChance(rhs._strafeChance)
    , _decisionRefreshInterval(rhs._decisionRefreshInterval)
    , _shouldUseChibakuTenseiKey(rhs._shouldUseChibakuTenseiKey)
    , _chibakuCooldownRemainKey(rhs._chibakuCooldownRemainKey)
    , _chibakuMinRange(rhs._chibakuMinRange)
    , _chibakuMaxRange(rhs._chibakuMaxRange)
    , _chibakuChance(rhs._chibakuChance)
{
}

void BTTask_UpdateBossContext::Initialize()
{
    BTTask::Initialize();

    //_targetAirborneTime = 0.f;
    //_airApproachCooldownRemain = 0.f;
    _decisionRefreshElapsed = 0.f;
    _shinraDecisionRoll = 1.f;
    _banshoDecisionRoll = 1.f;
    _retreatDecisionRoll = 1.f;
    _strafeDecisionRoll = 1.f;
    _painForceSkillCycleIndex = 0;
    _latchedPainForceSkillIndex = -1;
    _painForceSkillSelectionLatched = false;
}

EBTNodeResult BTTask_UpdateBossContext::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
        return _lastResult = EBTNodeResult::Failed;

    auto targetObject = blackboard->Get_ValueAsObject(_targetObjectKey);
    if (!targetObject || targetObject->Is_Destroy())
        return _lastResult = EBTNodeResult::Failed;

    auto ownerTransform = owner->Get_Component<Transform>();
    auto targetTransform = targetObject->Get_Component<Transform>();
    auto targetMovement = targetObject->Get_Component<MovementComponent>();

    if (!ownerTransform || !targetTransform)
        return _lastResult = EBTNodeResult::Failed;

    if (_airApproachCooldownRemain > 0.f)
        _airApproachCooldownRemain = max(0.f, _airApproachCooldownRemain - timeDelta);

    if (_decisionRefreshElapsed > 0.f)
        _decisionRefreshElapsed = max(0.f, _decisionRefreshElapsed - timeDelta);

    if (_decisionRefreshElapsed <= 0.f)
    {
        _shinraDecisionRoll = Utils::RandomRange(0.f, 1.f);
        _banshoDecisionRoll = Utils::RandomRange(0.f, 1.f);
        _retreatDecisionRoll = Utils::RandomRange(0.f, 1.f);
        _strafeDecisionRoll = Utils::RandomRange(0.f, 1.f);

        _chibakuDecisionRoll = Utils::RandomRange(0.f, 1.f);

        _decisionRefreshElapsed = _decisionRefreshInterval;
    }

    float globalSkillCooldown = blackboard->HasKey(_globalSkillCooldownKey)
        ? blackboard->Get_ValueAsFloat(_globalSkillCooldownKey)
        : 0.f;

    float painForceSkillCycleCooldown = blackboard->HasKey(_painForceSkillCycleCooldownKey)
        ? blackboard->Get_ValueAsFloat(_painForceSkillCycleCooldownKey)
        : 0.f;

    float shinraCooldown = blackboard->HasKey(_shinraCooldownRemainKey)
        ? blackboard->Get_ValueAsFloat(_shinraCooldownRemainKey)
        : 0.f;

    float banshoCooldown = blackboard->HasKey(_banshoCooldownRemainKey)
        ? blackboard->Get_ValueAsFloat(_banshoCooldownRemainKey)
        : 0.f;

    float chibakuCooldown = blackboard->HasKey(_chibakuCooldownRemainKey)
        ? blackboard->Get_ValueAsFloat(_chibakuCooldownRemainKey)
        : 0.f;

    globalSkillCooldown = max(0.f, globalSkillCooldown - timeDelta);
    painForceSkillCycleCooldown = max(0.f, painForceSkillCycleCooldown - timeDelta);
    shinraCooldown = max(0.f, shinraCooldown - timeDelta);
    banshoCooldown = max(0.f, banshoCooldown - timeDelta);
    chibakuCooldown = max(0.f, chibakuCooldown - timeDelta);

    blackboard->Set_ValueAsFloat(_globalSkillCooldownKey, globalSkillCooldown);
    blackboard->Set_ValueAsFloat(_painForceSkillCycleCooldownKey, painForceSkillCycleCooldown);
    blackboard->Set_ValueAsFloat(_shinraCooldownRemainKey, shinraCooldown);
    blackboard->Set_ValueAsFloat(_banshoCooldownRemainKey, banshoCooldown);

    blackboard->Set_ValueAsFloat(_chibakuCooldownRemainKey, chibakuCooldown);

    const Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    const Vec3 targetPos = targetTransform->Get_WorldPosition();

    Vec3 flatDelta = targetPos - ownerPos;
    flatDelta.y = 0.f;

    const float targetDistance = flatDelta.Length();
    const float targetHeightDelta = targetPos.y - ownerPos.y;

    bool targetIsAirborne = false;
    if (targetMovement)
        targetIsAirborne = !targetMovement->Is_OnGround();

    if (!targetIsAirborne && fabsf(targetHeightDelta) >= _airborneHeightThreshold)
        targetIsAirborne = true;

    if (targetIsAirborne)
        _targetAirborneTime += timeDelta;
    else
        _targetAirborneTime = 0.f;

    const bool airApproachCooldownReady = _airApproachCooldownRemain <= 0.f;

    const bool shouldAirApproach =
        targetIsAirborne &&
        _targetAirborneTime >= _airApproachMinAirborneTime &&
        targetDistance >= _airApproachMinDistance &&
        targetDistance <= _airApproachMaxDistance &&
        targetHeightDelta >= _airApproachMinHeightDelta &&
        airApproachCooldownReady;

    const bool shouldWaitLanding =
        targetIsAirborne &&
        !shouldAirApproach &&
        _targetAirborneTime <= _waitLandingMaxAirborneTime &&
        targetDistance <= _airApproachMaxDistance;

    bool shouldGroundChase =
        !shouldAirApproach &&
        !shouldWaitLanding;

    const bool canUseGlobalSkill = globalSkillCooldown <= 0.f;
    const bool canUsePainForceSkillCycle = painForceSkillCycleCooldown <= 0.f;
    const bool canUseShinra = canUseGlobalSkill && canUsePainForceSkillCycle && shinraCooldown <= 0.f;
    const bool canUseBansho = canUseGlobalSkill && canUsePainForceSkillCycle && banshoCooldown <= 0.f;

    const bool canUseChibaku = canUseGlobalSkill && chibakuCooldown <= 0.f;

    if (_painForceSkillSelectionLatched && globalSkillCooldown > 0.f)
    {
        _painForceSkillSelectionLatched = false;

        if (_latchedPainForceSkillIndex >= 0)
            _painForceSkillCycleIndex = (_latchedPainForceSkillIndex + 1) % 3;

        _latchedPainForceSkillIndex = -1;
    }

    bool shouldUseShinra = false;
    bool shouldUseBansho = false;

    bool shouldUseChibakuTensei = false;

    bool shouldMeleeCombo = false;
    bool shouldRetreat = false;
    bool shouldStrafe = false;

    if (!shouldAirApproach && !shouldWaitLanding)
    {
        if (targetDistance <= _retreatRange && _retreatDecisionRoll < _retreatChance)
        {
            shouldRetreat = true;
            shouldGroundChase = false;
        }

        else if (targetDistance <= _meleeComboRange)
        {
            shouldMeleeCombo = true;
            shouldGroundChase = false;
        }

        else if (targetDistance >= _chibakuMinRange && targetDistance <= _chibakuMaxRange)
        {
            shouldUseChibakuTensei =
                canUseChibaku &&
                _chibakuDecisionRoll < _chibakuChance;

            shouldUseBansho =
                !shouldUseChibakuTensei &&
                canUseBansho &&
                targetDistance >= _banshoMinRange &&
                targetDistance <= _banshoMaxRange &&
                _banshoDecisionRoll < _banshoChance;

            shouldStrafe =
                !shouldUseChibakuTensei &&
                !shouldUseBansho &&
                targetDistance >= _strafeMinRange &&
                targetDistance <= _strafeMaxRange &&
                _strafeDecisionRoll < _strafeChance;

            shouldGroundChase =
                !shouldUseChibakuTensei &&
                !shouldUseBansho &&
                !shouldStrafe;
        }
        else if (targetDistance <= _shinraRange)
        {
            shouldUseShinra = canUseShinra && _shinraDecisionRoll < _shinraChance;

            shouldStrafe =
                !shouldUseShinra &&
                targetDistance >= _strafeMinRange &&
                targetDistance <= _strafeMaxRange &&
                _strafeDecisionRoll < _strafeChance;

            shouldGroundChase = !shouldUseShinra && !shouldStrafe;
        }
        else if (targetDistance >= _banshoMinRange && targetDistance <= _banshoMaxRange)
        {
            shouldUseBansho = canUseBansho && _banshoDecisionRoll < _banshoChance;

            shouldStrafe =
                !shouldUseBansho &&
                targetDistance >= _strafeMinRange &&
                targetDistance <= _strafeMaxRange &&
                _strafeDecisionRoll < _strafeChance;

            shouldGroundChase = !shouldUseBansho && !shouldStrafe;
        }
        else
        {
            shouldGroundChase = true;
        }
    }

    if (!shouldAirApproach && !shouldWaitLanding && !shouldRetreat && !shouldMeleeCombo)
    {
        bool cycleSelected = false;
        int32 selectedSkillIndex = _latchedPainForceSkillIndex;

        const bool canUseShinraByRange = canUseShinra && targetDistance <= _shinraRange;
        const bool canUseBanshoByRange =
            canUseBansho &&
            targetDistance >= _banshoMinRange &&
            targetDistance <= _banshoMaxRange;
        const bool canUseChibakuByRange =
            canUseChibaku &&
            targetDistance >= _chibakuMinRange &&
            targetDistance <= _chibakuMaxRange;

        if (_painForceSkillSelectionLatched)
        {
            if (selectedSkillIndex == 0 && canUseShinraByRange)
            {
                shouldUseShinra = true;
                cycleSelected = true;
            }
            else if (selectedSkillIndex == 1 && canUseBanshoByRange)
            {
                shouldUseBansho = true;
                cycleSelected = true;
            }
            else if (selectedSkillIndex == 2 && canUseChibakuByRange)
            {
                shouldUseChibakuTensei = true;
                cycleSelected = true;
            }
        }

        if (!cycleSelected && canUseGlobalSkill)
        {
            for (int32 offset = 0; offset < 3; ++offset)
            {
                const int32 candidateIndex = (_painForceSkillCycleIndex + offset) % 3;

                if (candidateIndex == 0 && canUseShinraByRange)
                {
                    shouldUseShinra = true;
                    selectedSkillIndex = candidateIndex;
                    cycleSelected = true;
                    break;
                }

                if (candidateIndex == 1 && canUseBanshoByRange)
                {
                    shouldUseBansho = true;
                    selectedSkillIndex = candidateIndex;
                    cycleSelected = true;
                    break;
                }

                if (candidateIndex == 2 && canUseChibakuByRange)
                {
                    shouldUseChibakuTensei = true;
                    selectedSkillIndex = candidateIndex;
                    cycleSelected = true;
                    break;
                }
            }
        }

        if (cycleSelected)
        {
            shouldGroundChase = false;
            shouldStrafe = false;
            _painForceSkillSelectionLatched = true;
            _latchedPainForceSkillIndex = selectedSkillIndex;
        }
    }

    if (shouldAirApproach)
        _airApproachCooldownRemain = _airApproachCooldown;

    blackboard->Set_ValueAsBool(_targetIsAirborneKey, targetIsAirborne);
    blackboard->Set_ValueAsFloat(_targetDistanceKey, targetDistance);
    blackboard->Set_ValueAsFloat(_targetHeightDeltaKey, targetHeightDelta);
    blackboard->Set_ValueAsFloat(_targetAirborneTimeKey, _targetAirborneTime);

    blackboard->Set_ValueAsBool(_shouldAirApproachKey, shouldAirApproach);
    blackboard->Set_ValueAsBool(_shouldWaitLandingKey, shouldWaitLanding);
    blackboard->Set_ValueAsBool(_shouldGroundChaseKey, shouldGroundChase);

    blackboard->Set_ValueAsBool(_shouldUseShinraKey, shouldUseShinra);
    blackboard->Set_ValueAsBool(_shouldUseBanshoKey, shouldUseBansho);

    blackboard->Set_ValueAsBool(_shouldUseChibakuTenseiKey, shouldUseChibakuTensei);

    blackboard->Set_ValueAsBool(_shouldMeleeComboKey, shouldMeleeCombo);
    blackboard->Set_ValueAsBool(_shouldRetreatKey, shouldRetreat);
    blackboard->Set_ValueAsBool(_shouldStrafeKey, shouldStrafe);

    blackboard->Set_ValueAsBool(_canUseAerialAttackKey, shouldAirApproach);

    return _lastResult = EBTNodeResult::Succeeded;
}


Shared<BTTask_UpdateBossContext> BTTask_UpdateBossContext::Create()
{
    return make_shared<BTTask_UpdateBossContext>();
}

Shared<BTNode> BTTask_UpdateBossContext::Clone()
{
    auto clone = make_shared<BTTask_UpdateBossContext>(*this);
    clone->Initialize();

    return clone;
}
