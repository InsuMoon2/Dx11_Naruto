#pragma once

#include "Engine_Enum.h"
#include "Engine_Macro.h"

NS_BEGIN(Engine)

// 충돌 채널
enum class Collision_Channel : uint32
{
    Player = (1 << 0),
    Player_Body = (1 << 1),
    Player_Attack = (1 << 2),
    Monster = (1 << 3),
    Monster_Body = (1 << 4),
    Monster_Attack = (1 << 5),
    Weapon = (1 << 6),
    Projectile = (1 << 7),
    Item = (1 << 8),
    Trigger = (1 << 9),
    Enviroment = (1 << 10),
    Player_Target = (1 << 11),


    CHANNEL_ALL = 0xFFFFFFFF,   // 모든 충돌 OK
    CHANNEL_NONE = 0x00000000,  // 모든 충돌 ㄴㄴ
};

// 충돌 프리셋
enum class Collision_Preset
{
    Custom,     // 직접 세팅

    Player, Player_Body, Player_Attack,
    Monster, Monster_Body, Monster_Attack,

    Weapon,
    Projectile, // 투사체

    Item,
    Trigger,
    Enviroment,

    Player_Target,

    END
};

enum class ECollisionResponse
{
    Ignore = 0,     // 충돌 무시
    Overlap = 1,    // 충돌 시 겹쳐짐, 이벤트만 발생
    Block = 2,      // 충돌 시 서로 밀어냄

    END
};

// blockMask에 있으면 Block, overlapMask에 있으면 overlap 둘다 아니면 Ignore 판정
inline ECollisionResponse Get_Collision_Response(uint32 selfOverlapMask, uint32 selfBlockMask, Collision_Channel targetChannel)
{
    uint32 targetBit = ETOI(targetChannel);

    if (selfBlockMask & targetBit)
        return ECollisionResponse::Block;

    if (selfOverlapMask & targetBit)
        return ECollisionResponse::Overlap;

    return ECollisionResponse::Ignore;
}

// 둘 중 하나라도 Ignore -> Ignore
// Ignore가 아니고 둘 중 하나라도 Overlap이면 Overlap,
// 둘 다 Block이면 최종적으로 Block 판정이 되도록
inline ECollisionResponse Calculate_ResponseResult(
    Collision_Channel selfType, uint32 selfOverlap, uint32 selfBlock,
    Collision_Channel otherType, uint32 otherOverlap, uint32 otherBlock)
{
    ECollisionResponse selfResponse = Get_Collision_Response(selfOverlap, selfBlock, otherType);
    ECollisionResponse otherResponse = Get_Collision_Response(otherOverlap, otherBlock, selfType);

    if (selfResponse == ECollisionResponse::Ignore || otherResponse == ECollisionResponse::Ignore)
        return ECollisionResponse::Ignore;

    if (selfResponse == ECollisionResponse::Overlap || otherResponse == ECollisionResponse::Overlap)
        return ECollisionResponse::Overlap;

    return ECollisionResponse::Block;
}

struct FCollision_Preset_Data
{
    Collision_Channel channel; // 자기 채널

    uint32 overlapMask;
    uint32 blockMask;

};

// 프리셋 테이블
static const FCollision_Preset_Data g_CollisionPresets[ETOI(
    Collision_Preset::END)] =
{
    // Custom — 빈 값, 직접 설정
    { Collision_Channel::CHANNEL_NONE, 0, 0 },

    // ── Player ──
    { Collision_Channel::Player,
    /* overlapMask */ ETOI(Collision_Channel::Monster_Attack) |
                      ETOI(Collision_Channel::Item) |
                      ETOI(Collision_Channel::Trigger) |
                      ETOI(Collision_Channel::Projectile),
    /* blockMask   */ ETOI(Collision_Channel::Enviroment)
    },

    { Collision_Channel::Player_Body,
    /* overlapMask */ ETOI(Collision_Channel::Monster_Attack) |
                      ETOI(Collision_Channel::Item) |
                      ETOI(Collision_Channel::Trigger) |
                      ETOI(Collision_Channel::Projectile),
    /* blockMask   */ ETOI(Collision_Channel::Monster_Body) |
                          ETOI(Collision_Channel::Enviroment)
    },
    
    { Collision_Channel::Player_Attack,
    /* overlapMask */ ETOI(Collision_Channel::Monster) |
                      ETOI(Collision_Channel::Monster_Body),
    /* blockMask   */ 0
    },
    
    { Collision_Channel::Monster,
    /* overlapMask */ ETOI(Collision_Channel::Player_Attack) |
                      ETOI(Collision_Channel::Weapon) |
                      ETOI(Collision_Channel::Projectile) |
                      ETOI(Collision_Channel::Player_Target),
    /* blockMask   */ 0
    },
    
    { Collision_Channel::Monster_Body,
    /* overlapMask */ ETOI(Collision_Channel::Player_Attack) |
                      ETOI(Collision_Channel::Weapon) |
                      ETOI(Collision_Channel::Projectile) |
                      ETOI(Collision_Channel::Player_Target),
    /* blockMask   */ ETOI(Collision_Channel::Player_Body) |
                      ETOI(Collision_Channel::Enviroment)
    },
    
    { Collision_Channel::Monster_Attack,
    /* overlapMask */ ETOI(Collision_Channel::Player) |
                      ETOI(Collision_Channel::Player_Body),
    /* blockMask   */ 0
    },
    
    { Collision_Channel::Weapon,
    /* overlapMask */ ETOI(Collision_Channel::Monster) |
                      ETOI(Collision_Channel::Monster_Body),
    /* blockMask   */ 0
    },
    
    { Collision_Channel::Projectile,
    /* overlapMask */ ETOI(Collision_Channel::Monster) |
                      ETOI(Collision_Channel::Player),
    /* blockMask   */ ETOI(Collision_Channel::Enviroment)
    },
    
    { Collision_Channel::Item,
    /* overlapMask */ ETOI(Collision_Channel::Player),
    /* blockMask   */ 0
    },
    
    { Collision_Channel::Trigger,
    /* overlapMask */ ETOI(Collision_Channel::Player),
    /* blockMask   */ 0
    },
    
    // Enviroment — 환경물. 플레이어/투사체를 Block
    { Collision_Channel::Enviroment,
    /* overlapMask */ 0,
    /* blockMask   */ ETOI(Collision_Channel::Player) |
                      ETOI(Collision_Channel::Player_Body) |
                      ETOI(Collision_Channel::Projectile) |
                      ETOI(Collision_Channel::Monster_Body)
    },
    
    // Player_Target — 타겟팅용. 몬스터/몬스터 몸체에 Overlap
    { Collision_Channel::Player_Target,
    /* overlapMask */ ETOI(Collision_Channel::Monster) |
                      ETOI(Collision_Channel::Monster_Body),
    /* blockMask   */ 0
    },
};

inline const FCollision_Preset_Data& Get_PresetData(Collision_Preset preset)
{
    uint32 index = ETOI(preset);

    if (index >= ETOI(Collision_Preset::END))
        index = 0;

    return g_CollisionPresets[index];
}

NS_END
