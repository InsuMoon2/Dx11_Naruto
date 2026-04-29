#pragma once

#include "AnimNotify.h"

NS_BEGIN(Engine)
class Transform;
NS_END

NS_BEGIN(Client)

class AN_SpawnAttack final : public AnimNotify
{
    GENERATED_BODY(AN_SpawnAttack)

public:
    string Get_TypeName() const override { return "AN_SpawnAttack"; }
    void   Execute(const FAnimNotifyContext& context) override;


private:
    Protocol::OBJECT_TYPE _spawnObjectType = Protocol::OBJECT_TYPE_SKILL_MONSTER_ATTACK;
    Collision_Preset _collisionPreset = Collision_Preset::Monster_Attack;

    string _boneName = "CharacterRoot";
    Vec3 _localOffset = Vec3(0.f, 1.0f, 1.5f);
    float _colliderRadius = 1.0f;

    float _lifetime = 0.5f;

    bool _useOwnerForward = true;
    bool _useHitReactionOverride = false;
    EHitReactionType _overrideHitReactionType = EHitReactionType::Default;
    bool _useLaunchOverride = false; 
    float _overrideLaunchPower = 0.f;
    float _overrideLaunchUp = 0.f; 
    bool _useHitSoundOverride = false; // SpawnAttack가 맞힌 대상의 피격음을 직접 지정할지 여부다.
    int32 _overrideHitSound = 0; // useHitSoundOverride가 켜졌을 때 FDamageEvent에 실어 보낼 정수 기반 피격 사운드 ID다.
    string _overrideHitSoundFile = ""; // useHitSoundOverride가 켜졌을 때 FDamageEvent에 실어 보낼 직접 피격 사운드 파일명이다.

    string _layerTag = "Layer_MonsterAttack";

private:
    static Matrix Calculate_SpawnBasisMatrix(const FAnimNotifyContext& context, Shared<Transform> transform, const string& boneName);
    static Vec3 Calculate_WorldSpawnPosition(const Matrix& basisMatrix, const Vec3& localOffset);
    static Vec3 Calculate_ForwardFromBasis(const Matrix& basisMatrix);
};

NS_END
