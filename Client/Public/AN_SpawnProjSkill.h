#pragma once

#include "AnimNotify.h"

NS_BEGIN(Engine)
class Transform;
NS_END

NS_BEGIN(Client)

class AN_SpawnProjSkill final : public AnimNotify
{
    GENERATED_BODY(AN_SpawnProjSkill)

public:
    string Get_TypeName() const override;
    void Execute(const FAnimNotifyContext& context) override;

private:
    Protocol::OBJECT_TYPE _spawnObjectType = Protocol::OBJECT_TYPE_SKILL_FIREBALL;
    Collision_Preset _collisionPreset = Collision_Preset::Projectile;
    Vec3 _localOffset = Vec3(0.f, 1.2f, 1.8f);
    bool _useOwnerForward = true;
    bool _aimAtTarget = false;
    bool _ignoreY = false;

private:
    static Vec3 Calculate_WorldSpawnPosition(Shared<Transform> transform, const Vec3& localOffset);
    static Vec3 Resolve_OwnerForward(Shared<Transform> ownerTransform, bool ignoreY);
    static Vec3 Resolve_TargetDirection(
        GameObject* owner,
        const Vec3& spawnPosition,
        bool ignoreY,
        const Vec3& fallbackDirection);
};

NS_END
