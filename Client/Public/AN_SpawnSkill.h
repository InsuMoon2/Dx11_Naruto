#pragma once

#include "AnimNotify.h"

NS_BEGIN(Client)

class AN_SpawnSkill : public AnimNotify
{
    GENERATED_BODY(AN_SpawnSkill)

public:
    string Get_TypeName() const override;
    void   Execute(const FAnimNotifyContext& context) override;

public:
    Protocol::OBJECT_TYPE Get_SpawnObjectType() const { return _spawnObjectType; }
    void Set_SpawnObjectType(Protocol::OBJECT_TYPE type) { _spawnObjectType = type; }

private:
    Protocol::OBJECT_TYPE _spawnObjectType = Protocol::OBJECT_TYPE_SKILL_RASENSHURIKEN;

    Vec3    _localOffset = Vec3(0.f, 1.2f, 1.8f);

    // owner의 전방 발사 방향으로 사용할지?
    Collision_Preset _collisionPreset = Collision_Preset::Projectile;

    bool    _useOwnerForward = true;
    bool    _aimAtTarget = false;

private:
    // owner의 Transform 기준 로컬 오프셋을 월드 좌표로 변환해서 사용하게
    static Vec3 Calculate_WorldSpawnPosition(Shared<Transform> transform, const Vec3& localOffset);

};

NS_END
