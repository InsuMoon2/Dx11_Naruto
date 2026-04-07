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

    Vec3 _localOffset = Vec3(0.f, 1.0f, 1.5f);
    float _colliderRadius = 1.0f;

    float _lifetime = 0.5f;

    bool _useOwnerForward = true;

    string _layerTag = "Layer_MonsterAttack";

private:
    static Vec3 Calculate_WorldSpawnPosition(Shared<Transform> transform, const Vec3& localOffset);
};

NS_END
