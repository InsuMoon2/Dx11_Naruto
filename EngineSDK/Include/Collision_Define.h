#pragma once

#include "Engine_Enum.h"
#include "Engine_Macro.h"

NS_BEGIN(Engine)

// src 채널이 dst 마스크에 포함되고, dst 채널이 src 마스크에 포함되면 충돌이
// 가능하도록
inline bool Can_Collide(Collision_Channel src, uint32 srcMask,
                        Collision_Channel dst, uint32 dstMask) {
  uint32 srcIndex = ETOI(src);
  uint32 dstIndex = ETOI(dst);

  return (srcMask & dstIndex) && (dstMask & srcIndex);
}

struct FCollision_Preset_Data {
  Collision_Channel channel; // 자기 채널
  uint32 collisionMask;      // 충돌 대상 마스크
};

// 프리셋 테이블
static const FCollision_Preset_Data g_CollisionPresets[ETOI(
    Collision_Preset::END)] = {
    // Custom — 빈 값, 직접 설정
    {Collision_Channel::CHANNEL_NONE, ETOI(Collision_Channel::CHANNEL_NONE)},

    // Player — 플레이어 몸체: 몬스터 공격, 아이템, 트리거, 환경물, 투사체와
    // 충돌
    {Collision_Channel::Player, ETOI(Collision_Channel::Monster_Attack) |
                                    ETOI(Collision_Channel::Item) |
                                    ETOI(Collision_Channel::Trigger) |
                                    ETOI(Collision_Channel::Enviroment) |
                                    ETOI(Collision_Channel::Projectile)},

    // Player_Attack — 플레이어 공격: 몬스터 몸체와 충돌
    {Collision_Channel::Player_Attack,
     ETOI(Collision_Channel::Monster) | ETOI(Collision_Channel::Monster_Body)},

    // Monster — 몬스터 몸체: 플레이어 공격, 무기, 투사체와 충돌
    {Collision_Channel::Monster, ETOI(Collision_Channel::Player_Attack) |
                                     ETOI(Collision_Channel::Weapon) |
                                     ETOI(Collision_Channel::Projectile)},

    // Monster_Attack — 몬스터 공격: 플레이어와 충돌
    {Collision_Channel::Monster_Attack,
     ETOI(Collision_Channel::Player) | ETOI(Collision_Channel::Player_Body)},

    // Weapon — 무기: 몬스터와 충돌
    {Collision_Channel::Weapon,
     ETOI(Collision_Channel::Monster) | ETOI(Collision_Channel::Monster_Body)},

    // Projectile — 투사체: 몬스터, 환경물, 플레이어와 충돌
    {Collision_Channel::Projectile, ETOI(Collision_Channel::Monster) |
                                        ETOI(Collision_Channel::Enviroment) |
                                        ETOI(Collision_Channel::Player)},

    // Item — 아이템: 플레이어와만 충돌
    {Collision_Channel::Item, ETOI(Collision_Channel::Player)},

    // Trigger — 트리거: 플레이어와만 충돌
    {Collision_Channel::Trigger, ETOI(Collision_Channel::Player)},

    // Enviroment — 환경물: 플레이어, 투사체와 충돌
    {Collision_Channel::Enviroment,
     ETOI(Collision_Channel::Player) | ETOI(Collision_Channel::Projectile)},
};

inline const FCollision_Preset_Data &Get_PresetData(Collision_Preset preset) {
  uint32 index = ETOI(preset);

  if (index >= ETOI(Collision_Preset::END))
    index = 0;

  return g_CollisionPresets[index];
}

NS_END
