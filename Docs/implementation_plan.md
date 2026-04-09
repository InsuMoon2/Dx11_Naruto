# 플레이어 상태 동기화 상세 가이드라인

현재 서버-클라이언트 간 애니메이션과 상태 동기화가 일부(이동, 점프)만 처리되는 현상을 해결하기 위한 상세한 구현 스크립트와 가이드라인입니다. 

사용자 규칙에 따라 요약이나 축약 없이, 실제 사용할 수 있는 완성본 형태(A to Z)의 코드를 보강된 문서로 다시 제시합니다.

---

## 1. Protobuf 정의 파일 갱신

먼저, `Client_Enum.h`에 명시된 모든 클라이언트 `EPlayerState`를 수용할 수 있도록 서버 스키마의 `OBJECT_STATE_TYPE`을 재설계해야 합니다. 아래 코드로 완전히 교체합니다.

#### [대체] [Enum.proto](file:///c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Server/Protobuf/Bin/Enum.proto) 코드를 다음과 같이 교체하세요.

```protobuf
syntax = "proto3";
package Protocol;

enum PlayerType
{
	PLAYER_TYPE_NONE = 0;
	PLAYER_TYPE_KNIGHT = 1;
	PLAYER_TYPE_MAGE = 2;
	PLAYER_TYPE_ARCHER = 3;
}

enum OBJECT_TYPE
{
	OBJECT_TYPE_NONE = 0;
	OBJECT_TYPE_PLAYER = 1;
	OBJECT_TYPE_MONSTER = 2;
}

enum DIR_TYPE
{
	DIR_TYPE_UP = 0;
	DIR_TYPE_DOWN = 1;
	DIR_TYPE_LEFT = 2;
	DIR_TYPE_RIGHT = 3;
}

// [추가] EPlayerState의 모든 목록을 Protocol 버퍼로 매핑
enum OBJECT_STATE_TYPE
{
	OBJECT_STATE_TYPE_IDLE = 0;
	OBJECT_STATE_TYPE_BIGSWORD_IDLE = 1;
	OBJECT_STATE_TYPE_WALL_IDLE = 2;

	OBJECT_STATE_TYPE_RUN = 3;
	OBJECT_STATE_TYPE_WALL_RUN = 4;

	OBJECT_STATE_TYPE_JUMP = 5;
	OBJECT_STATE_TYPE_JUMP_FALL = 6;
	OBJECT_STATE_TYPE_DOUBLE_JUMP = 7;
	OBJECT_STATE_TYPE_JUMP_DASH = 8;

	OBJECT_STATE_TYPE_SUPER_JUMP_CHARGE = 9;
	OBJECT_STATE_TYPE_SUPER_JUMP = 10;
	OBJECT_STATE_TYPE_HEIGHT_LAND = 11;

	OBJECT_STATE_TYPE_WIRE_DASH = 12;
	OBJECT_STATE_TYPE_AIR_APPROACH = 13;

	// Attack 진입 판별용
	OBJECT_STATE_TYPE_ATTACK = 14;
	OBJECT_STATE_TYPE_JUMP_ATTACK = 15;

	// 세부 공격 콤보 애니메이션 스테이트
	OBJECT_STATE_TYPE_ATTACK_01 = 16;
	OBJECT_STATE_TYPE_ATTACK_02 = 17;
	OBJECT_STATE_TYPE_ATTACK_03 = 18;
	OBJECT_STATE_TYPE_ATTACK_04 = 19;
	OBJECT_STATE_TYPE_ATTACK_AIR_01 = 20;
	OBJECT_STATE_TYPE_ATTACK_AIR_02 = 21;
	OBJECT_STATE_TYPE_ATTACK_AIR_03 = 22;
	OBJECT_STATE_TYPE_ATTACK_AIR_04 = 23;
	OBJECT_STATE_TYPE_ATTACK_SWORD_01 = 24;
	OBJECT_STATE_TYPE_ATTACK_SWORD_02 = 25;
	OBJECT_STATE_TYPE_ATTACK_SWORD_03 = 26;
	OBJECT_STATE_TYPE_ATTACK_SWORD_04 = 27;
	OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_01 = 28;
	OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_02 = 29;

	OBJECT_STATE_TYPE_HIT = 30;
	OBJECT_STATE_TYPE_DASH = 31;

	// 스킬 스테이트
	OBJECT_STATE_TYPE_SKILL_RASENGAN = 32;
	OBJECT_STATE_TYPE_SKILL_RASENGAN_AIR = 33;
	OBJECT_STATE_TYPE_SKILL_RASENGAN_END = 34;

	OBJECT_STATE_TYPE_SKILL_RASENSHURIKEN = 35;
	OBJECT_STATE_TYPE_SKILL_RASENSHURIKEN_AIR = 36;

	OBJECT_STATE_TYPE_SKILL_CHIDORI = 37;
	OBJECT_STATE_TYPE_SKILL_CHIDORI_AIR = 38;
	OBJECT_STATE_TYPE_SKILL_CHIDORI_END = 39;

	OBJECT_STATE_TYPE_SKILL_FIREBALL = 40;
	OBJECT_STATE_TYPE_SKILL_FIREBALL_AIR = 41;

	OBJECT_STATE_TYPE_DEAD = 42;
}
```

> **수행 방법**: 위 내용을 적용한 뒤, `Server/Protobuf/Bin` 폴더 등에 위치한 `GenPackets.bat` 혹은 `Update.bat` 류의 protoc 컴파일러 배치 파일을 다시 실행시켜 클라이언트 및 서버 쪽의 `Enum.pb.h`와 `Enum.pb.cc` 파일을 갱신해야 합니다.

---

## 2. AnimationStateComponent의 맵핑 함수 확장 구현

`Client/Private/AnimationStateComponent.cpp`에 존재하는 세 개의 함수 `To_ProtoState`, `From_ProtoState`, `Requires_ForceRestart` 코드를 다음과 같이 교체합니다.

#### [변경] [AnimationStateComponent.cpp](file:///c:/Users/moon/Desktop/Jusin/GitDesktop/Dx11_Naruto/Client/Private/AnimationStateComponent.cpp) 하단부 맵핑 함수 전체

```cpp
bool AnimationStateComponent::Requires_ForceRestart(EPlayerState state)
{
    // [추가] 네트워크상으로 패킷을 수신했을 때, 방향이나 콤보인덱스가 변하지 않았더라도 
    // 동일 상태로 진입 시 애니메이션이 무조건 처음부터 재시작되어야 하는(단발성 팡/쾅) 상태들을 이곳에 등록합니다.
    switch (state)
    {
    case EPlayerState::Jump:
    case EPlayerState::DoubleJump:
    case EPlayerState::SuperJump:
    case EPlayerState::HeightLand:
    case EPlayerState::Dash:
    case EPlayerState::WireDash:
    case EPlayerState::AirApproach:
    case EPlayerState::Hit:
    // 스킬 관련
    case EPlayerState::Skill_Rasengan:
    case EPlayerState::Skill_Rasengan_Air:
    case EPlayerState::Skill_RasenShuriken:
    case EPlayerState::Skill_RasenShuriken_Air:
    case EPlayerState::Skill_Chidori:
    case EPlayerState::Skill_Chidori_Air:
    case EPlayerState::Skill_FireBall:
    case EPlayerState::Skill_FireBall_Air:
        return true;
    default:
        return false;
    }
}

Protocol::OBJECT_STATE_TYPE AnimationStateComponent::To_ProtoState(EPlayerState state)
{
    // [추가] 로컬의 EPlayerState를 네트워크용 프로토콜 Enum으로 변환합니다. 모든 상태를 누락없이 매핑합니다.
    switch (state)
    {
    case EPlayerState::Idle:                return Protocol::OBJECT_STATE_TYPE_IDLE;
    case EPlayerState::BigSword_Idle:       return Protocol::OBJECT_STATE_TYPE_BIGSWORD_IDLE;
    case EPlayerState::Wall_Idle:           return Protocol::OBJECT_STATE_TYPE_WALL_IDLE;
    case EPlayerState::Run:                 return Protocol::OBJECT_STATE_TYPE_RUN;
    case EPlayerState::Wall_Run:            return Protocol::OBJECT_STATE_TYPE_WALL_RUN;
    case EPlayerState::Jump:                return Protocol::OBJECT_STATE_TYPE_JUMP;
    case EPlayerState::JumpFall:            return Protocol::OBJECT_STATE_TYPE_JUMP_FALL;
    case EPlayerState::DoubleJump:          return Protocol::OBJECT_STATE_TYPE_DOUBLE_JUMP;
    case EPlayerState::JumpDash:            return Protocol::OBJECT_STATE_TYPE_JUMP_DASH;
    case EPlayerState::SuperJumpCharge:     return Protocol::OBJECT_STATE_TYPE_SUPER_JUMP_CHARGE;
    case EPlayerState::SuperJump:           return Protocol::OBJECT_STATE_TYPE_SUPER_JUMP;
    case EPlayerState::HeightLand:          return Protocol::OBJECT_STATE_TYPE_HEIGHT_LAND;
    case EPlayerState::WireDash:            return Protocol::OBJECT_STATE_TYPE_WIRE_DASH;
    case EPlayerState::AirApproach:         return Protocol::OBJECT_STATE_TYPE_AIR_APPROACH;

    case EPlayerState::Attack:              return Protocol::OBJECT_STATE_TYPE_ATTACK;
    case EPlayerState::JumpAttack:          return Protocol::OBJECT_STATE_TYPE_JUMP_ATTACK;

    case EPlayerState::Attack_01:           return Protocol::OBJECT_STATE_TYPE_ATTACK_01;
    case EPlayerState::Attack_02:           return Protocol::OBJECT_STATE_TYPE_ATTACK_02;
    case EPlayerState::Attack_03:           return Protocol::OBJECT_STATE_TYPE_ATTACK_03;
    case EPlayerState::Attack_04:           return Protocol::OBJECT_STATE_TYPE_ATTACK_04;
    case EPlayerState::Attack_Air_01:       return Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_01;
    case EPlayerState::Attack_Air_02:       return Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_02;
    case EPlayerState::Attack_Air_03:       return Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_03;
    case EPlayerState::Attack_Air_04:       return Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_04;
    case EPlayerState::Attack_Sword_01:     return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_01;
    case EPlayerState::Attack_Sword_02:     return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_02;
    case EPlayerState::Attack_Sword_03:     return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_03;
    case EPlayerState::Attack_Sword_04:     return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_04;
    case EPlayerState::Attack_SwordAir_01:  return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_01;
    case EPlayerState::Attack_SwordAir_02:  return Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_02;

    case EPlayerState::Hit:                 return Protocol::OBJECT_STATE_TYPE_HIT;
    case EPlayerState::Dash:                return Protocol::OBJECT_STATE_TYPE_DASH;

    case EPlayerState::Skill_Rasengan:      return Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN;
    case EPlayerState::Skill_Rasengan_Air:  return Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN_AIR;
    case EPlayerState::Skill_Rasengan_End:  return Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN_END;
    case EPlayerState::Skill_RasenShuriken: return Protocol::OBJECT_STATE_TYPE_SKILL_RASENSHURIKEN;
    case EPlayerState::Skill_RasenShuriken_Air: return Protocol::OBJECT_STATE_TYPE_SKILL_RASENSHURIKEN_AIR;
    case EPlayerState::Skill_Chidori:       return Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI;
    case EPlayerState::Skill_Chidori_Air:   return Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI_AIR;
    case EPlayerState::Skill_Chidori_End:   return Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI_END;
    case EPlayerState::Skill_FireBall:      return Protocol::OBJECT_STATE_TYPE_SKILL_FIREBALL;
    case EPlayerState::Skill_FireBall_Air:  return Protocol::OBJECT_STATE_TYPE_SKILL_FIREBALL_AIR;

    case EPlayerState::Dead:                return Protocol::OBJECT_STATE_TYPE_DEAD;
    default:                                return Protocol::OBJECT_STATE_TYPE_IDLE;
    }
}

EPlayerState AnimationStateComponent::From_ProtoState(Protocol::OBJECT_STATE_TYPE state)
{
    // [추가] 네트워크상으로 온 Protocol Enum을 로컬의 EPlayerState로 변환합니다.
    switch (state)
    {
    case Protocol::OBJECT_STATE_TYPE_IDLE:                  return EPlayerState::Idle;
    case Protocol::OBJECT_STATE_TYPE_BIGSWORD_IDLE:         return EPlayerState::BigSword_Idle;
    case Protocol::OBJECT_STATE_TYPE_WALL_IDLE:             return EPlayerState::Wall_Idle;
    case Protocol::OBJECT_STATE_TYPE_RUN:                   return EPlayerState::Run;
    case Protocol::OBJECT_STATE_TYPE_WALL_RUN:              return EPlayerState::Wall_Run;
    case Protocol::OBJECT_STATE_TYPE_JUMP:                  return EPlayerState::Jump;
    case Protocol::OBJECT_STATE_TYPE_JUMP_FALL:             return EPlayerState::JumpFall;
    case Protocol::OBJECT_STATE_TYPE_DOUBLE_JUMP:           return EPlayerState::DoubleJump;
    case Protocol::OBJECT_STATE_TYPE_JUMP_DASH:             return EPlayerState::JumpDash;
    case Protocol::OBJECT_STATE_TYPE_SUPER_JUMP_CHARGE:     return EPlayerState::SuperJumpCharge;
    case Protocol::OBJECT_STATE_TYPE_SUPER_JUMP:            return EPlayerState::SuperJump;
    case Protocol::OBJECT_STATE_TYPE_HEIGHT_LAND:           return EPlayerState::HeightLand;
    case Protocol::OBJECT_STATE_TYPE_WIRE_DASH:             return EPlayerState::WireDash;
    case Protocol::OBJECT_STATE_TYPE_AIR_APPROACH:          return EPlayerState::AirApproach;
        
    case Protocol::OBJECT_STATE_TYPE_ATTACK:                return EPlayerState::Attack;
    case Protocol::OBJECT_STATE_TYPE_JUMP_ATTACK:           return EPlayerState::JumpAttack;
        
    case Protocol::OBJECT_STATE_TYPE_ATTACK_01:             return EPlayerState::Attack_01;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_02:             return EPlayerState::Attack_02;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_03:             return EPlayerState::Attack_03;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_04:             return EPlayerState::Attack_04;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_01:         return EPlayerState::Attack_Air_01;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_02:         return EPlayerState::Attack_Air_02;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_03:         return EPlayerState::Attack_Air_03;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_AIR_04:         return EPlayerState::Attack_Air_04;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_01:       return EPlayerState::Attack_Sword_01;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_02:       return EPlayerState::Attack_Sword_02;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_03:       return EPlayerState::Attack_Sword_03;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_04:       return EPlayerState::Attack_Sword_04;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_01:   return EPlayerState::Attack_SwordAir_01;
    case Protocol::OBJECT_STATE_TYPE_ATTACK_SWORD_AIR_02:   return EPlayerState::Attack_SwordAir_02;

    case Protocol::OBJECT_STATE_TYPE_HIT:                   return EPlayerState::Hit;
    case Protocol::OBJECT_STATE_TYPE_DASH:                  return EPlayerState::Dash;

    case Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN:        return EPlayerState::Skill_Rasengan;
    case Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN_AIR:    return EPlayerState::Skill_Rasengan_Air;
    case Protocol::OBJECT_STATE_TYPE_SKILL_RASENGAN_END:    return EPlayerState::Skill_Rasengan_End;
    case Protocol::OBJECT_STATE_TYPE_SKILL_RASENSHURIKEN:   return EPlayerState::Skill_RasenShuriken;
    case Protocol::OBJECT_STATE_TYPE_SKILL_RASENSHURIKEN_AIR: return EPlayerState::Skill_RasenShuriken_Air;
    case Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI:         return EPlayerState::Skill_Chidori;
    case Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI_AIR:     return EPlayerState::Skill_Chidori_Air;
    case Protocol::OBJECT_STATE_TYPE_SKILL_CHIDORI_END:     return EPlayerState::Skill_Chidori_End;
    case Protocol::OBJECT_STATE_TYPE_SKILL_FIREBALL:        return EPlayerState::Skill_FireBall;
    case Protocol::OBJECT_STATE_TYPE_SKILL_FIREBALL_AIR:    return EPlayerState::Skill_FireBall_Air;

    case Protocol::OBJECT_STATE_TYPE_DEAD:                  return EPlayerState::Dead;
    default:                                                return EPlayerState::Idle;
    }
}
```

---

## 3. 진행 순서
1. `Server/Protobuf/Bin/Enum.proto` 파일에 위 코드를 덮어씌웁니다.
2. Protobuf 변환 스크립트(예: `.bat` 모음 중 protoc 명령어)를 실행하여 패킷 C++/H 파일들을 갱신합니다.
3. `AnimationStateComponent.cpp`의 맨 밑에 존재하는 3개의 함수를 위 완성본 코드로 전부 대체합니다.
4. 클라이언트를 빌드하고 로컬 환경(혹은 멀티)에서 게임 진입 시, 상대방의 피격(`Hit`), 공격(`Attack`), 스킬(`Skill_*`) 애니메이션이 정상 동기화됨을 확인합니다.

> 위 작업 절차에 대해 동의해주시면, 직접 코드를 편집해 적용하겠습니다!
