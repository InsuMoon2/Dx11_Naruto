#pragma once

#include "Engine_Enum.h"
#include "Engine_Macro.h"

NS_BEGIN(Engine)

// 충돌 채널
enum class Collision_Channel : uint32
{
    Player                  = (1 << 0),
    Player_Body             = (1 << 1),
    Player_Attack           = (1 << 2),
    Monster                 = (1 << 3),
    Monster_Body            = (1 << 4),
    Monster_Attack          = (1 << 5),
    Weapon                  = (1 << 6),
    Projectile              = (1 << 7),
    Item                    = (1 << 8),
    Trigger                 = (1 << 9),
    Enviroment              = (1 << 10),
    Player_Target           = (1 << 11),


    CHANNEL_ALL = 0xFFFFFFFF,   // 모든 충돌 OK
    CHANNEL_NONE = 0x00000000,  // 모든 충돌 ㄴㄴ
};

// 충돌 프리셋
enum class Collision_Preset
{
    Custom,     // 직접 세팅

    Player,  Player_Body,  Player_Attack,
    Monster, Monster_Body, Monster_Attack,

    Weapon,
    Projectile, // 투사체

    Item,
    Trigger,
    Enviroment,

    Player_Target,

    END
};

// src 채널이 dst 마스크에 포함되고, dst 채널이 src 마스크에 포함되면 충돌이
// 가능하도록
inline bool Can_Collide(Collision_Channel src, uint32 srcMask,
    Collision_Channel dst, uint32 dstMask)
{
    uint32 srcIndex = ETOI(src);
    uint32 dstIndex = ETOI(dst);

    return (srcMask & dstIndex) && (dstMask & srcIndex);
}

inline bool Is_Blocking(Collision_Channel a, Collision_Channel b)
{
    if ((a == Collision_Channel::Player_Body && b == Collision_Channel::Monster_Body) ||
        (a == Collision_Channel::Monster_Body && b == Collision_Channel::Player_Body))
    {
        return true;
    }

    // 필요 시 여기에 Player_Body <-> Enviroment 등 조건 추가해야함
    return false;
}

struct FCollision_Preset_Data
{
    Collision_Channel channel; // 자기 채널
    uint32 collisionMask;      // 충돌 대상 마스크
};

// 프리셋 테이블
static const FCollision_Preset_Data g_CollisionPresets[ETOI(
    Collision_Preset::END)] =
{
    // Custom — 빈 값, 직접 설정
    {Collision_Channel::CHANNEL_NONE,   ETOI(Collision_Channel::CHANNEL_NONE)},

    // Player — 플레이어 몸체
    {Collision_Channel::Player,         ETOI(Collision_Channel::Monster_Attack) |
                                        ETOI(Collision_Channel::Item) |
                                        ETOI(Collision_Channel::Trigger) |
                                        ETOI(Collision_Channel::Enviroment) |
                                        ETOI(Collision_Channel::Projectile)},

    {Collision_Channel::Player_Body,
                                        ETOI(Collision_Channel::Monster_Attack) |
                                        ETOI(Collision_Channel::Item) |
                                        ETOI(Collision_Channel::Trigger) |
                                        ETOI(Collision_Channel::Enviroment) |
                                        ETOI(Collision_Channel::Projectile) |
                                        ETOI(Collision_Channel::Monster_Body)},

    // Player_Attack
    {Collision_Channel::Player_Attack,
                                        ETOI(Collision_Channel::Monster) |
                                        ETOI(Collision_Channel::Monster_Body)},

    // Monster — 몬스터 몸체
    {Collision_Channel::Monster,
                                        ETOI(Collision_Channel::Player_Attack) |
                                        ETOI(Collision_Channel::Weapon) |
                                        ETOI(Collision_Channel::Projectile) |
                                        ETOI(Collision_Channel::Player_Target)},

    {Collision_Channel::Monster_Body,
                                       ETOI(Collision_Channel::Player_Attack) |
                                        ETOI(Collision_Channel::Weapon) |
                                        ETOI(Collision_Channel::Projectile) |
                                        ETOI(Collision_Channel::Player_Target) |
                                        ETOI(Collision_Channel::Player_Body)},

    // Monster_Attack — 몬스터 공격
    {Collision_Channel::Monster_Attack,
                                        ETOI(Collision_Channel::Player) |
                                        ETOI(Collision_Channel::Player_Body)},

    // Weapon — 무기
    {Collision_Channel::Weapon,
                                        ETOI(Collision_Channel::Monster) |
                                        ETOI(Collision_Channel::Monster_Body)},

    // Projectile — 투사체
    {Collision_Channel::Projectile,
                                        ETOI(Collision_Channel::Monster) |
                                        ETOI(Collision_Channel::Enviroment) |
                                        ETOI(Collision_Channel::Player)},

    // Item — 아이템
    {Collision_Channel::Item,
                                        ETOI(Collision_Channel::Player)},

    // Trigger — 트리거
    {Collision_Channel::Trigger,
                                        ETOI(Collision_Channel::Player)},

    // Enviroment — 환경물: 플레이어, 투사체와 충돌
    {Collision_Channel::Enviroment,
                                        ETOI(Collision_Channel::Player) |
                                        ETOI(Collision_Channel::Projectile)},

    {Collision_Channel::Player_Target,
                                        ETOI(Collision_Channel::Monster) |
                                        ETOI(Collision_Channel::Monster_Body)},
};

inline const FCollision_Preset_Data& Get_PresetData(Collision_Preset preset)
{
    uint32 index = ETOI(preset);

    if (index >= ETOI(Collision_Preset::END))
        index = 0;

    return g_CollisionPresets[index];
}

NS_END
